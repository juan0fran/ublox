#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <cc_beacon_iface.h>

#include <nmea.h>
#include <gps.h>

#include "../../libraries/dbman.h"

/* After 5 iterations of that, it will insert a msg */
#define DUTY_CYCLE 5

static bool have_time;
static bool have_pos;
static bool have_vel;

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
static double 			cpu_temp = 0.0;
static double 			gpu_temp = 0.0;

static int 				cycle_time;
static int 				cycle_pos;
static int 				cycle_vel;

static BeaconMessageHandler bmh;

void
initBeaconMessage(char * addr, char * port)
{
	InitUSBTemp();
	have_time 	= false;
	have_pos	= false;
	have_vel 	= false;

	cycle_vel 	= 0;
	cycle_pos 	= 0;
	cycle_time 	= 0;
}

void
closeBeaconMessage()
{
	ExitUSBTemp();
	have_time 	= false;
	have_pos	= false;
	have_vel 	= false;

	cycle_vel 	= 0;
	cycle_pos 	= 0;
	cycle_time 	= 0;
}

static int
SetBeaconMessage()
{
	char str[256];
	GPS_data data;
	if (have_time == true && have_vel == true && have_pos == true)
	{
		ProcessTemperature();
		sprintf(str, "%ld,%d,%d,%.6lf,%.6lf,%.2lf,%.2lf,%.2lf,%.1lf,%.1lf,%.1lf,%.1lf\r\n",
				gps_time, gps_quality, gps_sv, gps_lat, gps_lon, gps_alt_sea, gps_alt_geo, gps_vel, gps_course, sensor_temp, cpu_temp, gpu_temp);
		printf("%s", str);
		printf("Time: %ld, Quality: %d, SV: %d, Lat: %.6lf, Lon: %.6lf, Alt (sea): %.2lf, Alt (geo): %.2lf, Vel: %.1lf, Course: %.1lf, Temperature: %.1lf, CPU:%.1lf, GPU:%.1lf\n",
				gps_time, gps_quality, gps_sv, gps_lat, gps_lon, gps_alt_sea, gps_alt_geo, gps_vel, gps_course, sensor_temp, cpu_temp, gpu_temp);
		have_time 	= false;
		have_pos	= false;
		have_vel 	= false;

		cycle_vel 	= 0;
		cycle_pos 	= 0;
		cycle_time 	= 0;

        sprintf(data.time_local, "%11ld", time(NULL));
        sprintf(data.time_gps,   "%11ld", gps_time);
		data.lat 		= gps_lat;
		data.lng 		= gps_lon;
		data.v_kph 		= gps_vel;
		data.sea_alt 	= gps_alt_sea;
		data.geo_alt 	= gps_alt_geo;
		data.course 	= gps_course;
		data.temp 		= sensor_temp;
		data.cpu_temp 	= cpu_temp;
		data.gpu_temp 	= gpu_temp;

		dbman_save_gps_data(&data);
		/*BeaconWrite(&bmh, str, strlen(str)+1, GPS_TEMP);*/
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

	if (have_time == false && (++cycle_time == DUTY_CYCLE))
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

	if (have_pos == false && (++cycle_pos == DUTY_CYCLE))
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
			gps_lon 	= (double) ((double)decdegrees*1.0) + (minutes/60.0);
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

	if (have_vel == false && (++cycle_vel == DUTY_CYCLE))
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
    int t_aux = 0;
    FILE * cpufile;
    FILE * gpufile;
    /* Reads internal temperature sensors: */
    system("/opt/vc/bin/vcgencmd measure_temp > /home/pi/bbs/module_gps_temp/gpu_temp");
    if((cpufile = fopen("/sys/class/thermal/thermal_zone0/temp", "r")) == NULL) {
        perror("Error opening CPU file");
    } else {
        fscanf(cpufile, "%d", &t_aux);
        cpu_temp = t_aux / 1000.0;
    }
    if((gpufile = fopen("/home/pi/bbs/module_gps_temp/gpu_temp", "r")) == NULL) {
        perror("Error opening GPU file");
    } else {
        fscanf(gpufile, "%*[^=] %*[=]%lf", &gpu_temp);
    }
    if(cpufile != NULL) {
        fclose(cpufile);
    }
    if(gpufile != NULL) {
        fclose(gpufile);
    }
    /* Reads external USB temperature sensor: */
	ReadUSBTemp(&sensor_temp);
}
