extern volatile int timeout;
#include <stdio.h>

void interval_timer_ISR( )
{
	volatile int * interval_timer_ptr = (int *) 0xFF202000;
	
	* (interval_timer_ptr) = 0;
	timeout = 1;
	
	return;
}
