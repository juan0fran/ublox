
#ifndef __CC_BEACON_IFACE_H__
#define __CC_BEACON_IFACE_H__

#ifndef BYTE
#define BYTE unsigned char
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <stdint.h>

typedef enum MsgSource{
	VITOW,
	GPS_TEMP,
	SYSTEM,
}MsgSource;

typedef enum MessagePurpose{
	beacon_sender,
	beacon_receiver,
}MessagePurpose;

typedef struct BeaconMessageHandler{
	int fd;
	struct sockaddr_in addr;
	socklen_t len;
}BeaconMessageHandler;

/* Init functions */
int 	BeaconConnect (const char * ip, const char * port, BeaconMessageHandler * bmh, MessagePurpose trx);
void 	BeaconClose (BeaconMessageHandler * bmh);

/* Reading functions */
int 	BeaconWrite (BeaconMessageHandler * bmh, BYTE * msg, int32_t len, MsgSource m);
int 	BeaconRead (BeaconMessageHandler * bmh, BYTE * msg, int32_t maxbuflen, MsgSource * m); /* returns the source if wanted */

#endif
