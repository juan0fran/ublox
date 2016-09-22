#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>

#include <gps.h>

static enum Overhead{
	GPS_OVERHEAD 	= 8,
	CRC_OVERHEAD	= 4,
	PAYLOAD_START	= 6,
	FIRST_CRC		= 6,
	SECOND_CRC 		= 7,
}Overhead;

static int init_timeout;
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
OpenGPSIface(int ms_timeout)
{
	int uart0_filestream = -1;
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);		//Open in blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		fprintf(stderr, "Error - Unable to open UART.  Ensure it is not in use by another application\n");
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

	init_timeout = ms_timeout;
	
	return uart0_filestream;
}

static int
InputTimeout(
	int fd, 
	int milliseconds)
{
	fd_set set;
	struct timeval timeout;
	/* Initialize the file descriptor set. */
	FD_ZERO (&set);
	FD_SET (fd, &set);

	/* Initialize the timeout data structure. */
	timeout.tv_sec = 0;
	timeout.tv_usec = milliseconds * 1000; /* microsec*1000

	/* select returns 0 if timeout, 1 if input available, -1 if error. */
	return (select (FD_SETSIZE, &set, NULL, NULL, &timeout));	
}

static int
read_with_timeout(
	int fd,
	unsigned char * buff,
	int len,
	int timeout_ms)
{
	if (InputTimeout(fd, timeout_ms) > 0)
	{
		return read(fd, buff, len);
	}
	else
	{
		return 0;
	}
}

static int
GPSReceiveNMEA(
	int uart_fd,
	unsigned char * recv_message)
{
	unsigned char rx_buffer[1];
	unsigned char string[256];
	int offset;
	int rx_len;
	int i;
	int checksum;
	int read_checksum;
	offset = 0;
	do
	{
		if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
		{	
			memcpy(recv_message+offset, rx_buffer, 1);
			offset++;
		}
		else
		{
			return -1;
		}

	}while(rx_buffer[0] != '\n');
	rx_len = offset;
	checksum = 0;
	for (i = 0; i < offset - 5; i++)
	{
		checksum ^= recv_message[i];
	}
	i = 0;
	while(recv_message[i] != (unsigned char) '*')
	{
		i++;
	}
	sscanf(recv_message+i, "*%02x\r\n", &read_checksum);
	if (checksum == read_checksum)
	{
		return rx_len;
	}
	else
	{
		return -1;
	}
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
	if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
	{
		if (rx_buffer[0] == 0x62)
		{
			if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
			{
				memcpy(recv_message, rx_buffer, 1);
			}
			else
			{
				return -1;
			}
			if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
			{
				memcpy(recv_message+1, rx_buffer, 1);
			}
			else
			{
				return -1;
			}
			if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
			{
				memcpy(recv_message+2, rx_buffer, 1);
				msg_len = rx_buffer[0];
			}
			else
			{
				return -1;
			}
			if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
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
				if (rx_len = read_with_timeout(uart_fd, rx_buffer, msg_len - offset, init_timeout), rx_len > 0 )
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
				if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
				{
					crc1_c = rx_buffer[0];
				}
				else
				{
					return -1;
				}
				if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
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

MessageIdentifier 
GPSReceiveMessage(
	int uart_fd,
	unsigned char * recv_message,
	int * len)
{
	unsigned char rx_buffer[1];
	int rx_len;
	if (rx_len = read_with_timeout(uart_fd, rx_buffer, 1, init_timeout), rx_len > 0 )
	{
		if (rx_buffer[0] == (unsigned char) NMEA_ID)
		{
			if (rx_len = GPSReceiveNMEA(uart_fd, recv_message), rx_len != -1)
			{
				recv_message[rx_len] = '\0';
				*len = rx_len;
				return NMEA_ID;
			}
			else
			{
				return ERROR;
			}			
		}
		if (rx_buffer[0] == (unsigned char) UBX_ID)
		{
			if (rx_len = GPSReceiveUBX(uart_fd, recv_message), rx_len != -1) 
			{
				*len = rx_len;
				return UBX_ID;
			}
			else
			{
				return ERROR;
			}
		}
		return ERROR;
	}
	else
	{
		return ERROR; 
		/* i.e. no message */
	}
}


// -------------------------------------------------------------------------------------------------
static void _print_block(const unsigned char *pblock, size_t size)
// -------------------------------------------------------------------------------------------------
{
    size_t i;
    char   c;
    size_t lsize = 10;
    int    flush = 1;

    fprintf(stderr, "     ");

    for (i=0; i<lsize; i++)
    {
        fprintf(stderr, "%d    ", (int)i);
    }

    for (i=0; i<size; i++)
    {
        if (i % lsize == 0)
        {
            fprintf(stderr, "\n%03d: ", (int)i);
        }

        if ((pblock[i] < 0x20) || (pblock[i] >= 0x7F))
        {
            c = '.';
        }
        else
        {
            c = (char) pblock[i];
        }

        fprintf(stderr, "%02X:%c ", pblock[i], (char)c);
    }
    fprintf(stderr, "\n\n");
    if (flush)
        fflush(stderr);
}

static void
PrintGPSUBXMessage(
	unsigned char * p,
	int len)
{
	_print_block(p+CRC_OVERHEAD, len - CRC_OVERHEAD);
}

static void clean_read_buffer(int fd)
{
	unsigned char buffer[256];
	while(read_with_timeout(fd, buffer, 256, 0) > 0);
}

int
GetGPSMessage(
	int uart_fd,
	MessageType t)
{	
	int limit = 4;
	int recv_limit = 512;
	unsigned char buffer[256];
	unsigned char recv_message[256];
	unsigned char tmp;
	int msg_len;
	int rx_len;
	int i;
	int timeout = 0;
	int recv_timeout = 0;
	int message_delivered = 0;
	
	clean_read_buffer(uart_fd);
	while(!message_delivered && (++timeout < limit))
	{
		if (timeout > 1)
		{
			fprintf(stderr, "Message not delivered, trying again\n");
		}
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
		recv_timeout = 0;
		while( (rx_len = read_with_timeout(uart_fd, buffer, 1, init_timeout), rx_len > 0 ) && message_delivered == 0 && (++recv_timeout < recv_limit) )
		{
			if (buffer[0] == (unsigned char) UBX_ID)
			{
				if (rx_len = GPSReceiveUBX(uart_fd, recv_message), rx_len != -1)
				{
					switch (t)
					{
						case CFG_NAV5:
							if (recv_message[0] == 0x06 && recv_message[1] == 0x24)
							{
								fprintf(stderr, "CFG NAV5 received:\n");
								PrintGPSUBXMessage(recv_message, rx_len);
								message_delivered = 1;
							}
						break;

						case CFG_NAVX5:
							if (recv_message[0] == 0x06 && recv_message[1] == 0x23)
							{
								fprintf(stderr, "CFG NAVX5 received:\n");
								PrintGPSUBXMessage(recv_message, rx_len);
								message_delivered = 1;
							}
						break;

						case CFG_NMEA:
							if (recv_message[0] == 0x06 && recv_message[1] == 0x17)
							{
								fprintf(stderr, "CFG NMEA received:\n");
								PrintGPSUBXMessage(recv_message, rx_len);
								message_delivered = 1;
							}
						break;
					}
				}
			}
		}
	}
	return message_delivered;
}

int 
SetGPSMessage(
	int uart_fd,
	MessageType t)
{
	unsigned char buffer[256];
	unsigned char recv_message[256];
	unsigned char tmp;
	int msg_len;
	int rx_len;
	int i;
	int limit = 4;
	int recv_limit = 512;
	int message_delivered = 0;
	int timeout = 0;
	int recv_timeout = 0;
	
	clean_read_buffer(uart_fd);
	while(!message_delivered && (++timeout < limit) )
	{
		if (timeout > 1)
		{
			fprintf(stderr, "Message not deivered, trying again\n");
		}		
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
				msg_len = 36;
				buffer[4] = msg_len&0xFF;
				buffer[5] = (msg_len>>8)&0xFF;
				memset(buffer + PAYLOAD_START, 0, msg_len);
				buffer[PAYLOAD_START] = 0x01;
				buffer[PAYLOAD_START + 1] = 0x00;
				buffer[PAYLOAD_START + 2] = 7;
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
				buffer[4] = msg_len&0xFF;
				buffer[5] = (msg_len>>8)&0xFF;
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
				buffer[4] = msg_len&0xFF;
				buffer[5] = (msg_len>>8)&0xFF;
				GPSChecksum(buffer + msg_len + FIRST_CRC,  	/* First CRC character */
							buffer + msg_len + SECOND_CRC, 	/* Second CRC character */
							buffer + 2, 					/* buffer without sync word */
							msg_len + CRC_OVERHEAD);		/* payload length + len (2) + class (1) + id (1) */
				/* Message is complete now */
				write(uart_fd, buffer, msg_len + GPS_OVERHEAD);
			break;
		}
		recv_timeout = 0;
		while( (read_with_timeout(uart_fd, buffer, 1, init_timeout) ) && message_delivered == 0  && (++recv_timeout < recv_limit) )
		{
			if (buffer[0] == (unsigned char) UBX_ID)
			{
				if (rx_len = GPSReceiveUBX(uart_fd, recv_message), rx_len != -1)
				{
					if (recv_message[0] == 0x05 && recv_message[1] == 0x01)
					{
						fprintf(stderr, "ACK received:\n");
						PrintGPSUBXMessage(recv_message, rx_len);
						message_delivered = 1;
					}
				}
			}
		}
	}
	return message_delivered;
}
