
/**
 * @file sensor.h
 * @purpose Declarations for sensor thread related stuff
 */


#ifndef _SENSOR_H
#define _SENSOR_H

/** Number of data points to keep in sensor buffers **/
#define SENSOR_DATABUFSIZ 10

#define SENSOR_QUERYBUFSIZ 7

/** Number of sensors **/
#define NUMSENSORS 1

/** Structure to represent data recorded by a sensor at an instant in time **/
typedef struct
{
	/** Time at which data was taken **/
	float time;
	/** Value of data **/
	float value;
} DataPoint;

/** Structure to represent a sensor **/
typedef struct
{
	/** ID number of the sensor **/
	enum {SENSOR_TEST0=0, SENSOR_TEST1=1, SENSOR_NONE} id;
	/** Buffers to read and store data from the sensor **/
	DataPoint buffers[2][SENSOR_DATABUFSIZ];
	/** Pointer to the current buffer to write to **/
	DataPoint *write_buffer;
	/** Pointer to the current buffer to read from **/
	const DataPoint  *read_buffer;
	/** Index of the last point read from the read buffer **/
	int read_index;
	/** Index of last point written in the data buffer **/
	int write_index;
	/** File to write data into for logging purposes **/
	FILE * file;
	/** Thread running the sensor **/
	pthread_t thread;
	/** Mutex to protect access to stuff **/
	pthread_mutex_t mutex;

	
} Sensor;

/** Array of Sensors **/
extern Sensor g_sensors[];

extern void Sensor_Init(Sensor * s, int id); // Initialise sensor
extern void * Sensor_Main(void * args); // main loop for sensor thread; pass a Sensor* cast to void*

extern int Sensor_Query(Sensor * s, DataPoint * buffer, int bufsiz); // fill buffer with sensor data

extern void Sensor_Handler(FCGIContext *context, char * params);


#endif //_SENSOR_H

