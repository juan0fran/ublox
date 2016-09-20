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
	unsigned char rx_buffer[256];
	int i, rx_len;
	/* header (2)	class (1) 	id (1) 	length (2)	payload (36) crc (2) */
	unsigned char CFG_NAV5[GPS_OVERHEAD + 36];
	unsigned char TEST_MSG[GPS_OVERHEAD];
	
	if (uart_fd == -1)
	{
	    printf("No GPS Device\n");
	    return 0;
	}

	memset(CFG_NAV5, 0, sizeof(CFG_NAV5));
	SetGPSMessage(CFG_NAV5, sizeof(CFG_NAV5));
	SetGPSTestMessage(TEST_MSG, sizeof(TEST_MSG));

	//write(uart_fd, TEST_MSG, sizeof(TEST_MSG));
	write(uart_fd, CFG_NAV5, sizeof(CFG_NAV5));

	while(1)
	{
		if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
		{
			if (rx_buffer[0] == 0x24)
			{
				printf("NMEA message\n");		
			}
			if (rx_buffer[0] == 0xB5)
			{
				if (GPSReceiveMessage(uart_fd, recv_message) != -1) 
				{
					printf("Message Received from GPS\n");
					printf("Class: %02x. ID: %02x\n", recv_message[0], recv_message[1]);
				}
			}
		}
	}
}
	
