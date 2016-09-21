#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>

#include <gps.h>

int
main (void)
{
    int uart_fd = OpenGPSIface();
	unsigned char recv_message[256];
	unsigned char rx_buffer[1];
	int i, rx_len, id;
	/* header (2)	class (1) 	id (1) 	length (2)	payload (36) crc (2) */
	unsigned char CFG_NAV5[GPS_OVERHEAD + 36];
	unsigned char TEST_MSG[GPS_OVERHEAD];
	
	if (uart_fd == -1)
	{
	    printf("No GPS Device\n");
	    return 0;
	}

	while(1)
	{
		/* -1 means id failure */
		if (id = GPSReceiveMessage(uart_fd, recv_message, &rx_len), id != -1)
		{
			/* Process the message */

		}
	}
}
	
