#ifndef __GPS_H__
#define __GPS_H__

/* header (2)	class (1) 	id (1) 	length (2)	payload (36) crc (2) */
#define GPS_OVERHEAD	8
#define CRC_OVERHEAD	4
#define FIRST_CRC		6
#define SECOND_CRC		7

typedef enum MessageType{
	CFG_NAV5,	/* Navigation Engine Settings */
	CFG_NAVX5,	/* Navigation Engine Expert Settings */
	CFG_NMEA,	/* NMEA output configuration */
}MessageType;

typedef enum MessageIdentifier{
	NMEA_ID = 0x24,
	UBX_ID 	= 0xB5,
}MessageIdentifier;

int OpenGPSIface(void);
int GPSReceiveMessage(int uart_fd, unsigned char * recv_message, int * len);
int SetGPSMessage(int uart_fd, MessageType t);
int GetGPSMessage(int uart_fd, MessageType t);


#endif