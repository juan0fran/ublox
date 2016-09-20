#ifndef __GPS_H__
#define __GPS_H__

/* header (2)	class (1) 	id (1) 	length (2)	payload (36) crc (2) */
#define GPS_OVERHEAD	8

int OpenGPSIface(void);
int GPSReceiveMessage(int uart_fd, unsigned char * recv_message);
void SetGPSMessage(unsigned char * p, int len);
void SetGPSTestMessage(unsigned char * p, int len);

#endif