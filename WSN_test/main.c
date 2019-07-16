#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <getopt.h>

int sensor_number;
int measurements_number; //same for each sensor, total measured data sensor_number*measurements_number
int transmission_interval;
int transmission_time;

double ran_expo(double lambda) {
	double u;
	u = rand() / (RAND_MAX + 1.0);
	return -log(1 - u) / lambda;
}

void print_usage() {
	printf("Usage: -s (Number of sensors) -m (Number of measurements) -a (Average transmission interval in s) -t (Transmission time in ms)\n");
	printf("all parameters are integers\n");
}

int main(int argc, char *argv[])
{
	int option = 0;
	//default values without if no options given
	sensor_number = 10;
	measurements_number = 1000;
	transmission_interval = 180;
	transmission_time = 60;
	// input options processing
	while ((option = getopt(argc, argv,"s:m:a:t:")) != -1) {
		switch (option) {
		case 's' : sensor_number = atoi(optarg);
		break;
		case 'm' : measurements_number = atoi(optarg);
		break;
		case 'a' : transmission_interval = atoi(optarg);
		break;
		case 't' : transmission_time = atoi(optarg);
		break;
		default: print_usage();
		exit(EXIT_FAILURE);
		}
	}
    //display input options values
	printf(" Number of sensors:%d\n", sensor_number);
	printf(" Number of measurements:%d\n", measurements_number);
	printf(" Average transmission interval:%ds\n", transmission_interval);
	printf(" Transmission time:%dms\n", transmission_time);
    //define two-dimensional array, rows for sensors, columns for each measurement containing the poissonian intervals (transmission interval)
	//for how to for this values - https://preshing.com/20111007/how-to-generate-random-timings-for-a-poisson-process/
	int sensor_transmission[sensor_number][measurements_number];
	int i, j;
	// rate parameter
	double l;
	printf("---------------------------------\n");
	l = 1 / (double)transmission_interval;
	//printf("%f\n", l);
	srand((unsigned) time(NULL)); //seed random from actual time
	//two nested loops for initializing the values of the matrix
	for (i = 0; i < sensor_number; i++) {
		sensor_transmission[i][0] = (int) (ran_expo(l) * 1000); //the function ran_expo returns value in seconds, multiplied by 1000 to have milliseconds
		//sensor_transmission[i][0] = 0; initializing the first value on each to zero it is not a good idea, there will be much more collisions
		//printf("%d\n", sensor_transmission[i][0]);
		for (j = 1; j < measurements_number; j++) {
			sensor_transmission[i][j] = sensor_transmission[i][j-1] +(int) (ran_expo(l) * 1000); //add every generated value, to the precedent one, for having the intervals
		}
	}
	int alive = 0;
	for (i=0; i<sensor_number;i++) {
		if (sensor_transmission[i][measurements_number-1] > alive) {
			alive = sensor_transmission[i][measurements_number-1]; //sum of all maximum intervals
		}
	}
	//int days = (sensor_transmission[i][measurements_number-1])/(1000*3600*24);
	//milliseconds to...
	int seconds = (int) (alive / 1000) % 60;
	int minutes = (int) ((alive / (1000 * 60)) % 60);
	int hours = (int) ((alive / (1000 * 60 * 60)) % 24);
	int days = (int) (alive / (1000 * 60 * 60 * 24));
	printf ("Maximum measurement time %d day(s), %d hour(s), %d minute(s), %d second(s)\n", days, hours, minutes, seconds);

	printf("---------------------------------\n");
    //for each two rows
	int row1 = 0;
	int row2 = 0;
	int collisions = 0;
	//algorithm to compare two sorted lists, here every two rows of the array
	//inspired by https://www.geeksforgeeks.org/union-and-intersection-of-two-sorted-arrays-2/
	for (row1 = 0; row1 < sensor_number - 1; row1++) {
		for (row2 = row1 + 1; row2 < sensor_number; row2++) {
			i = 0;
			j = 0;
			//printf("row1:%d row2:%d\n",row1,row2);
			while (i < measurements_number && j < measurements_number)//if the last measurent not reached
			{
				int m = sensor_transmission[row1][i];
				int n = sensor_transmission[row2][j];
				if ((sensor_transmission[row1][i] - sensor_transmission[row2][j]) > transmission_time) //if value greater then transmission_time (millisecpnds) there is no collision
					j++;
				else if ((sensor_transmission[row2][j] - sensor_transmission[row1][i]) > transmission_time)// same here
					i++;
				else /*if (abs(sensor_=-transmission[row1][j] - sensor_transmission[row2][i]) <= transmission_time) - there is collision */
				{
					//display data about collision
					printf("  s:%d\t packet.id:%d\t value:%d",row1,i,m);
					printf("   \t s:%d\tpacket.id:%d\tvalue:%d\n",row2,j,n);
					i++;
					collisions++; //counting collisions
				}
			}
		}
	}
	printf("---------------------------------\n");
	printf("There are %d collisions\n", collisions);
	//calculate probability as in "Wireless sensor convergecast based on random operations procedure" - formula 4.5
	// https://www.infona.pl/resource/bwmeta1.element.baztech-article-BSW4-0079-0041
	float s,p1,p;
	s = (float)transmission_time/1000.0;
	p1 = ((float)sensor_number*s)/(float)transmission_interval;
	p = 1-pow((1-p1),(float)sensor_number);
	// Probability of collision for 100 measurements/each sensor
	printf("Probability of collision %.6f",p);
	return 0;
}
