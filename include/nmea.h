#ifndef __NMEA_H__
#define __NMEA_H__

void 
initBeaconMessage();
void ProcessGGA(unsigned char * buff);
void ProcessVTG(unsigned char * buff);
void ProcessRMC(unsigned char * buff);

#endif