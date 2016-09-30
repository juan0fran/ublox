#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <cc_beacon_iface.h>

#include <nmea.h>
#include <gps.h>

static bool have_time;
static bool have_pos;
static bool have_vel;
static bool have_temp;

static unsigned long 	gps_time;
static int 				gps_quality;
static int 	 			gps_sv;
static double 			gps_lat;
static double 			gps_lon;
static double 			gps_alt_sea;
static double 			gps_alt_geo;
static double 			gps_vel;
static double 			gps_course;
static double 			sensor_temp;

static BeaconMessageHandler bmh;

void
initBeaconMessage(char * addr, char * port)
{
	BeaconConnect(addr, port, &bmh, beacon_sender);
	InitUSBTemp();
	have_time 	= false;
	have_pos	= false;
	have_vel 	= false;
	have_temp 	= false;
}

void
closeBeaconMessage()
{
	BeaconClose(&bmh);
	ExitUSBTemp();
}

static int
SetBeaconMessage()
{	
	char str[256];
	if (have_time == true && have_vel == true && have_pos == true)
	{	
		sprintf(str, "%ld,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf\r\n",
				gps_time, gps_quality, gps_sv, gps_lat, gps_lon, gps_alt_sea, gps_alt_geo, gps_vel, gps_course, sensor_temp);
		printf("%s", str);
		printf("Time: %ld, Quality: %d, SV: %d, Lat: %lf, Lon: %lf, Alt (sea): %lf, Alt (geo): %lf, Vel: %lf, Course: %lf, Temperature: %lf\n",
				gps_time, gps_quality, gps_sv, gps_lat, gps_lon, gps_alt_sea, gps_alt_geo, gps_vel, gps_course, sensor_temp);
		have_time 	= false;
		have_pos	= false;
		have_vel 	= false;
		have_temp 	= false;
		BeaconWrite(&bmh, str, strlen(str)+1, GPS_TEMP);
	}
}

int
CommaParsing(
	char * buff,
	int max_args, int str_len,
	char str[max_args][str_len])
{
	char * aux = malloc(256);
	char * aux_malloced = aux;
	char * pt;
    int i = 0;
    int matches = 0;
	/* safe use of buff */
	strcpy(aux, buff);
    pt = strsep(&aux,",");  
    while (pt != NULL)
    {
        strcpy(str[i], pt);
        i++;
        pt = strsep (&aux, ",");
        if (i == max_args)
        {
            free(aux_malloced);
        	return i;
        }
    }
    free(aux_malloced);
    return i;
}

void
ProcessRMC(
	unsigned char * buff)
{
	int hour;
	int min;
	int sec;
	int day;
	int mon;
	int year;
	char str[9][20];
	struct tm t;
	int matches;

	if (have_time == false)
	{
		matches = CommaParsing((char *) buff, 9, 20, str);
		if (matches == 9)
		{
			sscanf(str[0], "%02d%02d%02d", &hour, &min, &sec);
			sscanf(str[8], "%02d%02d%02d", &day, &mon, &year);
			/* year must be YYYY - 1900, but we have YY, so, 2000 - 1900 = 100 */
			t.tm_year = 100 + year;
			t.tm_mon = mon - 1;
			t.tm_mday = day;
			t.tm_hour = hour;
			t.tm_min = min;
			t.tm_sec = sec;
			gps_time = mktime(&t);
			
			have_time = true;
			SetBeaconMessage();
		}
	}
}

void 
ProcessGGA(
	unsigned char * buff)
{
	double degrees;
	int decdegrees;
	double minutes;

	char str[12][20];

	int matches;

	if (have_pos == false)
	{
		matches = CommaParsing((char *) buff, 12, 20, str);
		if (matches == 12)
		{        
			degrees 	= strtof(str[1], NULL) / 100.0;
	        decdegrees 	= (int) degrees;
	        minutes  	= (double) (degrees - decdegrees)*100.0;
			gps_lat 	= (double) ((double)decdegrees*1.0) + (minutes/60.0);

			if (str[2][0] == 'S')
			{
				gps_lat 	= -1.0 * gps_lat;
			}
			
			degrees 	= strtof(str[3], NULL) / 100.0;
	        decdegrees 	= (int) degrees;
	        minutes  	= (double) (degrees - decdegrees)*100.0;
			gps_lon 	= (double) ((double)degrees*1.0) + (minutes/60.0);
			if (str[4][0] == 'W')
			{
				gps_lon 	= -1.0 * gps_lon;
			}

			gps_quality 	= strtol(str[5], NULL, 10);
			gps_sv 			= strtol(str[6], NULL, 10);
			gps_alt_sea 	= strtof(str[8], NULL);
			gps_alt_geo 	= strtof(str[10], NULL);

			have_pos = true;
			SetBeaconMessage();
		}
	}
}

void
ProcessVTG(
	unsigned char * buff)
{
	char str[8][20];
	int matches;

	if (have_vel == false)
	{
		matches = CommaParsing((char *) buff, 8, 20, str);
		if (matches == 8)
		{
			gps_course 	= strtof(str[0], NULL);
			gps_vel 	= strtof(str[6], NULL);

			have_vel = true;
			SetBeaconMessage();
		}
	}
}


void 
ProcessTemperature()
{
	if (have_temp == false)
	{
		sensor_temp = ReadUSBTemp();
		have_temp = true;
	}
}
