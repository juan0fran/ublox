#ifndef __NMEA_H__
#define __NMEA_H__

void initBeaconMessage(char * addr, char * port);
void closeBeaconMessage(void);
void ProcessGGA(unsigned char * buff);
void ProcessVTG(unsigned char * buff);
void ProcessRMC(unsigned char * buff);
void ProcessTemperature(void);

#endif