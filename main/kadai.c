#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

int main (void) {
    /*?? 変数の定義 */
    unsigned int a;
    unsigned int b;

    printf("a = ");
    scanf("%d",&a);
    printf("b = ");
    scanf("%d",&b);

    printf("a+b = %d\n", a+b); // 和

/*
    printf("a-b = %d\n", a-b); // 差
    printf("a*b = %d\n", a*b); // 積
    printf("a/b = %d\n", a/b); // 商
    printf("a%%b = %d\n", a%b); // 剰余

    return 0;
    */
}
