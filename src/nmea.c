#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gps.h>

void 
ProcessGGA(
	unsigned char * buff)
{
	char n_s;
	char e_w;
	double lat;
	double lon;
	double time;
	int quality;
	int sv;
	double sea_alt;
	double geoid_alt;

	int degrees;
	double minutes;

	char str[12][20];

	printf("Process GGA: %s", buff);
	char example[] = "082915.00,4123.27280,N,00206.72341,E,1,12,0.95,97.4,M,49.3,M,,*72";

	int matches = sscanf(example, 	"%[^,] %*[,] %[^,] %*[,] %[^,] %*[,] %[^,] %*[,] %[^,] %*[,]"
									"%[^,] %*[,] %[^,] %*[,] %[^,] %*[,] %[^,] %*[,] %[^,] %*[,]"
									"%[^,] %*[,] %[^,] %*[,]",
									str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], 
									str[8], str[9], str[10], str[11]);

	if (matches > 0)
	{
		printf("Amount of matches: %d\n", matches);
		time 		= strtof(str[0], NULL);
		
		sscanf(str[1], "%02d%f", &degrees, &minutes);
		printf("Degress: %d. Minutes: %f\n", degrees, minutes);
		lat 		= (double) ((double)degrees*1.0) + (minutes/60.0);
		/* North-South */
		if (str[2][0] == 'S')
		{
			lat 	= -1.0 * lat;
		}
		sscanf(str[3], "%02d%f", &degrees, &minutes);
		printf("Degress: %d. Minutes: %f\n", degrees, minutes);
		lon 		= (double) ((double)degrees*1.0) + (minutes/60.0);
		if (str[4][0] == 'W')
		{
			lon 	= -1.0 * lon;
		}
		/* East-Weast */
		memcpy(&e_w, str[4], 1);
		quality 	= strtol(str[5], NULL, 10);
		sv 			= strtol(str[6], NULL, 10);
		sea_alt 	= strtof(str[8], NULL);
		geoid_alt 	= strtof(str[10], NULL);
		printf("Time: %f, Lat: %f Lon: %f, Quality: %d, SV: %d, Altura(mar): %f m, Altura(geoid): %f m\n",
					time, lat, lon, quality, sv, 
					sea_alt, geoid_alt);
	}
}

void
ProcessVTG(
	unsigned char * buff)
{
	double covg;
	double knots;
	double kph;
	printf("Process VTG: %s", buff);
	int matches = sscanf(buff, "%f,T,,M,%f,N,%f,K,", 
						&covg, &knots, &kph);
	printf("Amount of matches: %d\n", matches);
	if (matches > 0)
	{
		printf("Course over Ground: %f, Km/h: %f\n", covg, kph);
	}
}