#include <stdio.h>
#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"


/* ****************************************************************
 * JTAG keyboard interface Example
 * For use inside WIT Only
 *
 *
 ******************************************************************/
//declaring global variables and pointers
volatile int timeout;	// used to synchronize line updates on 250ms
extern volatile char byte1, byte2;								// intervals set by the intervalTimer_ISR
volatile int * HEX3_HEX0_ptr = (int *) 0xFF200020; //HEX address
volatile int * KEY_ptr = (int *) 0xFF200050;		// pushbutton KEY address								 
volatile int pixel_buffer_start = 0x08000000;// VGA pixel buffer
int resolution_x = 320; // VGA screen size
int resolution_y = 240; // VGA screen size
int hex_values[] = {(0b00111111),(0b00000110),(0b01011011),(0b01001111),(0b01100110),(0b01101101),(0b01111101),(0b00000111),(0b01111111),(0b01100111),((0b00000110 << 8) |(0b00111111)),((0b00000110 << 8) |(0b00000110))};
volatile int * JTAG_UART_ptr = (int *) JTAG_UART_BASE; // JTAG UART address
int data, i, rbound, bbound, lbound, ubound, hex_bits, collect, v, end;				

//function prototypes 
void VGA_text (int, int, char *);
void VGA_box (int, int, int, int, short);
void clear_screen(void);
void end_screen(void);      // winning screen 
void draw_hline(int, int, int, int, int);
void draw_vline(int, int, int, int, int);
void draw_box(int, int, int, int, int);
void plot_pixel(int, int, short int);
void put_jtag(volatile int *, char);		//used for keyboard movement
void clearscreen(void); 			// initial reset of screen
void field_gen1(void);				// Generating level 1 field
void coins(void);					// plot coins
int collection(int,int, int, int);  //identifies if the character is collecting a coin, displays coin count on HEX 
int check_game_end( int, int, int, int);
//function prototypes for boundaries
int right_bound( int, int);			
int left_bound( int, int);
int upper_bound( int, int);
int bottom_bound( int, int);
 


/* Draw a single character on the screen */

void drawcharacter(int x_char, int y_char, char mychar)
{
	volatile char* character_buffer = (char *) (0x09000000 + (y_char <<7) + x_char);
	*character_buffer = mychar;
}
// sets strings to displayed when you when 
char* fullstring1  = "You Have Won The Game!!!";
char* fullstring2 = "Continue moving around for fun!";

int main(void) 

{

clear_screen();
field_gen1();
coins();

draw_vline(305,201,305,320, 0xFFE0);					// finish line marker 

hex_bits = 0b00111111; 						//initializing the 7 segment display to 0
(*HEX3_HEX0_ptr) = hex_bits;
int ydir2 = 1;
int ydir1 =1;
int ydir = 1;

int win=0;

int color1; //RED
int color2; //BLACK

color2 = 0; //BLACK

// initial spot for user controlled object
int x1 = 25;
int x2 = x1 + 3;
int y1 = 40; 
int y2 = 43;

int color = 0x07E0;


		 

/* read and echo characters */
while(1)
{
	
	volatile int * interval_timer_ptr = (int *) 0xFF202000;	// interval timer base address
	volatile int * PS2_ptr = (int *) 0xFF200100;			// PS/2 keyboard port address

	color1=0x07E0;
	
	timeout = 0; //Start the program with the "update line position now" flag cleared

	int counter =( 0x00BEBC20 >> 3 );		// 1/(50 MHz) x (0xBEBC20) = 250 msec	
	//int counter = 0x02FAF080;		// 1/(50 MHz) x (0x02FAF080) = 1sec
	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
	*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;

	
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1 
	
    *(PS2_ptr) = 0xFF;		// reset
	*(PS2_ptr + 1) = 0x1; 	//write to the PS/2 Control register to enable interrupts

	NIOS2_WRITE_IENABLE( 0x01 );/* set interrupt mask for level 0 (interval timer)*/

	NIOS2_WRITE_STATUS( 1 );		// enable Nios II interrupts
	
	
	draw_box(x1,y1,x2,y2,color1);

	data = *(JTAG_UART_ptr); // read the JTAG_UART data register
	(*HEX3_HEX0_ptr) = hex_values[v];
	
	if ((*KEY_ptr))
	{
		draw_vline(305,200,305,320, 0x07FF);
		draw_box( 310, 230, 314, 234, 0x07E0);
	}
if (data & 0x00008000) // check RVALID to see if there is new data
{
	data = data & 0x000000FF; // the data is in the least significant byte
	
if (data == 119)
{ //if "w" key is pressed then object moves upwards
	color = 224;
	printf("\n move up ");
	ubound = upper_bound(x1,y2);			// funtion returns a 1 if at boundary moving up
	if(!ubound)							// only will enter loop to move if not at boundary
	{
	y1--;
	y2--;
	color2= 0;
	draw_box(x1,y1,x2,y2,color);			
	draw_box(x1,y1+1,x2,y2+1,color2);		  
	v = collection(x1,y1, x2, y2);				// checks position after you have made and updates global var. v. 
	end = check_game_end(x1,y1,x2,y2);			// checks if you have moved to the finishing position
	if (end)								//executes the end screen if you have made it to the end position
	{
		end_screen();
	}
	}
}
else if (data == 97)
{ // if "a" key is pressed object moves to the left 
	color = 224;
	printf("\n move left ");
	//color2 = 0;
	lbound = left_bound(x1,y2);
	if(!lbound)
	{
	x1--;
	x2--;
	draw_box(x1,y1,x2,y2,color);
	draw_box(x1+1,y1,x2+1,y2,color2);
	v = collection(x1,y1, x2, y2);
	end = check_game_end(x1,y1,x2,y2);
	if (end)
	{
		end_screen();
	}
	}
}
else if (data == 115)
{ // if "s" key is pressed then object moves downwards
	color =224;
	printf("\n move down ");
	//color2 = 0;
	bbound = bottom_bound(x1,y2);
	if(!bbound)
	{
	y1++;
	y2++;
	draw_box(x1,y1,x2,y2,color);
	draw_box(x1,y1-1,x2,y2-1,color2);
	v = collection(x1,y1, x2, y2);
	end = check_game_end(x1,y1,x2,y2);
	if (end)
	{
		end_screen();
	}
	}
}
else if (data == 100)
{ // if "d" key is pressed then object starts moving to the right
	printf("\n move right ");
	color1=224;
	rbound = right_bound(x2,y1);
	if(!rbound)
	{
	x1++;
	x2++;
	draw_box(x1,y1,x2,y2,color);
	draw_box(x1-1,y1,x2-1,y2,color2);
	v = collection(x1,y1, x2, y2);
	end = check_game_end(x1,y1,x2,y2);
	if (end)
	{
		end_screen();
	}
	}

} /* Close main()

/* echo the character */
put_jtag (JTAG_UART_ptr, (char) data & 0xFF );
}
}
}








/* Function to blank the VGA screen */
void clear_screen( )
{
	int y, x;
	int pixel_ptr;

	for (y = 0; y < resolution_y; y++)
	{
		for (x = 0; x < resolution_x; x++)
		{
			plot_pixel(x, y, 0x0);
			volatile char * pixel_ptr = (volatile char *) (pixel_buffer_start + (y << 10) + (x << 1));
			(pixel_ptr) = 0;	// clear pixel- background color is set to black
			pixel_ptr += 1;		// Next		
		}
	}
	volatile	int x_CharStart = 0;
	volatile	int x_CharEnd = 79;
	volatile	int y_CharStart = 0;
	volatile	int y_CharEnd = 59;
	volatile	int y_Line;
	volatile	int x_Char;

	for (y_Line = y_CharStart; y_Line <= y_CharEnd; y_Line++)
		{
			for(x_Char = x_CharStart; x_Char <= x_CharEnd; x_Char++)			
			{
				drawcharacter(x_Char, y_Line,' ' );
			}
		}		
}




/* Line drawing or Box shading algorithm.  This function draws a line between points (x0, y0)
 * and (x1, y1). One ROW at a time. The function assumes that the coordinates are valid
 * 
 */
 
void draw_box(int x0, int y0, int x1, int y1, int color)
{
	int y;
	int x;
	
	for(y=y0;y<y1;y++)
	{
		for(x=x0;x<x1;x++)
		{
			plot_pixel(x,y,color);
		}
	}
}

/* Draw Horizontal Line */
void draw_hline(int x0, int y0, int x1, int y1, int color)
{
	int y=y0;
	int x;
	
	for(x=x0;x<x1;x++)
	{
		plot_pixel(x,y,color);
	}
}
/* Vertical Line */
void draw_vline(int x0, int y0, int x1, int y1, int color)
{
	int x=x0;
	int y;
	
	for(y=y0;y<=y1;y++)
	{
		plot_pixel(x,y,color);
	}
}

void plot_pixel(int x, int y, short int line_color)
{
	//*(volatile char *)(pixel_buffer_start + (y << 7)+x) = line_color;
	*(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color; //For DE0-CV
}

void put_jtag( volatile int * JTAG_UART_ptr, char c )
{
int control;
control = *(JTAG_UART_ptr + 1); // read the JTAG_UART control register
if (control & 0xFFFF0000) // if space, then echo character, else ignore 
*(JTAG_UART_ptr) = c;
}


/****************************************************************************************
 * Draw a filled rectangle on the VGA monitor 
****************************************************************************************/
void VGA_box(int x1, int y1, int x2, int y2, short pixel_color)
{
	int offset, row, col;
  	volatile char * pixel_buffer = (char *) 0x08000000;	// VGA pixel buffer

	/* assume that the box coordinates are valid */
	for (row = y1; row <= y2; row++)
	{
		col = x1;
		while (col <= x2)
		{
			offset = (row << 7) + col;
			*(pixel_buffer + offset) = pixel_color;	
			++col;
		}
	}
}






/****************************************************************************************
 * Subroutine to send a string of text to the VGA monitor 
****************************************************************************************/
void VGA_text(int x, int y, char * text_ptr)
{
	int offset;
  	volatile char * character_buffer = (char *) 0x09000000;	// VGA character buffer

	/* assume that the text string fits on one line */
	offset = (y << 7) + x;
	while ( *(text_ptr) )
	{
		*(character_buffer + offset) = *(text_ptr);	// write to the character buffer
		++text_ptr;
		++offset;
	}
}

void field_gen1( void)					// function definition of field level 1
{
	int y0, y1, y2, y3, y4, y5, y6, ybottom, red;    	// local variables defining boundary
	y0 = 0;
	y1 = 220;
	y2 = 110;
	y3 = 230;
	y4 = 150;
	y5 = 100;
	y6 = 50;
	ybottom = 240;
	red = 0xF800;
	
	// for loops draw rectangle boundaries that were predetermined 
	for (y0; y0<=y1; y0++)			
	{
		draw_hline(40,y0,80,y0, red);
	}
	for (y0 =0; y0<=y2; y0++)
	{
		draw_hline(80,y0,120,y0, red);
	}
	for (y0 = 0; y0<=y3; y0++)
	{
				draw_hline(120,y0,140,y0, red);
	}
	for (y4; y4<=y3; y4++)
	{
		draw_hline(140,y4,160,y4, red);
	}
	for (y6 = 50; y6<=y5; y6++)
	{
		draw_hline(160,y6,200,y6, red);
	}
	for (y6 = 50; y6<=ybottom; y6++)
	{
		draw_hline(200,y6,240,y6, red);
	}
	for (y0 = 0; y0<=200; y0++)
	{
		draw_hline(300,y0,320,y0, red);
	}
	
}
int right_bound (int x, int y)						//function defintion restricts movement to the right
{
	int z = 0;							// sets z to 0 when function is called
	
	// sets z to 1 if you are at a boundary 
	if (( x == 40) && ((y >= 0) && (y <= 220)))
	{
		z = 1;
	}
	if (( x == 120) && ((y >= 0) && (y <= 230)))
	{
		z = 1;
	}
	if (( x == 160) && ((y >= 50) && (y <= 100)))
	{
		z = 1;
	}
	if (( x == 200) && ((y >= 100) && (y <= 240)))
	{
		z = 1;
	}
	if (( x == 300) && ((y >= 0) && (y <= 200)))
	{
		z = 1;
	}
	return z;
}
int left_bound (int x, int y)						// function definition that restricts movement the left
{
	int z = 0;				// sets z to 0 when function is called
	
	// sets z to 1 if you are at a boundary 
	if (( x == 0) && ((y >= 0) && (y <= 240)))
	{
		z = 1;
	}
	if (( x == 80) && ((y >= 110) && (y <= 223)))
	{
		z = 1;
	}
	if (( x == 140) && ((y >= 0) && (y <= 153)))
	{
		z = 1;
	}
	if (( x == 160) && ((y >= 151) && (y <= 233)))
	{
		z = 1;
	}
	if (( x == 240) && ((y >= 51) && (y <= 240)))
	{
		z = 1;
	}
	return z;        // returns the value of z: 0 not at boundary or 1 at boundary
}

int upper_bound (int x, int y)		
{
	int z = 0;					// sets z to 0 when function is called
	
	// sets z to 1 if you are at a boundary 
	if (( y == 4) && ((x >= 0) && (x <= 320)))
	{
		z = 1;
	}
	if (( y == 224) && ((x >= 38) && (x <= 79)))
	{
		z = 1;
	}
	if (( y == 114) && ((x >= 78) && (x <= 120)))
	{
		z = 1;
	}
	if (( y == 104) && ((x >= 158) && (x <= 200)))
	{
		z = 1;
	}
	if (( y == 234) && ((x >= 118) && (x <= 159)))
	{
		z = 1;
	}
	if (( y == 204) && ((x >= 298) && (x <= 320)))
	{
		z = 1;
	}
	
	return z;
}
int bottom_bound (int x, int y)		
{
	int z = 0;					// sets z to 0 when function is called
	
	// sets z to 1 if you are at a boundary 
	if (( y == 240) && ((x >= 0) && (x <= 320)))
	{
		z = 1;
	}
	
	if (( y == 150) && ((x >= 140) && (x <= 159)))
	{
		z = 1;
	}
	if (( y == 50) && ((x >= 158) && (x <= 239)))
	{
		z = 1;
	}
	
	return z;
}
void coins(void)			// draws coins that are to collected
{
	plot_pixel(20, 50,0x07FF);
	plot_pixel(25, 150, 0x07FF);
	plot_pixel(100, 125, 0x07FF);
	plot_pixel(110, 130, 0x07FF);
	plot_pixel(115, 225, 0x07FF);
	plot_pixel(180, 220, 0x07FF);
	plot_pixel(150, 25, 0x07FF);
	plot_pixel(280, 30, 0x07FF);
	plot_pixel(280, 50, 0x07FF);
	plot_pixel(280, 70, 0x07FF);
	plot_pixel(280, 90, 0x07FF);
}


int collection(int x,int y, int x1, int y1)
{

	static int count = 0;  // static used to initially set seven segment to 0 but to remember the count of coins you have as you enter and exit function
	// checks if you are at coin position.. if yes then increments variable count 
	if(((x<=20) &&(x1>=20)) && ((y <= 50) && (y1 >= 50)))
	{
		count = 1;
	}	
	if(((x<=25) &&(x1>=25)) && ((y <= 150) && (y1 >= 150)))
	{
		count = 2;
	}
	if(((x<=100) &&(x1>=100)) && ((y <= 125) && (y1 >= 125)))
	{
		count = 3;
	}	
	if(((x<=110) &&(x1>=110)) && ((y <= 130) && (y1 >= 130)))
	{
		count = 4;
	}	
	if(((x<=115) &&(x1>=115)) && ((y <= 225) && (y1 >= 225)))
	{
		count = 5;
	}	
	if(((x<=180) &&(x1>=180)) && ((y <= 220) && (y1 >= 220)))
	{
		count = 6;
	}	
	if(((x<=150) &&(x1>=150)) && ((y <= 25) && (y1 >= 25)))
	{
		count = 7;
	}	
	if(((x<=280) &&(x1>=280)) && ((y <= 30) && (y1 >= 30)))
	{
		count = 8;
	}		
	if(((x<=280) &&(x1>=280)) && ((y <= 50) && (y1 >= 50)))
	{
		count = 9;
	}
	if(((x<=280) &&(x1>=280)) && ((y <= 70) && (y1 >= 70)))
	{
		count = 10;
	}
	if(((x<=280) &&(x1>=280)) && ((y <= 90) && (y1 >= 90)))
	{
		count = 11;
	}
	return count; 		// returns count which is used to display on the seven segent display
}


 void end_screen(void)			//function definition for when you get to winning position
{
	int y, x, c, x_start, y_start;
	int pixel_ptr;
	
	int colorful[] = {0xF180, 0x0FF0, 0xF800, 0x0EEF, 0x007F};    // array of colors
		
	for (y = 0; y < resolution_y; y++)
	{
		for(c =0; c<=4; c++)			//cycles through colors as it draws end screen
		{
		draw_hline( 0, y, 320, y, colorful[c]); 
		while(!timeout)					//uses interval timer to determine the speed it draws.
		{};
	timeout = 0;
		}
	c = 0;
	}
	
	x_start = 28;
	y_start = 5;
	
	while (*fullstring1)
	{
		drawcharacter(x_start, y_start, *fullstring1);
		x_start = x_start + 1;
		fullstring1 = fullstring1 + 1;
	}
	
	/* Write text to the monitor beginning at (col=x_start, row=y_start)   fullstring2*/
	
	x_start = 28;
	y_start = 10;
	
	while (*fullstring2)
	{
		drawcharacter(x_start, y_start, *fullstring2);
		x_start = x_start + 1;
		fullstring2 = fullstring2 + 1;
	}
			
		
	

	 


}

int check_game_end( int x, int y, int x1, int y1)			// function used to determine if you are at the wining position
{
	 int end = 0;
	if(((x==310) &&(x1==313)) && ((y == 230) && (y1 == 233)))
	{
		end = 1;
	}	
		
	return end;
}

