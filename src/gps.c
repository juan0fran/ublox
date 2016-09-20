#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>

#include <gps.h>

static void
GPSChecksum(
	unsigned char * check_a, 
	unsigned char * check_b, 
	unsigned char * buffer, 
	int len)
{
	int i;
	int CK_A = 0;
	int CK_B = 0;
	for(i=0;i<len;i++)
	{
		CK_A = CK_A + buffer[i];
		CK_B = CK_B + CK_A;
	}
	*check_a = (unsigned char) CK_A & 0xFF;
	*check_b = (unsigned char) CK_B & 0xFF;
}

int
OpenGPSIface(void)
{
	int uart0_filestream = -1;
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
		exit(0);
	}
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);
 	struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (uart0_filestream, &tty) != 0)
    {
            exit(0);
            return;
    }

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 10;            // 0.5 seconds read timeout
    if (tcsetattr (uart0_filestream, TCSANOW, &tty) != 0)
    	exit(0);

	return uart0_filestream;
}

int
GPSReceiveMessage(
	int uart_fd,
	unsigned char * recv_message)
{
	unsigned char rx_buffer[256];
	unsigned char crc1_c; unsigned char crc2_c;
	unsigned char crc1; unsigned char crc2;
	int rx_len;
	int offset;
	int msg_len;

	if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
	{
		if (rx_buffer[0] == 0x62)
		{
			//printf("Sync Word Complete\n");
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				//printf("Class: %02x, ", rx_buffer[0]);
				memcpy(recv_message, rx_buffer, 1);
			}
			else
			{
				return -1;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				//printf("ID: %02x, ", rx_buffer[0]);
				memcpy(recv_message+1, rx_buffer, 1);
			}
			else
			{
				return -1;;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				memcpy(recv_message+2, rx_buffer, 1);
				msg_len = rx_buffer[0];
			}
			else
			{
				return -1;;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				msg_len |= rx_buffer[0]<<8;
				//printf("Message len: %d\n", msg_len);
				memcpy(recv_message+3, rx_buffer, 1);
			}
			else
			{
				return -1;;
			}	
			offset = 0;
			while(offset < msg_len)
			{
				if (rx_len = read(uart_fd, rx_buffer, msg_len - offset), rx_len > 0 )
				{
					memcpy(recv_message+offset+4, rx_buffer, rx_len);
					offset += rx_len;
				}
				else
				{
					return -1;;
				}
			}
			if (offset == msg_len)		
			{
				if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
				{
					crc1_c = rx_buffer[0];
				}
				else
				{
					return -1;;
				}
				if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
				{
					crc2_c = rx_buffer[0];
				}
				else
				{
					return -1;;
				}	
				recv_message[msg_len + 4] = '\0';
				//printf("Received: %s\n", recv_message + 4);
				GPSChecksum(&crc1, &crc2, recv_message, msg_len + 4);
				if (crc1 == crc1_c && crc2 == crc2_c)
				{
					//printf("CRC OK\n");
					return msg_len;
				}
				else
				{
					return -1;
				}
			}
		}
	}	
}

void
SetGPSTestMessage(
	unsigned char * p,
	int len)
{
		/* sync bytes */
	p[0] = 0xB5;
	p[1] = 0x62;
	/* class */
	p[2] = 0x0A;
	/* id */
	p[3] = 0x04;
	p[4] = 0x00;
	p[5] = 0x00;
	GPSChecksum(p+len-2, p+len-1, p + 2, len - 2 - 2);
}


void
SetGPSMessage(
	unsigned char * p,
	int len)
{
	/* sync bytes */
	p[0] = 0xB5;
	p[1] = 0x62;
	/* class */
	p[2] = 0x06;
	/* id */
	p[3] = 0x24;
	/* length in 16 bit little endian */
	p[4] = 36;
	p[5] = 0;
	/* payload */
	/* bit mask -> applies only to dynamism of the receiver */
	p[6] = 0x01;
	/* put the receiver in airborne 2g mode */
	p[7] = 7;
	/* rest leave it */
	/* length MINUS (-) sync bytes (2) and crc (2) */
	GPSChecksum(p+len-2, p+len-1, p + 2, len - 2 - 2);
}