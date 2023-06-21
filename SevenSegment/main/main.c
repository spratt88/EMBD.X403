#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define NUM_MAX 9
#define SEG_A GPIO_NUM_9
#define SEG_B GPIO_NUM_10
#define SEG_C GPIO_NUM_45
#define SEG_D GPIO_NUM_48
#define SEG_E GPIO_NUM_47
#define SEG_F GPIO_NUM_11
#define SEG_G GPIO_NUM_12
#define PIN_SWITCH GPIO_NUM_13
#define PIN_LED GPIO_NUM_2

/*
#define SS_ZERO     0b0111111
#define SS_ONE      0b0000110
#define SS_TWO      0b1011011
#define SS_THREE    0b1001111
#define SS_FOUR     0b1100110
#define SS_FIVE     0b1101101
#define SS_SIX      0b1011111
#define SS_SEVEN    0b0000111
#define SS_EIGHT    0b1111111
#define SS_NINE     0b1101111
#define SS_BAD      0
*/

#define SS_ZERO     0b1000000
#define SS_ONE      0b1111001
#define SS_TWO      0b0100100
#define SS_THREE    0b0110000
#define SS_FOUR     0b0011001
#define SS_FIVE     0b0010010
#define SS_SIX      0b0000010
#define SS_SEVEN    0b1111000
#define SS_EIGHT    0b0000000
#define SS_NINE     0b0010000
#define SS_BAD      0b1111111

int mode = 0;

static void IRAM_ATTR gpio_interrupt_handler(void *args){
    if ((mode >= 0) && (mode < 9))
        mode += 1;
    else if (mode >= 9)
        mode = 0;
    else if (mode < 0)
        mode = 0; 
}

/* See if bit position of provided number is a 1 or 0 and return value */
int getBitStatus (int num, int pos) {
    return ((num >> pos) & 1);
}

/* Set LEDs of 7-segment display */
void sevenSegmentDisplay(int numCode){
    gpio_set_level(SEG_A, getBitStatus(numCode, 0));
    gpio_set_level(SEG_B, getBitStatus(numCode, 1));
    gpio_set_level(SEG_C, getBitStatus(numCode, 2));
    gpio_set_level(SEG_D, getBitStatus(numCode, 3));
    gpio_set_level(SEG_E, getBitStatus(numCode, 4));
    gpio_set_level(SEG_F, getBitStatus(numCode, 5));
    gpio_set_level(SEG_G, getBitStatus(numCode, 6));
}

/* Function for displaying numbers on 7-segment display */
void displayNum () {
   
    switch (mode)
    {
    case 0 : /* display a 0*/
        sevenSegmentDisplay(SS_ZERO);
        break;
    case 1 : /* display a 1*/
        sevenSegmentDisplay(SS_ONE);
        break;
    case 2 : /* display a 2*/
        sevenSegmentDisplay(SS_TWO);
        break;
    case 3 : /* display a 3*/
        sevenSegmentDisplay(SS_THREE);
        break;
    case 4 : /* display a 4*/
        sevenSegmentDisplay(SS_FOUR);
        break;
    case 5 : /* display a 5*/
        sevenSegmentDisplay(SS_FIVE);
        break;
    case 6 : /* display a 6*/
        sevenSegmentDisplay(SS_SIX);
        break;
    case 7 : /* display a 7*/
        sevenSegmentDisplay(SS_SEVEN);
        break;
    case 8 : /* display a 8*/
        sevenSegmentDisplay(SS_EIGHT);
        break;
    case 9 : /* display a 9*/
        sevenSegmentDisplay(SS_NINE);
        break;
    default:
        sevenSegmentDisplay(SS_BAD);
        break;
    }

}

void app_main(void)
{
    /* Set output modes */
    gpio_set_direction(SEG_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEG_G, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);


    /* Set switch to pull-down */
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);

    /* Set ISR for rising edge */
    gpio_set_intr_type(PIN_SWITCH, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_SWITCH, gpio_interrupt_handler, (void *)PIN_SWITCH);

    /* main app */
    while(true) {
         
        displayNum();
        
//        if ((mode >= 0) && (mode < 9))
//            mode += 1;
//        else if (mode >= 9)
//            mode = 0;
//        else if (mode < 0)
//            mode = 0;     

        /* TaskDelay */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
