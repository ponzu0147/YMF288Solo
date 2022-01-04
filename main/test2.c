// s98dmp.exe  - s98 register dump tool

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "s98.h"
#include "log_common.h"

// 対応S98バージョン上限
#define MAXVER 3

// S98ヘッダ情報
unsigned char S98_ID[] = "S98";

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

// dword値を取得
int GetInt(unsigned char *_p) {
	int ret;
	ret = (_p[3]<<24) + (_p[2]<<16) + (_p[1]<<8) + _p[0];
	return ret;
}

// 時刻の表示
void PrintTime(int _t, int _timebase) {
	int time = _t;
	int hh,mm,ss,dd;
	hh = time / (_timebase*3600);
	time = _t % (_timebase*3600);
	mm = time / (_timebase*60);
	time = _t % (_timebase*60);
	ss = time / _timebase;
	dd = time % _timebase;
	printf("%02d:%02d:%02d.%03d",hh,mm,ss,dd);
	return;
}

int main(int argc, char *argv[]) {
	unsigned char s98head[HEADER_SIZE_MAX];
	FILE *fp;
	int i=0,j=0;
	int dumpmode = 1;
	int detaillog = 0;
	int forceversion = 0;
	int dt,dt1,dt2,dc;
	int dumpend = 0x7fffffff;
	int time = 0;
	int count = 0;
	int filesize = 0;

	if (argc<2) {
		// オプション指定がないとき
		fprintf(stderr, "S98DMP.EXE [S98 File] [-d] [-v] [-f]\n");
		fprintf(stderr, "    -d : do not output dump data\n");
		fprintf(stderr, "    -v : show detail log (PSG/OPN/OPN2/OPNA/OPM only)\n");
		fprintf(stderr, "    -f : do not check S98 format version\n");
		return 1;
	}
	if (argc>2) {
		for (i=2; i<argc; i++) {
			if (strcmp(argv[i],"-d")==0) dumpmode=0;
			if (strcmp(argv[i],"-v")==0) detaillog=1;
			if (strcmp(argv[i],"-f")==0) forceversion=1;
		}
	}

	// S98ヘッダ読み込み
	fp = fopen(argv[1],"rb");
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
	double timer_period = ((double)timer_info1/(double)timer_info2)*1000;
	int tag_offset    = GetInt(&s98head[TAG_OFFSET]);
	int data_offset   = GetInt(&s98head[DATA_OFFSET]);
	int loop_offset   = GetInt(&s98head[LOOP_OFFSET]);
	int device_count  = GetInt(&s98head[DEVICE_COUNT]);
	// タグがデータより後にある場合の対処
	if (data_offset < tag_offset) dumpend = tag_offset;

	// エラー処理
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

	int device_type = YM2608;	// S98v1初期値(OPNA)
	int clock = 7987200;
	int pan = 0xff;

	// DEVICE INFO読み込み
	if (fread(&s98head[HEADER_SIZE],1,0x10*device_count,fp)!=0x10*device_count) return 1;

	// ヘッダデータ表示
	printf("[ S98 Information ]\n");
	printf("Filename       = %s\n", argv[1]);
	printf("Format Version = %c\n", '0'+s98_version);
	printf("Timer Period   = %d/%d (%f msec)\n", timer_info1, timer_info2, timer_period);
	printf("Tag Offset     = 0x%08x",tag_offset);
	if (tag_offset) printf("\n"); else printf(" (no tag)\n");
	printf("Data Offset    = 0x%08x\n",data_offset);
	printf("Loop Offset    = 0x%08x",loop_offset);
	if (loop_offset) printf("\n"); else printf(" (no loop)\n");
	printf("Device Count   = %d\n\n",device_count);

	// デバイス表示
	printf("[ Device List ]\n");
	device_num = device_count ? device_count : 1;
	char *device_name;

	for (i=0; i<device_num; i++) {
		if (device_count) {
			device_type = GetInt(&s98head[HEADER_SIZE+i*0x10     ]);
			clock       = GetInt(&s98head[HEADER_SIZE+i*0x10+0x04]);
			pan         = GetInt(&s98head[HEADER_SIZE+i*0x10+0x08]);
		}
		device_name = (device_type <= MAXCHIPTYPE)? chip[device_type] : chipunknown;
		printf("#%02d %-16s Clock: %dHz  PAN: ",i+1,device_name,clock);
		for (j=7; j>=0; j--) {
			char c;
			c = (pan & (1<<j))? '1' : '0';
			printf("%c",c);
		}
		printf("\n");
	}

	if (dumpmode) {
		// ダンプデータ表示
		printf("\n[ Dump Data ]\n");
		fseek(fp, data_offset, SEEK_SET);
		int ofs = data_offset;
		bool flag = true;
		int n,s;
		while (flag && ofs<dumpend) {
			dt = fgetc(fp);
			if (feof(fp)) break;
			dc = 1;
			printf("%08x : %02x ",ofs,dt);
			switch (dt) {
			case 0xff:
				time += timer_info1;
				count++;
				printf("\t\t[ SYNC 1 (%d)]\t",count);
				PrintTime(time, timer_info2);
				break;
			case 0xfe:
				n=0, s=0;
				while (1) {
					if (feof(fp)) {
						fprintf(stderr,"\nfound unexpected EOF.  ofs=%08x\n",ofs);
						exit(1);
					}
					dt = fgetc(fp);
					printf("%02x ",dt);
					dc++;
					n |= (dt & 0x7f) << s;
					s += 7;
					if (!(dt & 0x80)) break;
				}
				time += (n+2)*timer_info1;
				count += n+2;
				printf("\t[ SYNC %d (%d) ]\t",n+2, count);
				PrintTime(time, timer_info2);
				break;
			case 0xfd:
				if (loop_offset) {
					printf("\t\t[ LOOP OFFSET=%08x ]",loop_offset);
				}
				else {
					printf("\t\t[ END ]");
				}
				flag = false;
				break;

			default:
				int dev_no = dt/2;
				int dev_ex = dt&1;
				dt1 = fgetc(fp);
				dt2 = fgetc(fp);
				if (feof(fp)) {
					fprintf(stderr,"\nfound unexpected EOF.  ofs=%08x\n",ofs);
					exit(1);
				}
//				printf("%02x %02x",dt1,dt2);

				if (device_count) device_type = GetInt(&s98head[HEADER_SIZE+dev_no*0x10]);
				if (device_count && (dev_no > device_count)) {
					fprintf(stderr,"\nfound corrupted data.  ofs=%08x : %02x %02x %02x\n",ofs, dt, dt1, dt2);
					exit(1);
				}

				device_name = (device_type <= MAXCHIPTYPE)? chip[device_type] : chipunknown;
				printf("%02x %02x\t[ #%02d %s ] ",dt1,dt2,dev_no,device_name);
				dc += 2;

				// 詳細ログオプション表示が無効の時はここで終わり
				if (!detaillog) break;

				// 詳細ログ表示
				switch (device_type) {

				case YM2149:
				case YM2203:
				case YM2612:
				case YM2608:
				case AY_3_8910:
					log_opna( dev_no, dev_ex, dt1, dt2 );
					break;

				case YM2151:
					log_opm( dev_no, dev_ex, dt1, dt2 );
					break;

				case YM2413:
					log_opll( dev_no, dt1, dt2 );
					break;

				case YM3526:
				case YM3812:
				case YMF262:
					log_opl3( dev_no, dev_ex, dt1, dt2 );
					break;

				case SN76489:
					log_dcsg( dev_no, dt2 );
					break;

				default:
					break;
				}
			}
			if (ofs==loop_offset) printf(" *** LOOP POINT ***");
			ofs += dc;
			printf("\n");
		}
	}
	fclose(fp);

	return 0;
}

