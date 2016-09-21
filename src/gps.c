#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>

#include <gps.h>

static unsigned char UBX_header[2] = {0xB5, 0x62};

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
    }

    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 10;            // 0.5 seconds read timeout
    if (tcsetattr (uart0_filestream, TCSANOW, &tty) != 0)
    	exit(0);

	return uart0_filestream;
}

static int
GPSReceiveNMEA(
	int uart_fd,
	unsigned char * recv_message)
{

	return -1;
}

static int
GPSReceiveUBX(
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
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				memcpy(recv_message, rx_buffer, 1);
			}
			else
			{
				return -1;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				memcpy(recv_message+1, rx_buffer, 1);
			}
			else
			{
				return -1;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				memcpy(recv_message+2, rx_buffer, 1);
				msg_len = rx_buffer[0];
			}
			else
			{
				return -1;
			}
			if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
			{
				msg_len |= rx_buffer[0]<<8;
				memcpy(recv_message+3, rx_buffer, 1);
			}
			else
			{
				return -1;
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
					return -1;
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
					return -1;
				}
				if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
				{
					crc2_c = rx_buffer[0];
				}
				else
				{
					return -1;
				}
				GPSChecksum(&crc1, &crc2, recv_message, msg_len + 4);
				if (crc1 == crc1_c && crc2 == crc2_c)
				{	
					/* payload_len + class + id + 2 length bytes */
					return msg_len + 4;
				}
				else
				{
					return -1;
				}
			}
		}
	}	
	return -1;
}

/* -1 error while reading a packet! */
/* otherwise: 0 on NMEA msg, 1 on UBX message */

int 
GPSReceiveMessage(
	int uart_fd,
	unsigned char * recv_message,
	int * len)
{
	unsigned char rx_buffer[1];
	int rx_len;
	if (rx_len = read(uart_fd, rx_buffer, 1), rx_len > 0 )
	{
		if (rx_buffer[0] == NMEA_ID)
		{
			if (rx_len = GPSReceiveNMEA(uart_fd, recv_message), rx_len != -1)
			{
				printf("NMEA message received from GPS\n");
				recv_message[rx_len] = '\0';
				printf("%s\n", recv_message);
				*len = rx_len;
				return 0;
			}
			else
			{
				return -1;
			}			
		}
		if (rx_buffer[0] == UBX_ID)
		{
			if (rx_len = GPSReceiveUBX(uart_fd, recv_message), rx_len != -1) 
			{
				printf("Message Received from GPS\n");
				printf("Class: %02x. ID: %02x\n", recv_message[0], recv_message[1]);
				*len = rx_len;
				return 1;
			}
			else
			{
				return -1;
			}
		}
		return -1;
	}
	else
	{
		return -1; 
		/* i.e. no message */
	}
}

static void
PrintGPSUBXMessage(
	unsigned char * p,
	int len)
{
	int i;
	for (i = 0; i < len - CRC_OVERHEAD; i++)
	{
		printf("%02x ", p[i+CRC_OVERHEAD]);
	}
	printf("\n");	
}

int
GetGPSMessage(
	int uart_fd,
	MessageType t)
{
	unsigned char buffer[256];
	unsigned char recv_message[256];
	unsigned char tmp;
	int msg_len;
	int rx_len;
	int i;
	int message_delivered = 0;
	while(!message_delivered)
	{
		switch (t)
		{
			case CFG_NAV5:
				/* HEADER START */
				memcpy(buffer, UBX_header, sizeof(UBX_header));
				tmp = 0x06; /* class */
				memcpy(buffer+2, &tmp, 1);
				tmp = 0x24; /* id */
				memcpy(buffer+3, &tmp, 1);
				/* HEADER END */
				/* Length equal to 0 to GET */
				msg_len = 0;
				buffer[4] = 0;
				buffer[5] = 0;
				GPSChecksum(buffer + msg_len + FIRST_CRC,  	/* First CRC character */
							buffer + msg_len + SECOND_CRC, 	/* Second CRC character */
							buffer + 2, 					/* buffer without sync word */
							msg_len + CRC_OVERHEAD);		/* payload length + len (2) + class (1) + id (1) */
				/* Message is complete now */
				write(uart_fd, buffer, msg_len + GPS_OVERHEAD);
			break;

			case CFG_NAVX5:
				/* HEADER START */
				memcpy(buffer, UBX_header, sizeof(UBX_header));
				tmp = 0x06; /* class */
				memcpy(buffer+2, &tmp, 1);
				tmp = 0x23; /* id */
				memcpy(buffer+3, &tmp, 1);
				/* HEADER END */
				/* Length equal to 0 to GET */
				msg_len = 0;
				buffer[4] = 0;
				buffer[5] = 0;
				GPSChecksum(buffer + msg_len + FIRST_CRC,  	/* First CRC character */
							buffer + msg_len + SECOND_CRC, 	/* Second CRC character */
							buffer + 2, 					/* buffer without sync word */
							msg_len + CRC_OVERHEAD);		/* payload length + len (2) + class (1) + id (1) */
				/* Message is complete now */
				write(uart_fd, buffer, msg_len + GPS_OVERHEAD);
			break;

			case CFG_NMEA:
				/* HEADER START */
				memcpy(buffer, UBX_header, sizeof(UBX_header));
				tmp = 0x06; /* class */
				memcpy(buffer+2, &tmp, 1);
				tmp = 0x17; /* id */
				memcpy(buffer+3, &tmp, 1);
				/* HEADER END */
				/* Length equal to 0 to GET */
				msg_len = 0;
				buffer[4] = 0;
				buffer[5] = 0;
				GPSChecksum(buffer + msg_len + FIRST_CRC,  	/* First CRC character */
							buffer + msg_len + SECOND_CRC, 	/* Second CRC character */
							buffer + 2, 					/* buffer without sync word */
							msg_len + CRC_OVERHEAD);		/* payload length + len (2) + class (1) + id (1) */
				/* Message is complete now */
				write(uart_fd, buffer, msg_len + GPS_OVERHEAD);
			break;
		}
		if (rx_len = GPSReceiveUBX(uart_fd, recv_message), rx_len != -1)
		{
			switch (t)
			{
				case CFG_NAV5:
					if (recv_message[0] == 0x06 && recv_message[1] == 0x24)
					{
						printf("CFG NAV5 received\n");
						PrintGPSUBXMessage(recv_message, rx_len);
					}
				break;

				case CFG_NAVX5:
					if (recv_message[0] == 0x06 && recv_message[1] == 0x23)
					{
						printf("CFG NAVX5 received\n");
						PrintGPSUBXMessage(recv_message, rx_len);
					}
				break;

				case CFG_NMEA:
					if (recv_message[0] == 0x06 && recv_message[1] == 0x17)
					{
						printf("CFG NMEA received\n");
						PrintGPSUBXMessage(recv_message, rx_len);
					}
				break;
			}
			message_delivered = 1;
		}
	}
	return 0;
}

int 
SetGPSMessage(
	int uart_fd,
	MessageType t)
{
	switch (t)
	{
		case CFG_NAV5:


		break;

		case CFG_NAVX5:


		break;

		case CFG_NMEA:


		break;
	}
	return 0;
}
