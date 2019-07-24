#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "address_map_arm.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "confirmationSound.c"
//#include "image.s"

#define PI 3.14159265
#define e  2.71828
#define NUM_ELEMENTS 67060

volatile int pixel_buffer_start; // global variable
extern short MYIMAGE [240][320];
bool add = false, subtract = false, multiply = false;

int characters[24][3] = {
    {0x45, '0',0x3F},
	{0x16, '1',0x06},
	{0x1E, '2',0x5B},
	{0x26, '3',0x4F},
	{0x25, '4',0x66},
	{0x2E, '5',0x6D},
	{0x36, '6',0x7D},
	{0x3d, '7',0x07},
	{0x3E, '8',0x7F},
	{0x46, '9',0x67},
	{0x22, 'x',0x52},
	{0x4A, '/',0x00},
	{0x79, '+',0x73},
	{0x4E, '-',0x40},
	{0x5A, 'r',0x00}, // Enter key
	{0x7C, '*',0x00},
    {0x1B, 's',0x6D},
    {0x76, '~',0x00}, // Escape key
    {0x29, ' ',0x00}, // Space key
    {0x21, 'c',0x39},
    {0x24, 'e',0x79},
    {0x43, 'i',0x30},
    {0x31, 'n',0x54},
    {0x44, 'o',0x5C}
};


void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}


void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

// code not shown for clear_screen() and draw_line() subroutines
void draw_line(int x0, int y0, int x1, int y1, short int line_color) {

    bool is_steep = abs(y1 - y0) > abs(x1 - x0);

    if(is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if(x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax/2);
    int y = y0;

    int y_step;
    if (y0 < y1) {
        y_step = 1;
    } else {
        y_step = -1;
    }

    int x;
    for (x = x0; x < x1; x++) {
        if(is_steep) {
            plot_pixel(y,x, line_color);
        } else {
            plot_pixel(x,y, line_color);
        }
        error = error + deltay;

        if(error >= 0) {
            y = y + y_step;
            error = error - deltax;
        }

    }
}


void clear_screen() {
    int x;
    for(x = 0; x < 320; x++) {
        int y;
        for(y = 0; y < 240; y++) {
            plot_pixel(x, y, 0xffff);
        }
    }
}

void background(){
    clear_screen();
    
    
    draw_line(0, 120, 319, 120, 0x0000);   // this line is blue
    draw_line(160, 0, 160, 239, 0x0000); // this line is green
    int x,y;
    for(x=0;x<320;x=x+16){
        draw_line(x,0,x,239,0x3432);
    }
    for(y=0;y<240;y=y+12){
        draw_line(0,y,319,y,0x1111);
    }
    draw_line(0,239,319,239,0x0000);
    draw_line(319,0,319,239,0x3432);
    draw_line(0, 120, 319, 120, 0x0000);   // this line is blue
    draw_line(160, 0, 160, 239, 0x0000); // this line is green
}

void plotsin(int *xValues, int *yValues, bool cosPlot) {
    int x;
    for(x = 0; x < 320; x++) {
        double xin = (x-160.0)/16.0;
        xValues[x] = x;
        if(cosPlot) {
            if(add) yValues[x] += round(12*cos(xin));
            else if(subtract) yValues[x] -= round(12*cos(xin));
        } else {
            if(add) yValues[x] += round(12*sin(xin));
            else if(subtract) yValues[x] -= round(12*sin(xin));
        }
        
    }
}

void plote(int *xValues, int *yValues){
    int x;
    for(x = 0; x < 320; x++) {
        double xin = (x-160.0)/16.0;
        xValues[x] = x;
        if(add) yValues[x] += 12*exp(xin);
        else if(subtract) yValues[x] -= 12*exp(xin);
    }
}

void plotx(int power, int shiftx, int shifty, int *prevX, int *prevY){
    int x,y=0;
    int count = 120;
    
    int counter = 0;
    
    for(x=0; x<320; x++) {
        double xin = (x-160.0)/16.0;
        
        int y = 12*pow(xin, power);
        
         int plotx = (x)+shiftx*16;
        
            prevX[x] = round(plotx);
            if(add) prevY[x] += y;
            else if(subtract) prevY[x] -= y;
        
    }
}

void plotconstant(int* xValues, int* yValues, int constant) {
    int x;
    for(x = 0; x < 320; x++) {
        xValues[x] = x;
        if(add) yValues[x] -= -12*constant;
        else if(subtract) yValues[x] += -12*constant;
    }
    
}

void delay(int number_of_seconds) {
    volatile int * MPcore_private_timer_ptr = (int *)MPCORE_PRIV_TIMER;
    int counter = 2000000; // timeout = 1/(200 MHz) x 200x10^6 = 1 sec

    *(MPcore_private_timer_ptr) = counter;
    *(MPcore_private_timer_ptr + 2) = 0b001;

    while (*(MPcore_private_timer_ptr + 3) == 0);
    *(MPcore_private_timer_ptr + 3) = 1;
    
}

void audio(){
    unsigned int fifospace;
    volatile int * audio_ptr = (int *) 0xFF203040; // audio port
    int count = 0;
    while (1)
    {
        fifospace = *(audio_ptr+1);
        if ((fifospace & 0x00FF0000) > 0 && (fifospace & 0xFF000000) > 0) {
            int sample = 500000*data[count];	// read right channel only
            *(audio_ptr + 2) = sample;		// Write to both channels
            *(audio_ptr + 3) = sample;
	    if (count == NUM_ELEMENTS)
            	break;
	    else
	        count++;
        }

    }
}

void drawFunction(int *xValues, int *yValues) {
    //background();
    audio();
    int i;
    for(i = 0; i < 319; i++) {
        int y = 120 - yValues[i];
        int yNext = 120 - yValues[i+1];
        if(y > 0 && y < 240 && xValues[i] > 0 && xValues[i] < 320) {
            if(yNext > 0 && yNext < 240 && xValues[i+1] > 0 && xValues[i+1] < 320) {
                draw_line(xValues[i], y, xValues[i+1], yNext ,0x0000);
                delay(1);
            }
        }
    }
	
}

void check_KEYs (int * option) {
    volatile int * KEY_ptr = (int *)KEY_BASE;
    int KEY_value;
    
    KEY_value = *(KEY_ptr);
    
    if (KEY_value == 0x1)  {
        *option = 1;
    } else if (KEY_value == 0x2) {
        *option = 2;
    } else if(KEY_value == 0x4) {
        *option = 3;
    } else {
        *option = 0;
    }
}


void load_screen (){
   volatile short * pixelbuf = 0xc8000000;
   int i, j;
   for (i=0; i<240; i++)
   for (j=0; j<320; j++)
   *(pixelbuf + (j<<0) + (i<<9)) = MYIMAGE[i][j];
}

void displayOnHEX(char * eqn) {
    volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
    volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;
    
    int sizeOfEqn = strlen(eqn);
    int i;
    char display[6];
    
    unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    for(i = 0; i < sizeOfEqn && i < 6; i++) {
        display[i] = eqn[sizeOfEqn - 1 - i];
    }
    
    
    for(i = 0; i < sizeof(display)/sizeof(char); i++) {
        int j;
        for(j = 0; j < sizeof(characters)/sizeof(int); j++) {
            if(display[i] == characters[j][1]) {
                hex_segs[i] = characters[j][2];
            }
        }
    }
    *(HEX3_HEX0_ptr) = *(int *)(hex_segs);
    *(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);
}

char HEX_PS2(char b1, char b2, char b3){
	int j;
	char returnedChar;
	for (j = 0; j < sizeof(characters)/sizeof(char); j++) {
		if(characters[j][0] == b3) {
			returnedChar = characters[j][1];
		}
	}
    return returnedChar;
}

void append(char* s, char c)
{
    int len = strlen(s);
    s[len] = c;
    s[len+1] = '\0';
}



int main(void){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;
    //audio();
    int option;
    while(true) {
        check_KEYs(&option);
        load_screen();
        if(option != 0) break;
    }
    //clear_screen();
    background();
	
    volatile int * PS2_ptr = (int *)PS2_BASE;
    int PS2_data, RVALID;
    
    int holdPower = -1, holdXShift = -1, holdYShift = -1;
    
    char eqn[512];
    int xValues[320] = {0};
    int yValues[320] = {0};
    
    while(true) {
        PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
        RVALID = PS2_data & 0x8000; // extract the RVALID field
        char byte1;
        byte1 = PS2_data & 0xFF;
        char returnedChar = '0';
        displayOnHEX(eqn);
        
        int a;
        for(a = 0; a < 320; a++) {
            xValues[a] = 0;
            yValues[a] = 0;
        }
        
        while(returnedChar != 'r') {
            PS2_data = *(PS2_ptr);
            byte1 = PS2_data & 0xFF;
            RVALID = PS2_data & 0x8000;
            if(RVALID) {
                byte1 = PS2_data & 0xFF;
                
                returnedChar = HEX_PS2(0,0,byte1);
                
                if(returnedChar == '~') {
                    int i;
                    for(i =0; i < 320; i++) {
                        yValues[i] = 0;
                    }
                    clear_screen();
                    background();
                    break;
                } else if(returnedChar != 'r' && returnedChar != eqn[strlen(eqn)-1]) {
                    append(eqn, returnedChar);
                    displayOnHEX(eqn);
                }
                
                while(RVALID) {
                    PS2_data = *(PS2_ptr);
                    RVALID = PS2_data & 0x8000;
                }
            }
        }
		
        returnedChar = '0';
        
        // Find the length of the character array
        int charLength = 0;
        while(eqn[charLength] != '\0') {
            charLength++;
        }
        
        bool reachedEnd = false;
        int startingPoint = 0;
        char* expression;
        expression = strtok(eqn, " ");
        
        while(true) {
            if(expression == NULL) {
                break;
            } else if(expression[0] == '+') {
                add = true, subtract = false, multiply = false;
            } else if(expression[0] == '-') {
                add = false, subtract = true, multiply = false;
            } else if(expression[0] == '*') {
                add = false, subtract = false, multiply = true;
            } else {
                add = true, subtract = false, multiply = false;
            }
            
            
            int i;
            for(i = 0; i < strlen(expression); i++) {
                if(expression[i] == 'x') {
                    int number;
                    if(strlen(expression) == i+1) {
                        number = 1;
                    } else {
                        number = expression[i+1]-'0';
                    }
                    plotx(number, 0, 0, xValues, yValues);
                    i++;
                } else if(expression[i] == 's') {
                    plotsin(xValues, yValues, false);
                    break;
                } else if(expression[i] == 'c') {
                    plotsin(xValues, yValues, true);
                    break;
                } else if(expression[i] == 'e') {
                    plote(xValues, yValues);
                    break;
                } else if(expression[i] >= '0' && expression[i] <= '9') {
                    plotconstant(xValues, yValues, expression[i] - '0');
                }
            }
            expression = strtok(NULL, " ");
        }
        drawFunction(xValues, yValues);
        
        int charEmpty = 0;
        for(charEmpty = 0; charEmpty < 512; charEmpty++) {
            eqn[charEmpty] = '\0';
        }
    }

    return 0;
}

