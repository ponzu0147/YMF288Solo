/*
    a2p.c programmed by ponzu0147
    input :  4bit OPNA ADPCM
    output: 16bit PCM (h,l)
*/

#include <stdio.h>
#include <stdlib.h>

int delta_n = 127;
int delta_x = 0;
int delta_x2 = 0;
int bit[8];

const int mode_f[] = {
    57,57,57,57,77,102,128,153,
    57,57,57,57,77,102,128,153
};

// BigEndian to LitteleEndian
int xcg_int(int _a){
    int a = _a;
    int ret = 0;
    ret = a >> 8;
    int ret2 = 0;
    ret2 = (a & 0xff); 
    a = ret + (ret2 << 8);
    return a;
}

int get_deltan(int cc, int ddelta_n){
    int nplus;
    int f = mode_f[cc] ;
    //printf(" f = %03d", f);
    nplus = (f * ddelta_n) >> 6; // Divide by 64.
    //printf(" n+ = %03d ",nplus);

    if (nplus < 128) {
        nplus = 127;
    } else if ( nplus > 24576 ) {
        nplus = 24576;
    }
    delta_n = nplus;
    return nplus;
}

int get_x(int _data, int _delta_x, int _delta_n){
    u_int8_t c = _data;
    int d;
    int n;
    int16_t x;

    for (int i = 3; i >= 0; i--) {
        bit[i] = (c >> i) & 1;
        //printf("%x",bit[i]);
    }

    int delta_n = get_deltan(c,_delta_n);
    int parts_x;
    c = c & 7;
    c = c << 1 + 1;

    //printf("(delta_n: %05d) ",delta_n);
    parts_x= (1 - 2 * bit[3]) * (8 * bit[2] + 4 * bit[1] + 2 * bit[0] + 1) * delta_n;
    if(bit[3] == 1){
        parts_x = (parts_x >> 3) +1;
    } else {
        parts_x = parts_x >> 3;
    }
    x = parts_x + _delta_x;

    // 2's complement
    if (x < 0){
        d = x;
        x = d ^ 0xffffffff;
        x = x + 1;
        }

    //printf("x= %03d ", x);
    return x; // Xn+1
}

void main(void){
    FILE *fp, *pp;
    int c;
    int ch,cl;
    //int num = 0;
    fp=fopen("12.pcm","rb");
    pp=fopen("12.pc8","wb");

    while(1){
        c = fgetc(fp);
        if(feof(fp)) break;

        //printf("%05d ",num);

        ch = c >> 4;
        cl = c & 15;
 
        delta_x = get_x(ch,delta_x,delta_n);
        int x_xcg = xcg_int(delta_x);
        delta_x2 = get_x(cl,delta_x,delta_n);
        int x2_xcg = xcg_int(delta_x2);

        //printf(" %04x %04x\n",x_xcg,x2_xcg);
        fwrite(&x_xcg, 2, 1, pp);
        fwrite(&x2_xcg, 2, 1, pp);
        
        delta_x = delta_x2;
        //num++;
    }
    fclose(pp);
    fclose(fp);

}