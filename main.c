#include <msp430.h> 

void max_bit(int bit){
    // low pins 0 and 1
    P1OUT &= ~(BIT0 | BIT1);
    // set data (pin 0)
    P1OUT |= bit & BIT0;
    // clk (pin 1)
    P1OUT |= BIT1;
    // both pins off
    P1OUT &= ~(BIT0 | BIT1);
}
void max_byte(int byte, int reg){
    int x;
    // four blanks
    for(x = 4; x > 0; x--){
        max_bit(0);
    }
    // register address msb first
    for(x = 3; x > -1; x--){
        max_bit(reg >> x);
    }
    // data msb first
    for(x = 7; x > -1; x--){
        max_bit(byte >> x);
    }
    // latch data
    P1OUT |= BIT2;
    P1OUT &= ~BIT2;
}
void max_init(){
    // init pins
    P1DIR |= (BIT0 | BIT1 | BIT2);
    P1OUT &= ~(BIT0 | BIT1 | BIT2);
    max_byte(0x01, 0x0C); // shutdown off
    max_byte(0x07, 0x0B); // scan limit 8
    max_byte(0x00, 0x09); // decode mode off
    max_byte(0x0F, 0x0A); // full brightness
    // initally clear display
    int x;
    for(x = 1; x < 9; x++){
        max_byte(0x00, x);
    }
    // flash test
    max_byte(0x01, 0x0F);
    __delay_cycles(500000);
    max_byte(0x00, 0x0F);
}
void max_update(unsigned int data[]){
    unsigned int y;
    for(y = 1; y < 9; y++){
        int b = data[8 - y];
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        max_byte(b, y);
    }
}
void max_ind(int x, int y, int state){
    max_byte((state & 1) << (7 - x), 10 - y);
}
int analog_readx(){
    ADC10CTL0 = ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_3;
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    int ret = ADC10MEM >> 7;
    ADC10CTL0 &= ~ADC10ON;
    return ret;
}
int analog_ready(){
    ADC10CTL0 = ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = INCH_4;
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    int ret = 9 - (ADC10MEM >> 7);
    ADC10CTL0 &= ~ADC10ON;
    return ret;
}
void analog_init(){
    ADC10AE0 |= (BIT3 | BIT4);
}

int main(void){
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // screen init
    unsigned int scr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    max_init();

    // analog stick init
    analog_init();
    unsigned int analogx;
    unsigned int analogy;

    // buttons init
    P1DIR &= ~(BIT5 | BIT6);
    P1REN |= (BIT5 | BIT6);
    P1OUT |= (BIT5 | BIT6);

    while(1){
        analogx = analog_readx();
        analogy = analog_ready();

        if(!(P1IN & BIT5)){
            scr[analogy - 2] |= (1 << analogx);
        }
        if(!(P1IN & BIT6)){
            scr[analogy - 2] &= ~(1 << analogx);
        }
        max_update(scr);
        max_ind(analogx, analogy, 1);
    }
    return 0;
}
