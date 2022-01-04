#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include "s98.h"
#include <esp32/rom/ets_sys.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"

#include <stdio.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
	#include "sdkconfig.h"

#include <stdlib.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;

static xQueueHandle s_timer_queue;

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    BaseType_t high_task_awoken = pdFALSE;
    example_timer_info_t *info = (example_timer_info_t *) args;

    uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx);

    /* Prepare basic event data that will be then sent back to task */
    example_timer_event_t evt = {
        .info.timer_group = info->timer_group,
        .info.timer_idx = info->timer_idx,
        .info.auto_reload = info->auto_reload,
        .info.alarm_interval = info->alarm_interval,
        .timer_counter_value = timer_counter_value
    };

    if (!info->auto_reload) {
        timer_counter_value += info->alarm_interval * TIMER_SCALE;
        timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
    }

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);

    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE / 100 ); // 10ms
    timer_enable_intr(group, timer);

    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    timer_info->timer_group = group;
    timer_info->timer_idx = timer;
    timer_info->auto_reload = auto_reload;
    timer_info->alarm_interval = timer_interval_sec/1000;
    timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0);

    timer_start(group, timer);
}

static void check_efuse()
{
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }

    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}


// S98 header
unsigned char S98_ID[] = "S98";

// loop_count
int count_ff;

// buttons flag
volatile bool f_flag, p_flag, r_flag, q_flag, s_flag;
volatile bool switch_off = false;

// Max S98 version
#define MAXVER 3

char chipunknown[] = "[ UNKNOWN ]";
char *chip[] = {
	"NONE",				// 0
	"PSG(YM2149)",		// 1
	"OPN(YM2203)",		// 2
	"OPN2(YM2612)",		// 3
	"OPNA(YM2608)",		// 4
	"OPM(YM2151)",		// 5
	"OPLL(YM2413)",		// 6
	"OPL(YM3526)",		// 7
	"OPL2(YM3812)",		// 8
	"OPL3(YMF262)",		// 9
	"[ UNKNOWN ]",		// 10
	"[ UNKNOWN ]",		// 11
	"[ UNKNOWN ]",		// 12
	"[ UNKNOWN ]",		// 13
	"[ UNKNOWN ]",		// 14
	"PSG(AY-3-8910)",	// 15
	"DCSG(SN76489)"		// 16
};

static const char *TAG = "sdcard";

volatile int rep=1;

#define MOUNT_POINT "/sdcard"

// Pin mapping
#if CONFIG_IDF_TARGET_ESP32

#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

#elif CONFIG_IDF_TARGET_ESP32S2

// adapted for internal test board ESP-32-S3-USB-OTG-Ev-BOARD_V1.0 (with ESP32-S2-MINI-1 module)
#define PIN_NUM_MISO 37
#define PIN_NUM_MOSI 35
#define PIN_NUM_CLK  36
#define PIN_NUM_CS   34

#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO 6
#define PIN_NUM_MOSI 4
#define PIN_NUM_CLK  5
#define PIN_NUM_CS   1

#endif //CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2

#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN    host.slot
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#else
#define SPI_DMA_CHAN    1
#endif

#define LOW 0
#define HIGH 1


unsigned char pin_set[7];
int S98_read(char* s98file);	

//fgets data strings
int fgsds(char **buf, const char *fname, int lines, int *len)
{
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL) {
		printf("file open error!\n");
		return -1;
	}
	for (int i = 0; i < lines && fgets(buf[i], len[i] + 1, fp) != NULL; i++) {}
	fclose(fp);
	return 0;
}
 
//fgets list
int fgl(const char *fname)
{
	FILE *fp = fopen(fname, "rb");
	int c;
	int lines = 0; //count lines
	if (fp == NULL) {
		printf("file open error!\n");
		return -1;
	}
	
	while (1) {
		c = fgetc(fp);
		if (c == '\n') { 
			lines++;
		} else if (c == EOF) { //EOF is include
			lines++;
			break;
		}
	}
	
	fclose(fp);
	return lines;
}
 
//fgets length
int fglen(int *len, const char *fname)
{
	FILE *fp = fopen(fname, "rb");
	int c;
	int i = 0;
	int byt = 0; // count for byte
	int bytc = 0; // copy of byt
	if (fp == NULL) {
		printf("file open error!\n");
		return -1;
	}
	
	while (1) {
		c = fgetc(fp);
		byt++;
		if (c == '\n') {
			len[i] = byt - bytc;
			i++;
			bytc = byt;
		} else if (c == EOF) {
			if ((byt - bytc) <= 1) { // if return with /n
				len[i] = 0;
			} else {
				len[i] = byt - bytc; // if return without /n
			}
			break;
		}
	}
	
	fclose(fp);
	return 0;
}

void reset() {
    //YMF288 D0-D7
    gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);

    gpio_set_direction(GPIO_NUM_25, GPIO_MODE_OUTPUT); // A0_
    gpio_set_direction(GPIO_NUM_26, GPIO_MODE_OUTPUT); // A1_
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT); // WR
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT); // CS
    gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT); // RST

	gpio_set_direction(GPIO_NUM_34, GPIO_MODE_INPUT); // 3 buttons

    gpio_set_level(GPIO_NUM_23, HIGH);
    gpio_set_level(GPIO_NUM_27, HIGH);


    gpio_set_level(GPIO_NUM_22, LOW);
    ets_delay_us(1000);
    gpio_set_level(GPIO_NUM_22, HIGH);
    ets_delay_us(1000);

}

void write_data(unsigned char dat) {
    u_int32_t pin_set[8];
    /*
    char* pin_name[] = {
        "GPIO_PIN_33",
        "GPIO_PIN_5",
        "GPIO_PIN_4",
        "GPIO_PIN_16",
        "GPIO_PIN_17",
        "GPIO_PIN_18",
        "GPIO_PIN_19",
        "GPIO_PIN_21"
    };*/
     for ( int i=7; i >= 0; i-- ){
        pin_set[i] = (dat >> i ) & 1;
     }
        if (pin_set[0]) {
            GPIO.out1_w1ts.val = (1 << 1);
        } else {
            GPIO.out1_w1tc.val = (1 << 1);
        }
        if (pin_set[1]) {
            GPIO.out_w1ts = (1 << 5);
        } else {
            GPIO.out_w1tc = (1 << 5);
        }
        if (pin_set[2]) {
            GPIO.out_w1ts = (1 << 4);
        } else {
            GPIO.out_w1tc = (1 << 4);
        }
        if (pin_set[3]) {
            GPIO.out_w1ts = (1 << 16);
        } else {
            GPIO.out_w1tc = (1 << 16);
        }
        if (pin_set[4]) {
            GPIO.out_w1ts = (1 << 17);
        } else {
            GPIO.out_w1tc = (1 << 17);
        }
        if (pin_set[5]) {
            GPIO.out_w1ts = (1 << 18);
        } else {
            GPIO.out_w1tc = (1 << 18);
        }
        if (pin_set[6]) {
            GPIO.out_w1ts = (1 << 19);
        } else {
            GPIO.out_w1tc = (1 << 19);
        }
        if (pin_set[7]) {
            GPIO.out_w1ts = (1 << 21);
        } else {
            GPIO.out_w1tc = (1 << 21);
        }

/*
    gpio_set_level( GPIO_NUM_33, pin_set[0] );
    gpio_set_level( GPIO_NUM_5, pin_set[1] );
    gpio_set_level( GPIO_NUM_4, pin_set[2] );
    gpio_set_level( GPIO_NUM_16, pin_set[3] );
    gpio_set_level( GPIO_NUM_17, pin_set[4] );
    gpio_set_level( GPIO_NUM_18, pin_set[5] );
    gpio_set_level( GPIO_NUM_19, pin_set[6] );
    gpio_set_level( GPIO_NUM_21, pin_set[7] );
*/    
}

void law_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
    gpio_set_level(GPIO_NUM_25, LOW);       // A0=LOW
    if (ifadr) {
        gpio_set_level(GPIO_NUM_26, HIGH);  // A1=HIGH
    } else {
        gpio_set_level(GPIO_NUM_26, LOW);   // A1=LOW
    }
    write_data(adr);                  		// Address set
    gpio_set_level(GPIO_NUM_27, LOW);       // CS=LOW
    ets_delay_us(2);
    gpio_set_level(GPIO_NUM_27, HIGH);      // CS=HIGH
    ets_delay_us(1);

    gpio_set_level(GPIO_NUM_25, HIGH);      // A0=HIGH
    write_data(dat);                  		// Data set
    gpio_set_level(GPIO_NUM_27, LOW);       // CS
    ets_delay_us(2);
    gpio_set_level(GPIO_NUM_27, HIGH);      // CS
    //ets_delay_us(1);
}

void reg_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
    law_write(ifadr, adr, dat);
    switch (adr) { 							// Wait for data write
        case 0x28: { 						// FM Address 0x28
            ets_delay_us(29);  				// min: 24us wait
            break;
        }
        case 0x10: { 						// RHYTHM Address 0x10
            ets_delay_us(28);  				// min: 22us wait
            break;
        }
        default: { 							// Other Address
            ets_delay_us(4);  				// min.1.9us wait
        }
    }
}

// Convert byte to dword
int GetInt(unsigned char *_p) {
	int ret;
	ret = (_p[3]<<24) + (_p[2]<<16) + (_p[1]<<8) + _p[0];
	return ret;
}

void lntrim(char *str) {  
  char *p;  
  p = strchr(str, '\n');  
  if(p != NULL) {  
    *p = '\0';  
  }  
}  

char* cat_path(char* str1) {
    char* cp = NULL;
    cp = (char*)malloc(256);
    
    if (cp == NULL) {
        ESP_LOGE(TAG,"Can't create array!");
    }

    strcat(strcpy(cp,MOUNT_POINT),"/");
    strcat(cp,str1);
    ESP_LOGI(TAG,"Path: %s",cp);

    return cp;
}

char* cat_path_file(char* path, char* file) {
    //char* str1 = entry->d_name;
    char* cp = NULL;
    cp = (char*)malloc(256);
    
    if (cp == NULL) {
        ESP_LOGE(TAG,"Can't create array!");
    }

    strcat(strcpy(cp,path),"/");
    strcat(cp,file);
    ESP_LOGI(TAG,"Path  : %s",cp);

    return cp;
}

void S98_play2(char* s98file) {
    const char *ext = strrchr(s98file, '.');

    if (strcmp(ext,".S98") == 0 || strcmp(ext,".s98") == 0) {
            ESP_LOGI(TAG, "Extension is match. (%s)", ext);

            // FILE *fp;
			reset();
            ESP_LOGI(TAG, "Reading %s", s98file);

            if ( (S98_read(s98file)) == 1) {
                ESP_LOGE(TAG, "File open error! (%s)", s98file);
            } else {
                ESP_LOGI(TAG, "%s is close.", s98file);
            }
        } else {
            ESP_LOGI(TAG, "Extension is not match. (%s)", ext);
        }
    return;
}

void read_from_list(void* arg) {

    esp_err_t ret;

		esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
			.format_if_mount_failed = true,
	#else
			.format_if_mount_failed = false,
	#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
			.max_files = 5,
			.allocation_unit_size = 16 * 1024
		};
		sdmmc_card_t *card;


		char mount_point[] = MOUNT_POINT;
		ESP_LOGI(TAG, "Initializing SD card");

		ESP_LOGI(TAG, "Using SPI peripheral");

		sdmmc_host_t host = SDSPI_HOST_DEFAULT();
		host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
		spi_bus_config_t bus_cfg = {
			.mosi_io_num = PIN_NUM_MOSI,
			.miso_io_num = PIN_NUM_MISO,
			.sclk_io_num = PIN_NUM_CLK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 16000,
		};
		ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to initialize bus.");
			return;
		}

		sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
		slot_config.gpio_cs = PIN_NUM_CS;
		slot_config.host_id = host.slot;

		ESP_LOGI(TAG, "Mounting filesystem");
		ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

		if (ret != ESP_OK) {
			if (ret == ESP_FAIL) {
				ESP_LOGE(TAG, "Failed to mount filesystem. "
						"If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
			} else {
				ESP_LOGE(TAG, "Failed to initialize the card (%s). "
						"Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
			}
			return;
		}
		ESP_LOGI(TAG, "Filesystem mounted");

		sdmmc_card_print_info(stdout, card);

    FILE *lp;
    DIR *dir;
    int k=0;

    const char *list_file2 = MOUNT_POINT"/s98.lst";

    if ((dir = opendir(MOUNT_POINT)) == NULL) {
      ESP_LOGE(TAG, "Directory is not found.");
    } else {

    lp = fopen(list_file2,"r");
    if (lp == NULL) {
        fprintf(stderr, "List open error.\n");
        return;
    }

	// fgsds
	int lines = fgl(list_file2); //fgl関数を用いて、ファイルの行数を取得
	int *len = (int *)malloc(lines * sizeof(int)); //int型の配列lenの配列数（行数）を設定
	fglen(len, list_file2); //fglen関数を用いて、配列lenに行ごとのバイト数を入れる
	char **buf = (char **)malloc(lines * sizeof(char *)); //格納配列[buf]に行数を確保
	for (int i = 0; i < lines; i++) {
		buf[i] = (char *)malloc((len[i] + 1) * sizeof(char)); //各行に文字列を格納するメモリを確保
	}
	fgsds(buf, list_file2, lines, len); //fgsds関数を用いて、二次元配列bufにファイルの文字列データを格納

    // Read a list from file
    while(1){

		if(q_flag == true){
			k=k-1;
			if(k<=0){
				k=lines-2;
			} else {
				k = k- 1;
			}
			q_flag = false;
		}

		//改行コードを削除する
		int last = strlen(buf[k]) - 1;
		if (last >= 0 && buf[k][last] == '\n') {
			buf[k][last] = '\0';
		} 
        if(k==lines-1){
            fseek(lp,0,SEEK_SET); // 先頭に戻る
            k=0;
        } else {
			printf("Current number: %d\n",k);
			printf("Playing file: %s\n",buf[k]);
            S98_play2(buf[k]);
			k++;
        }
    }

	//メモリの解放
	for (int i = 0; i < lines; i++) {
		free(buf[i]); //各行のメモリを解放
	}
	free(buf);
	free(len);

    fclose(lp);
    closedir(dir);
    }
}

int S98_read(char* s98file) {

    unsigned char s98head[HEADER_SIZE_MAX];
	FILE *fp;
    //DIR *dir2;
	int i=0;
    //int j=0;
	int dumpmode = 1;
	//int detaillog = 0;
	int forceversion = 0;
	int dt1,dt2,dc;
	int dumpend = 0x7fffffff;
	int time = 0;
	int count = 0;
	int filesize = 0;

    printf("Openfile: %s\n",s98file);

	// S98ヘッダ読み込み
	fp = fopen(s98file,"rb");
	if (!fp) {
		fprintf(stderr, "File open error.\n");
		return 1;
	}
	// ファイルサイズを取得
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (fread(s98head, 1, HEADER_SIZE, fp)!=HEADER_SIZE) {
		fprintf(stderr, "S98 header read error.\n");
		return 1;
	}
	if (memcmp(s98head,S98_ID,3)) {
		fprintf(stderr, "This is not S98 format.\n");
		return 1;
	}

	int s98_version   = s98head[S98_VERSION] - '0';

	if (((s98_version==2) || s98_version>MAXVER) && !forceversion) {
		fprintf(stderr, "S98 version error.\n");
		return 1;
	}

	int timer_info1   = GetInt(&s98head[TIMER_INFO]);
	if (!timer_info1) timer_info1 = 10;
	int timer_info2   = GetInt(&s98head[TIMER_INFO2]);
	if (!timer_info2) timer_info2 = 1000;
	int tag_offset    = GetInt(&s98head[TAG_OFFSET]);
	int data_offset   = GetInt(&s98head[DATA_OFFSET]);
	int loop_offset   = GetInt(&s98head[LOOP_OFFSET]);
	int device_count  = GetInt(&s98head[DEVICE_COUNT]);

	if (data_offset < tag_offset) dumpend = tag_offset;

	// error handle
	if ((data_offset < HEADER_SIZE) || (data_offset > filesize)) {
		fprintf(stderr,"data offset error. %08x\n", data_offset);
		return 1;
	}
	if ((tag_offset && (tag_offset < HEADER_SIZE)) || (tag_offset > filesize)) {
		fprintf(stderr,"tag offset error. %08x\n", tag_offset);
		return 1;
	}
	if ((loop_offset && (loop_offset < data_offset)) || (loop_offset > filesize)) {
		fprintf(stderr,"loop offset error. %08x\n",loop_offset);
		return 1;
	}
	if (device_count && (device_count > MAXDEVICE)) {
		fprintf(stderr,"device number error. (MAX=%d) %d\n",MAXDEVICE, device_count);
		return 1;
	}

	int device_num = 1;

	int device_type = YM2608;	// S98v1 default(OPNA)

	// DEVICE INFO reading
	if (fread(&s98head[HEADER_SIZE],1,0x10*device_count,fp)!=0x10*device_count) return 1;

	device_num = device_count ? device_count : 1;

	for (i=0; i<device_num; i++) {
		if (device_count) {
			device_type = GetInt(&s98head[HEADER_SIZE+i*0x10     ]);
		}
	}

    dumpmode = true;
	if (dumpmode) {
		// 
		fseek(fp, data_offset, SEEK_SET);
		int ofs = data_offset;
		int loop_count = 3;
		bool flag = true;
        int dt = 0;
		int n,s;
		while (flag && ofs<dumpend) {
            if (f_flag == true || p_flag == true || r_flag == true){ // 曲送り
                if (f_flag == true){
                    dt = 0xfd; // done.
                    loop_count = 1;
                    f_flag = false;
					s_flag = true;
        			vTaskDelay(pdMS_TO_TICKS(450));
					break;
                } else if (p_flag == true){ // 曲の先頭に戻る
					reset();
					vTaskDelay(pdMS_TO_TICKS(150));
                    fseek(fp, data_offset, SEEK_SET);
					loop_count = 3;
					p_flag = false;
                } else if (r_flag == true){
					dt = 0xfd; // done.
					loop_count = 3;
					r_flag = false;
					q_flag = true;
					reset();
					break;
				}
            } else {
			    dt = fgetc(fp);
            }
			if (feof(fp)) break;
			dc = 1;
			switch (dt) {
			case 0xff: {
				time += timer_info1;
				if(q_flag == false){
					count++;
					count_ff = count;
				}
                vTaskDelay(1/0.95 / portTICK_RATE_MS * timer_info1 / timer_info2 * 1000);
				break;
                }
			case 0xfe: {
				n=0, s=0;
				while (1) {
					if (feof(fp)) {
						fprintf(stderr,"\nfound unexpected EOF.  ofs=%08x\n",ofs);
						exit(1);
					}
					dt = fgetc(fp);
					dc++;
					n |= (dt & 0x7f) << s;
					s += 7;
					if (!(dt & 0x80)) break;
				}
				time += (n+2)*timer_info1;
				count += n+2;
                vTaskDelay((n+2)/0.98 / portTICK_RATE_MS * timer_info1 / timer_info2 * 1000);
				break;
                }
			case 0xfd:{
				if (loop_offset) {
					loop_count--;
					printf("loop count: %d\n",loop_count);
					if (loop_count > 0){
						printf("loop music.\n");
						fseek(fp, loop_offset, SEEK_SET);
						break;
					} else {
                        printf("file end.\n");
                    }
				}
				flag = false;
				break;
                }

			default: {
				int dev_no;
                dev_no = dt/2;
				dt1 = fgetc(fp);
				dt2 = fgetc(fp);

				if (dt == 0x00){
					if(dt1 == 0x08 || dt1 == 0x09 || dt1 == 0x0a){
						//char mod_vol[16]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} 	// Default
						//char mod_vol[16]= {0,1,2,4,5,6,7,8,9,11,12,13,14,14,15,15}; 	// Curve 1
						//char mod_vol[16]= {0,1,3,4,5,6,8,9,10,11,13,14,15,15,15,15}; 	// Curve 2
						//char mod_vol[16]= {0,2,4,5,6,8,10,11,12,13,14,15,15,15,15,15};// Curve 3
						//char mod_vol[16]= {0,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15}; 	// Curve 4
						char mod_vol[16]= {0,4,5,6,7,8,9,10,11,12,13,14,15,15,15,15}; 	// Straight strong
						//char mod_vol[16]= {0,3,4,5,6,7,8,9,10,11,12,13,14,15,15,15};	// Straight medium
						//char mod_vol[16]= {0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15};	// Straight weak

						dt2 = mod_vol[dt2];
					}

				}

				if (feof(fp)) {
					fprintf(stderr,"\nfound unexpected EOF.  ofs=%08x\n",ofs);
					exit(1);
				}

				if (device_count) device_type = GetInt(&s98head[HEADER_SIZE+dev_no*0x10]);
				if (device_count && (dev_no > device_count)) {
					fprintf(stderr,"\nfound corrupted data.  ofs=%08x : %02x %02x %02x\n",ofs, dt, dt1, dt2);
					exit(1);
				}

				// Device check
				switch (device_type) {

                    case YM2149:
                    case YM2203:
                    case YM2612:
                    case YM2608:
                    case AY_3_8910:
                        reg_write(dt,dt1,dt2);
                        break;

                    case YM2151:
                        //reg_write_opm( dev_no, dt, dt1, dt2 );
                        break;

                    case YM2413:
                        //reg_write_opll( dt, dt1, dt2 );
                        break;

                    case YM3526:
                    case YM3812:
                    case YMF262:
                        //reg_write_opl3( dev_no, dt, dt1, dt2 );
                        break;

                    case SN76489:
                        //reg_write_dcsg( dt, dt2 );
                        break;

                    default:
                        break;
                }
			}
			if (ofs==loop_offset) //printf(" *** LOOP POINT ***");
			ofs += dc;
			//printf("\n");
		    }
        }
    }
	fclose(fp);
    //closedir(dir);

	return 0;
}



void S98_play(char* path, char* s98file) {
    const char *ext = strrchr(s98file, '.');

    if (strcmp(ext,".S98") == 0 || strcmp(ext,".s98") == 0) {
            ESP_LOGI(TAG, "Extension is match. (%s)", ext);

            // FILE *fp;
			reset();
            char* cp = NULL;
            cp = cat_path_file(path, s98file);
            ESP_LOGI(TAG, "Reading %s", cp);

            if ( (S98_read(cp)) == 1) {
                ESP_LOGE(TAG, "File open error! (%s)", cp);
                
            } else {
                ESP_LOGI(TAG, "%s is close.", cp);
                free(cp);
            }
        } else {
            ESP_LOGI(TAG, "Extension is not match. (%s)", ext);
        }
    return;
}

void key_check(void* arg) {
    while(1){
    example_timer_event_t evt;
    xQueueReceive(s_timer_queue, &evt, portMAX_DELAY);

    /* Print information that the timer reported an event */
    if (evt.info.auto_reload) {
        //printf("Timer Group with auto reload\n");
    } else {
        //printf("Timer Group without auto reload\n");
    }

    uint32_t adc_reading = 0;
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;
    //Convert adc_reading to voltage in mV
    // uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
    if((int)adc_reading<200){
        printf("button 1\n");
        adc_reading=4095;
        f_flag = true;
    } else if(adc_reading<2000) {
        printf("button 2\n");
        adc_reading=4095;
        p_flag = true;
    } else if(adc_reading<2700) {
        printf("button 3\n");
        vTaskDelay(pdMS_TO_TICKS(250));
        adc_reading=4095;
        r_flag = true;

    } else {
        //printf("no button\n");
    }
    // /printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
    vTaskDelay(pdMS_TO_TICKS(250));
    }

}

void s98file_check(void){
    esp_err_t ret;

		esp_vfs_fat_sdmmc_mount_config_t mount_config = {
	#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
			.format_if_mount_failed = true,
	#else
			.format_if_mount_failed = false,
	#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
			.max_files = 5,
			.allocation_unit_size = 16 * 1024
		};
		sdmmc_card_t *card;


		char mount_point[] = MOUNT_POINT;
		ESP_LOGI(TAG, "Initializing SD card");

		ESP_LOGI(TAG, "Using SPI peripheral");

		sdmmc_host_t host = SDSPI_HOST_DEFAULT();
		host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
		spi_bus_config_t bus_cfg = {
			.mosi_io_num = PIN_NUM_MOSI,
			.miso_io_num = PIN_NUM_MISO,
			.sclk_io_num = PIN_NUM_CLK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 16000,
		};
		ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to initialize bus.");
			return;
		}

		sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
		slot_config.gpio_cs = PIN_NUM_CS;
		slot_config.host_id = host.slot;

		ESP_LOGI(TAG, "Mounting filesystem");
		ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

		if (ret != ESP_OK) {
			if (ret == ESP_FAIL) {
				ESP_LOGE(TAG, "Failed to mount filesystem. "
						"If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
			} else {
				ESP_LOGE(TAG, "Failed to initialize the card (%s). "
						"Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
			}
			return;
		}
		ESP_LOGI(TAG, "Filesystem mounted");

		// Card has been initialized, print its properties
		sdmmc_card_print_info(stdout, card);



		DIR *dir, *dir_sub;
		struct stat sb, sbsub;
		struct dirent *entry, *entry2;
		int s_count, d_count, f_count;
		int d_subct, f_subct;
		char* str1 = NULL;

        const char *list_file = MOUNT_POINT"/s98.lst";
        FILE *f = fopen(list_file, "w"); // text, overwrite
        if (f == NULL) {
            ESP_LOGE(TAG, "Failed to open file for writing");
        return;
        }
		if ((dir = opendir(MOUNT_POINT)) == NULL) {
		ESP_LOGE(TAG, "Directory is not found.");
		} else {
			s_count = 0;
			d_count = 0;
			f_count = 0;
			d_subct = 0;
			f_subct = 0;

			while ((entry = readdir(dir)) != NULL) {
				stat(entry->d_name, &sb);
				if (entry->d_type & DT_DIR) {
					if ( !(strcmp(entry->d_name,"System Volume Information"))) {
						ESP_LOGI(TAG, "System directory entry %03d: %s", ++s_count, entry->d_name);
					} else {
						ESP_LOGI(TAG, "Normal directory entry %03d: %s", ++d_count, entry->d_name);

						// get fullpath directory
						str1 = cat_path(entry->d_name);
						if ((dir_sub = opendir(str1)) == NULL) {
							ESP_LOGI(TAG, "directory can't open.");
						} else {
							ESP_LOGI(TAG, "Entering directory : %s", str1);
							while ((entry2 = readdir(dir_sub)) != NULL) {
								stat(entry2->d_name, &sbsub);
								if (entry2->d_type & DT_DIR) {
									ESP_LOGI(TAG, "Sub directory entry %03d: %s", ++d_subct, entry2->d_name);
								} else {
									ESP_LOGI(TAG, "Sub  file entry %03d: %s", ++f_subct, entry2->d_name);

									const char *ext = strrchr(entry2->d_name, '.');

                                    if (strcmp(ext,".S98") == 0 || strcmp(ext,".s98") == 0) {
                                        ESP_LOGI(TAG, "Extension is match. (%s)", ext);

                                        // FILE *fp;
                                        char* cp = NULL;
                                        cp = cat_path_file(str1, entry2->d_name);
                                        //ESP_LOGI(TAG, "Reading %s", cp);
                                        printf("%s\n",cp);
                                        fprintf(f, "%s\n", cp);
                                    } else {
                                        ESP_LOGI(TAG, "Extension is not match. (%s)", ext);
                                    }
                               }
							}
							ESP_LOGI(TAG, "Leaving directory : %s", str1);
							free(str1);
							ESP_LOGI(TAG, "Array memory cleared.");
							d_subct=0;
							f_subct=0;
							closedir(dir_sub);

						}
					}
				} else {
					ESP_LOGI(TAG, "Root file entry %03d: %s", ++f_count, entry->d_name);
					const char *ext = strrchr(entry->d_name, '.');

                    if (strcmp(ext,".S98") == 0 || strcmp(ext,".s98") == 0) {
                        ESP_LOGI(TAG, "Extension is match. (%s)", ext);

                        // FILE *fp;
                        char* cp = NULL;
                        cp = cat_path_file(str1, entry->d_name);
                        //ESP_LOGI(TAG, "Reading %s", cp);
                        printf("%s\n",cp);
                        fprintf(f, "%s\n", cp);
                    } else {
                        ESP_LOGI(TAG, "Extension is not match. (%s)", ext);
                    }
				}

			}
			closedir(dir);
		}

        fclose(f);

		// All done, unmount partition and disable SPI peripheral
		esp_vfs_fat_sdcard_unmount(mount_point, card);
		ESP_LOGI(TAG, "Card unmounted");

		//deinitialize the bus after all devices are removed
		spi_bus_free(host.slot);
}

void app_main(void)
{
    reset();

    //Check if Two Point or Vref are burned into eFuse
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    s_timer_queue = xQueueCreate(10, sizeof(example_timer_event_t));

    example_tg_timer_init(TIMER_GROUP_0, TIMER_0, true, 1);

    //Write S98 list file
    s98file_check();

    xTaskCreate(&key_check, "Task1", 2048, NULL, 5, NULL);
    xTaskCreate(&read_from_list, "Task2", 8192, NULL, 5, NULL);

}
