#ifndef __CC_BEACON_IFACE_H__
#define __CC_BEACON_IFACE_H__

#ifndef BYTE
#define BYTE unsigned char
#endif

#include <stdint.h>

/* Init functions */
int 	BeaconConnect (char * addr, char * port);
void 	BeaconClose (int fd);

/* Reading functions */
int 	BeaconWrite (int fd, BYTE * msg, int32_t len);
int 	BeaconRead (int fd, BYTE * msg, int32_t maxbuflen);

#endif