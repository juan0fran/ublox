#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>

#include <gps.h>
#include <nmea.h>

int
main (void)
{
    int uart_fd;
	unsigned char recv_message[256];
	int i, rx_len;
	MessageIdentifier id;
	uart_fd = OpenGPSIface(500);
	initBeaconMessage("localhost", "52001");
	if (uart_fd == -1)
	{
	    printf("No GPS Device\n");
	    return 0;
	}
	if (SetGPSMessage(uart_fd, CFG_NAV5) != 1)
	{
		printf("Error\n");
		exit(-1);
	}
	if (GetGPSMessage(uart_fd, CFG_NAV5) != 1)
	{
		exit(-1);
		printf("Error\n");
	}
	while(1)
	{
		/* -1 means id failure */
		if (id = GPSReceiveMessage(uart_fd, recv_message, &rx_len), id != ERROR)
		{
			if (id == NMEA_ID)
			{
				if (strncmp(recv_message+2, "GGA,", 4) == 0)
				{
					fprintf(stderr, "%s", recv_message);
					ProcessGGA(recv_message+6);
					/* Global fix frame arrived */
				}
				if (strncmp(recv_message+2, "VTG,", 4) == 0)
				{
					fprintf(stderr, "%s", recv_message);
					ProcessVTG(recv_message+6);
					/* Velocity frame arrived */
				}
				if (strncmp(recv_message+2, "RMC,", 4) == 0)
				{
					fprintf(stderr, "%s", recv_message);
					ProcessRMC(recv_message+6);
					/* Time frame arrvied */
				}
			}
		}
	}
}
	
