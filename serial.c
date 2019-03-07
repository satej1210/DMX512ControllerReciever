// Serial Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"

#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))

#define GREEN_LED_MASK 8
#define RED_LED_MASK 2

#define delay4Cycles() __asm(" NOP\n NOP\n NOP\n NOP")

char str[40];
int strPos = 0;
int maxAddress = 512;
int mode = 1; //0-controller, 1-device
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Set GPIO ports to use APB (not needed since default configuration -- for clarity)
    // Note UART on port A must use APB
    SYSCTL_GPIOHBCTL_R = 0;

    // Enable GPIO port A and F peripherals
    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOA | SYSCTL_RCGC2_GPIOF;

    // Configure LED pins
    GPIO_PORTF_DIR_R = GREEN_LED_MASK | RED_LED_MASK;  // bits 1 and 3 are outputs
    GPIO_PORTF_DR2R_R = GREEN_LED_MASK | RED_LED_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTF_DEN_R = GREEN_LED_MASK | RED_LED_MASK;  // enable LEDs

    // Configure UART0 pins
    GPIO_PORTA_DIR_R |= 2;                           // enable output on UART0 TX pin: default, added for clarity
    GPIO_PORTA_DEN_R |= 3;                           // enable digital on UART0 pins: default, added for clarity
    GPIO_PORTA_AFSEL_R |= 3;                         // use peripheral to drive PA0, PA1: default, added for clarity
    GPIO_PORTA_PCTL_R &= 0xFFFFFF00;                 // set fields for PA0 and PA1 to zero
    GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA1_U0TX | GPIO_PCTL_PA0_U0RX;
                                                     // select UART0 to drive pins PA0 and PA1: default, added for clarity

    // Configure UART0 to 115200 baud, 8N1 format
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;         // turn-on UART0, leave other UARTs in same status
    delay4Cycles();                                  // wait 4 clock cycles
    UART0_CTL_R = 0;                                 // turn-off UART0 to allow safe programming
    UART0_CC_R = UART_CC_CS_SYSCLK;                  // use system clock (40 MHz)
    UART0_IBRD_R = 21;                               // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART0_FBRD_R = 45;                               // round(fract(r)*64)=45
    UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;
                                                     // enable TX, RX, and module
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);               // wait if uart0 tx fifo full
    UART0_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i;
    for (i = 0; i < strlen(str); i++)
      putcUart0(str[i]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    if (!(UART0_FR_R & UART_FR_RXFE))             // wait if uart0 rx fifo empty
        return UART0_DR_R & 0xFF;                        // get character from fifo
    else
        return '!';
}



void parseCommand(char* s){
    if(mode == 0){ //controller mode
        if(strcmp(s, "device") == 0){
                putsUart0("Device Mode\n");
                mode = 1;
        }
        else if(strcmp(s, "clear") == 0){
            putsUart0("Cleared\n");
        }
        else if(strstr(s, "set") != NULL){
            s += 4;
            char *token = strtok(s, ",");
            putsUart0("Setting \n");
            putsUart0("Address:");
            if(token == NULL){
                putsUart0("Invalid Command\n");
            }
            putsUart0(token);
            token = strtok(NULL, "\n");
            putsUart0("Value:");
            if(token == NULL){
                putsUart0("Invalid Command\n");
            }
            putsUart0(token);
        }
        else if(strstr(s, "get") != NULL){
            s += 4;
            char *token = strtok(s, "\n");
            if(token == NULL){
                putsUart0("Invalid Command\n");
            }
            putsUart0("Getting \n");
            putsUart0("Address:");
            putsUart0(token);
        }
        else if(strstr(s, "max") != NULL){
            s += 4;
            char *token = strtok(s, "\n");
            if(token == NULL){
                putsUart0("Invalid Command\n");
            }
            putsUart0("Setting max \n");
            putsUart0(token);
            maxAddress = atoi(token);
        }
        else if(strcmp(s, "on") == 0){
            putsUart0("continuous\n");
        }
        else if(strcmp(s, "off") == 0){
            putsUart0("continuous off\n");
        }

        else{
            putsUart0("Invalid Controller Mode Command\n");
        }
    }
    else if(mode == 1){
        if(strstr(s, "address") != NULL){
            s += 4;
            char *token = strtok(s, "\n");
            putsUart0("Device address is \n");
            putsUart0(token);
        }
        else if(strcmp(s, "controller") == 0){
            putsUart0("Controller Mode\n");
            mode = 0;
        }
        else{
            putsUart0("Invalid Device Mode Command\n");
        }
    }
    else{
        putsUart0("Invalid Mode\n");
    }
}


void clearStr(){
    int i = 0;
    for(;i < strPos + 1; ++i){
        str[i] = '\0';
    }
}



//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

uint8_t main(void)
{
    // Initialize hardware
    initHw();

    // Display greeting
    putsUart0("Command\r\n");
    putsUart0("Enter string followed by new line:\r\n");
    putcUart0('>');


    // For each received character, toggle the green LED
    // For each received "1", set the red LED
    // For each received "0", clear the red LED
    while(1)
    {
        char c = getcUart0();
        if(isLetter(c)){

            if(c == '\n' || strPos > 38){

                parseCommand(&str);
                clearStr();
                strPos = 0;
            }
            else{
                str[strPos++] = c;
            }
        }
    }
}
