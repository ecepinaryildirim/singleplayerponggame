#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "address_map_arm.h"
#include <math.h>
//new branch
// VGA, keys, and LEDs

#define SWAP(X,Y) do{int temp=X; X=Y; Y=temp;}while(0) 
#define FPGA_CHAR_BASE        0xC9000000 
/* Cyclone V FPGA devices */
#define HW_REGS_BASE          0xff200000
#define HW_REGS_SPAN        0x00200000 
//#define HW_REGS_SPAN          0x00005000 
#define FPGA_ONCHIP_BASE  0xC8000000

/* function prototypes */

void VGA_box (int, int, int, int, short);
void VGA_disc(double, double, int, short) ;

// virtual to real address pointers

volatile unsigned int * red_LED_ptr = NULL ;
void *h2p_lw_virtual_base;

volatile unsigned int * vga_pixel_ptr = NULL ;
void *vga_pixel_virtual_base;

volatile unsigned int * vga_char_ptr = NULL ;
void *vga_char_virtual_base;

#define red  0xff0000
#define dark_red (0+(0<<5)+(15<<11))
#define green (0+(63<<5)+(0<<11))
#define dark_green (0+(31<<5)+(0<<11))
#define blue (31+(0<<5)+(0<<11))
#define dark_blue (15+(0<<5)+(0<<11))
#define yellow (0+(63<<5)+(31<<11))
#define cyan (31+(63<<5)+(0<<11))
#define magenta (31+(0<<5)+(31<<11))
#define black (0x0000)
#define gray (15+(31<<5)+(51<<11))
#define white (0xffff)
int colors[] = {red, dark_red, green, dark_green, blue, dark_blue, 
yellow, cyan, magenta, gray, black, white};
volatile int * ADC_ptr;
int fd;

double x1, x2, sayma;

// pixel macro
#define VGA_PIXEL(x,y,color) do{\
char  *pixel_ptr ;\
pixel_ptr = (char *)vga_pixel_ptr + ((y)<<10) + (x) ;\
*(char *)pixel_ptr = (color);\
} while(0)

#define VIDEO_IN_PIXEL(x,y,color) do{\
char  *pixel_ptr ;\
pixel_ptr = (char *)video_in_ptr + ((y)<<9) + (x) ;\
*(char *)pixel_ptr = (color);\
} while(0)


// measure time
struct timeval t1, t2;
double elapsedTime;

int main(void)
{
volatile int * LEDR_ptr; 
// Declare volatile pointers to I/O registers (volatile // means that IO load and store instructions will be used // to access these pointer locations, 
// instead of regular memory loads and stores) 
    

// === get FPGA addresses ==================
// Open /dev/mem
if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
printf( "ERROR: could not open \"/dev/mem\"...\n" );
return( 1 );
}
    
// get virtual addr that maps to physical
h2p_lw_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE ); 
if( h2p_lw_virtual_base == MAP_FAILED ) {
printf( "ERROR: mmap1() failed...\n" );
close( fd );
return(1);
}
    
// Get the address that maps to the FPGA LED control 
red_LED_ptr =(unsigned int *)(h2p_lw_virtual_base +     LEDR_BASE);
ADC_ptr = (unsigned int *) (h2p_lw_virtual_base + ADC_BASE);
*(ADC_ptr) = 0;
*(ADC_ptr + 1) = 0x01;
volatile int data = *(ADC_ptr + 2);
LEDR_ptr = (unsigned int *) (h2p_lw_virtual_base + LEDR_BASE);


// === get VGA char addr =====================
// get virtual addr that maps to physical
vga_char_virtual_base = mmap( NULL, FPGA_CHAR_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_CHAR_BASE );   
if( vga_char_virtual_base == MAP_FAILED ) {
printf( "ERROR: mmap2() failed...\n" );
close( fd );
return(1);
}
    
// Get the address that maps to the FPGA LED control 
vga_char_ptr =(unsigned int *)(vga_char_virtual_base);

// === get VGA pixel addr ====================
// get virtual addr that maps to physical
vga_pixel_virtual_base = mmap( NULL, FPGA_ONCHIP_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_ONCHIP_BASE);   
if( vga_pixel_virtual_base == MAP_FAILED ) {
printf( "ERROR: mmap3() failed...\n" );
close( fd );
return(1);
}
    
// Get the address that maps to the FPGA LED control 
vga_pixel_ptr =(unsigned int *)(vga_pixel_virtual_base);

// ===========================================


// clear the screen
VGA_box (0, 0, 319, 239, 0);

double a=0;
double xd1, yd1; // center coordinates of the disc (ball)

xd1=320.0; // initial position of the ball
yd1=120.0;

double vx = 0.3; // velocity of the ball in x and y axis
double vy = 0.3;


*(LEDR_ptr) = 0x200; // lighting up the LED at the very left to indicate that game is about to start in 5 seconds.
struct timespec tim, tim2;
tim.tv_sec=5; 
tim.tv_nsec=500;
nanosleep(&tim,&tim2); // setting a delay

while(1) {

    *(LEDR_ptr) = 0; // clearing the LEDs

    VGA_disc(xd1,yd1,5,0xffe0); // drawing a disc to show the ball


    if(abs(xd1+vx) > 639.0) vx = -vx; // if the ball is at the end of x axis, perform an elastic collusion
    if(abs(yd1+vy) >=230.0 && (xd1/2 <= x2 && xd1/2 >= x1 )) vy = -vy; // if the ball is at the end of y axis and hits the bar, perform an elastic collusion
    if(abs(yd1+vy) >230.0 && xd1/2>x2){ // if the ball doesn't meet with the bar reset the ball coordinates and start over
	  nanosleep(&tim,&tim2);
        xd1=320.0;
        yd1=120.0;
    }
    if(abs(yd1+vy) >230.0 &&  xd1/2<x1){ // if the ball doesn't meet with the bar reset the ball coordinates and start over
	  nanosleep(&tim,&tim2);
        xd1=320.0;
        yd1=120.0;
    }
    if(abs(yd1+vy) < 5.0 ) vy = -vy; // if the ball is at the end of y axis, perform an elastic collusion
    if(abs(xd1+vx) < 5.0) vx = -vx; // if the ball is at the end of x axis, perform an elastic collusion

    xd1 = xd1 + vx; // always add the velocity to the ball coordinates
    yd1 = yd1 + vy;


    data = *(ADC_ptr + 2); // take the potentiometer data from ADC Channel 2.
    
    if(data < 7000.0){ // if the data is in the boundary that we decide perform the bar drawing.
        VGA_box (0, 0, 319, 239, 0); // clear the screen in every while loop.
    
        x2 = x1 + 60.0; // define the 2 x axis boundaries of the bar. (The bar has its length which is 60)
    
        if(data < 12.0) { // generalizing the first 12 data.
            x1 = 0.0;}
    
        else if (data >= 3120.0) { // clearing the noisy data (there were super noise while we were trying to hold the connections with bare hand, after, we connected it to a breadboard)
            x1 = 260.0;
        }
    
        else  // lowering data to fit it to the VGA screen (turning the data to coordinates)
            x1 = data / 12.0;
    
        VGA_box ((int)x1, 235, (int)x2, 240, 0xffe0); // drawing the Bottom bar
        *(ADC_ptr + 1) = 0x01; // giving the stated register 1 to update the data automatically.
    }

}


short color ;

}

/****************************************************************************************
 * Draw a filled rectangle on the VGA monitor 
****************************************************************************************/
void VGA_box(int x1, int y1, int x2, int y2, short pixel_color)
{
int *pixel_ptr, row, col;

/* assume that the box coordinates are valid */
for (row = y1; row <= y2; row++)
for (col = x1; col <= x2; ++col)
{
pixel_ptr = (char *)vga_pixel_ptr + (row << 10)    + (col << 1); 
// set pixel color
*(short *)pixel_ptr = pixel_color;  
}
}


/****************************************************************************************
 * Draw a filled circle on the VGA monitor 
****************************************************************************************/

void VGA_disc(double x, double y, int r, short pixel_color)
{
char  *pixel_ptr ; 
int row, col, rsqr, xc, yc;

rsqr = r*r;

for (yc = -r; yc <= r; yc++)
for (xc = -r; xc <= r; xc++)
{
col = xc;
row = yc;
// add the r to make the edge smoother
if(col*col+row*row <= rsqr+r){
col += x; // add the center point
row += y; // add the center point
//check for valid 640x480
if (col>639) col = 639;
if (row>479) row = 479;
if (col<0) col = 0;
if (row<0) row = 0;
pixel_ptr = (char *)vga_pixel_ptr + (row<<10) + col ;
// set pixel color
*(char *)pixel_ptr = pixel_color;
//VGA_PIXEL(col,row,pixel_color);
}
}
}
