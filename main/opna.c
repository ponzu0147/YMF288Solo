void reset() {
    gpio_set_level(GPIO_NUM_22, LOW);
    vTaskDelay(10 / portTICK_RATE_MS); // min. 25us
    gpio_set_level(GPIO_NUM_22, HIGH);
    vTaskDelay(100 / portTICK_RATE_MS);
}

void law_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
    gpio_set_level(GPIO_NUM_25, LOW);         // A0=LOW
    if (ifadr) {
    gpio_set_level(GPIO_NUM_26, HIGH);      // A1=HIGH
    } else {
    gpio_set_level(GPIO_NUM_26, LOW);       // A1=LOW
    }
    write_data(adr);                  // Address set

    gpio_set_level(GPIO_NUM_27, LOW);          // CS=LOW
    vTaskDelay(1 / portTICK_RATE_MS); // min: 200ns wait

    gpio_set_level(GPIO_NUM_27, HIGH);          // CS=HIGH
    vTaskDelay(2 / portTICK_RATE_MS); //  min: 1.9us wait

    gpio_set_level(GPIO_NUM_25, HIGH);        // A0=HIGH
    write_data(dat);                  // Data set

    gpio_set_level(GPIO_NUM_27, LOW);          // CS
    vTaskDelay(1 / portTICK_RATE_MS); // min: 200ns wait
    gpio_set_level(GPIO_NUM_27, HIGH);         // CS
}

void reg_write(unsigned char ifadr, unsigned char adr, unsigned char dat) {
    law_write(ifadr, adr, dat);
    switch (adr) { // Wait for data write
        case 0x28: { // FM Address 0x28
            vTaskDelay(25 / portTICK_RATE_MS);  // min: 24us wait
            break;
        }
        case 0x10: { // RHYTHM Address 0x10
            vTaskDelay(23 / portTICK_RATE_MS);  // min: 22us wait
            break;
        }
        default: { // Other Address
            vTaskDelay(2 / portTICK_RATE_MS);  // min.1.9us wait
        }
    }
}

void write_data(unsigned char dat) {
    int i;
    unsigned char pin_set[7];
    for ( i=0; i >= 7; i++ ){
        pin_set[i] = dat >> i & 0B0000001;
    }
    gpio_set_level( GPIO_NUM_0, pin_set[0] );
    gpio_set_level( GPIO_NUM_5, pin_set[1] );
    gpio_set_level( GPIO_NUM_12, pin_set[2] );
    gpio_set_level( GPIO_NUM_16, pin_set[3] );
    gpio_set_level( GPIO_NUM_17, pin_set[4] );
    gpio_set_level( GPIO_NUM_18, pin_set[5] );
    gpio_set_level( GPIO_NUM_19, pin_set[6] );
    gpio_set_level( GPIO_NUM_21, pin_set[7] );
}

