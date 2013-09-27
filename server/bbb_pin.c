/**
 * @file bbb_pin.c
 * @purpose Implementation of BBB pin control functions and structures
 * THIS CODE IS NOT THREADSAFE
 */

#define _BBB_PIN_SRC
#include "bbb_pin.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "options.h"

/**
 * Structure to represent a GPIO pin
 * Note: Only accessable to this file; to use the functions pass a GPIOId
 */
typedef struct
{
	int fd_value;
	int fd_direction;
} GPIO_Pin;

/**
 * Structure to represent an ADC pin
 * Note: Only accessable to this file; to use the functions pass a ADCId
 */
typedef struct
{
	int fd_value;
} ADC_Pin;

/**
 * Structure to represent a PWM pin
 * Note: Only accessable to this file; to use the functions pass a PWMId
 */
typedef struct
{
	int fd_run;
	FILE * file_duty;
	FILE * file_period;
	int fd_polarity;
} PWM_Pin;

/** Array of GPIO pins **/
static GPIO_Pin g_gpio[GPIO_NUM_PINS] = {{0}};
/** Array of ADC pins **/
static ADC_Pin g_adc[ADC_NUM_PINS] = {{0}};
/** Array of PWM pins **/
static PWM_Pin g_pwm[PWM_NUM_PINS] = {{0}};

static char g_buffer[BUFSIZ] = "";


/**
 * Export a GPIO pin and open the file descriptors
 */
void GPIO_Export(int pin)
{
	if (pin < 0 || pin >= GPIO_INDEX_SIZE || g_gpio_to_index[pin] == 128)
	{
		Abort("Not a useable pin number: %d", pin);
	}

	GPIO_Pin *gpio = &g_gpio[g_gpio_to_index[pin]];
	// Export the pin
	sprintf(g_buffer, "%s/export", GPIO_DEVICE_PATH);
	FILE * export = fopen(g_buffer, "w");
	if (export == NULL)
	{
		Abort("Couldn't open %s to export GPIO pin %d - %s", g_buffer, pin, strerror(errno));
	}

	fprintf(export, "%d", pin);	
	fclose(export);
	
	// Setup direction file descriptor
	sprintf(g_buffer, "%s/gpio%d/direction", GPIO_DEVICE_PATH, pin);
	gpio->fd_direction = open(g_buffer, O_RDWR);
	if (gpio->fd_direction < 0)
	{
		Abort("Couldn't open %s for GPIO pin %d - %s", g_buffer, pin, strerror(errno));
	}


	// Setup value file descriptor
	sprintf(g_buffer, "%s/gpio%d/value", GPIO_DEVICE_PATH, pin);
	gpio->fd_value = open(g_buffer, O_RDWR);
	if (gpio->fd_value < 0)
	{
		Abort("Couldn't open %s for GPIO pin %d - %s", g_buffer, pin, strerror(errno));
	}

	Log(LOGDEBUG, "Exported GPIO%d", pin);
	//sleep(1);
}

/**
 * Unexport a GPIO pin and close its' file descriptors
 */
void GPIO_Unexport(int pin)
{
	if (pin < 0 || pin >= GPIO_INDEX_SIZE || g_gpio_to_index[pin] == 128)
	{
		Abort("Not a useable pin number: %d", pin);
	}

	GPIO_Pin *gpio = &g_gpio[g_gpio_to_index[pin]];
	// Close file descriptors
	close(gpio->fd_value);
	close(gpio->fd_direction);

	// Unexport the pin

	if (g_buffer[0] == '\0')
		sprintf(g_buffer, "%s/unexport", GPIO_DEVICE_PATH);	
	FILE * export = fopen(g_buffer, "w");
	if (export == NULL)
	{
		Abort("Couldn't open %s to export GPIO pin %d - %s", g_buffer, pin, strerror(errno));
	}

	fprintf(export, "%d", pin);	
	fclose(export);
}


/**
 * Initialise all PWM pins and open file descriptors
 * @param pin - The sysfs pin number
 */
void PWM_Export(int pin)
{
	if (pin < 0 || pin >= PWM_NUM_PINS)
	{
		Abort("Invalid PWM pin number %d specified.", pin);
	}

	PWM_Pin *pwm = &g_pwm[pin];

	if (pwm->file_duty != NULL)
	{
		Abort("PWM %d already exported.", pin);
	}

	// Open file descriptors
	sprintf(g_buffer, "%s/pwm%d/run", PWM_DEVICE_PATH, pin);
	pwm->fd_run = open(g_buffer, O_WRONLY);
	if (pwm->fd_run < 0)
	{
		Abort("Couldn't open %s for PWM%d - %s", g_buffer, pin, strerror(errno));
	}

	sprintf(g_buffer, "%s/pwm%d/polarity", PWM_DEVICE_PATH, pin);
	pwm->fd_polarity = open(g_buffer, O_WRONLY);
	if (pwm->fd_polarity < 0)
	{
		Abort("Couldn't open %s for PWM%d - %s", g_buffer, pin, strerror(errno));
	}

	sprintf(g_buffer, "%s/pwm%d/period_ns", PWM_DEVICE_PATH, pin);
	pwm->file_period = fopen(g_buffer, "w");
	if (pwm->file_period == NULL)
	{
		Abort("Couldn't open %s for PWM%d - %s", g_buffer, pin, strerror(errno));
	}

	sprintf(g_buffer, "%s/pwm%d/duty_ns", PWM_DEVICE_PATH, pin);
	pwm->file_duty = fopen(g_buffer, "w");
	if (pwm->file_duty == NULL)
	{
		Abort("Couldn't open %s for PWM%d - %s", g_buffer, pin, strerror(errno));
	}

	// Don't buffer the streams
	setbuf(pwm->file_period, NULL);
	setbuf(pwm->file_duty, NULL);	

	Log(LOGDEBUG, "Exported PWM%d", pin);
}

/**
 * Unexport a PWM pin and close its file descriptors
 * @param pin - The sysfs pin number
 */
void PWM_Unexport(int pin)
{
	if (pin < 0 || pin >= PWM_NUM_PINS)
	{
		Abort("Invalid PWM pin number %d specified.", pin);
	}

	PWM_Pin *pwm = &g_pwm[pin];
	// Close the file descriptors
	if (pwm->fd_polarity != -1)
		close(pwm->fd_polarity);
	if (pwm->fd_run != -1)
		close(pwm->fd_run);
	if (pwm->file_period != NULL)
		fclose(pwm->file_period);
	if (pwm->file_duty != NULL)
		fclose(pwm->file_duty);

	//So that another call to PWM_Unexport is OK.
	pwm->fd_polarity = pwm->fd_run = -1;
	pwm->file_period = pwm->file_duty = NULL;
}


/**
 * Initialise ADC structures
 * @param pin The ADC pin number
 */
void ADC_Export(int pin)
{
	if (pin < 0 || pin >= ADC_NUM_PINS)
	{
		Abort("Invalid ADC pin %d specified.", pin);
	}

	sprintf(g_buffer, "%s/in_voltage%d_raw", g_options.adc_device_path, pin);
	g_adc[pin].fd_value = open(g_buffer, O_RDONLY);
	if (g_adc[pin].fd_value <0)
	{
		Abort("Couldn't open ADC %d device file %s - %s", pin, g_buffer, strerror(errno));
	}
	Log(LOGDEBUG, "Opened ADC %d", pin);
}

/**
 * Unexport ADC pins
 * @param pin The ADC pin number
 */
void ADC_Unexport(int pin)
{
	if (pin < 0 || pin >= ADC_NUM_PINS)
	{
		Abort("Invalid ADC pin %d specified.", pin);
	}
	if (pin != -1)
		close(g_adc[pin].fd_value);	

	g_adc[pin].fd_value = -1;
}

/**
 * Set a GPIO pin
 * @param pin - The pin to set. MUST have been exported before calling this function.
 */
void GPIO_Set(int pin, bool value)
{
	if (pin < 0 || pin >= GPIO_INDEX_SIZE || g_gpio_to_index[pin] == 128)
	{
		Abort("Not a useable pin number: %d", pin);
	}

	GPIO_Pin *gpio = &g_gpio[g_gpio_to_index[pin]];

	if (pwrite(gpio->fd_direction, "out", 3, 0) != 3)
	{
		Abort("Couldn't set GPIO %d direction - %s", pin, strerror(errno));
	}

	char c = '0' + (value);
	if (pwrite(gpio->fd_value, &c, 1, 0) != 1)
	{
		Abort("Couldn't read GPIO %d value - %s", pin, strerror(errno));
	}

}

/** 
 * Read from a GPIO Pin
 * @param pin - The pin to read
 */
bool GPIO_Read(int pin)
{
	if (pin < 0 || pin >= GPIO_INDEX_SIZE || g_gpio_to_index[pin] == 128)
	{
		Log(LOGERR, "Not a useable pin number: %d", pin);
		return false;
	}

	GPIO_Pin *gpio = &g_gpio[g_gpio_to_index[pin]];

	if (pwrite(gpio->fd_direction, "in", 2, 0) != 2)
		Log(LOGERR,"Couldn't set GPIO %d direction - %s", pin, strerror(errno)); 
	char c = '0';
	if (pread(gpio->fd_value, &c, 1, 0) != 1)
		Log(LOGERR,"Couldn't read GPIO %d value - %s", pin, strerror(errno));

	return (c == '1');

}

/**
 * Activate a PWM pin
 * @param pin - The sysfs pin number
 * @param polarity - if true, pin is active high, else active low
 * @param period - The period in ns
 * @param duty - The time the pin is active in ns
 */
void PWM_Set(int pin, bool polarity, long period, long duty)
{
	Log(LOGDEBUG, "Pin %d, pol %d, period: %lu, duty: %lu", pin, polarity, period, duty);
	
	if (pin < 0 || pin >= PWM_NUM_PINS)
	{
		Abort("Invalid PWM pin number %d specified.", pin);
	}

	PWM_Pin *pwm = &g_pwm[pin];

	// Have to stop PWM before changing it
	if (pwrite(pwm->fd_run, "0", 1, 0) != 1)
	{
		Abort("Couldn't stop PWM%d - %s", pin, strerror(errno));
	}

	char c = polarity ? '1' : '0';
	if (pwrite(pwm->fd_polarity, &c, 1, 0) != 1)
	{
		Abort("Couldn't set PWM%d polarity - %s", pin, strerror(errno));
	}

	if (fprintf(pwm->file_period, "%lu", period) < 0)
	{
		Abort("Couldn't set period for PWM%d - %s", pin, strerror(errno));
	}

	if (fprintf(pwm->file_duty, "%lu", duty) < 0)
	{
		Abort("Couldn't set duty cycle for PWM%d - %s", pin, strerror(errno));
	}

	if (pwrite(pwm->fd_run, "1", 1, 0) != 1)
	{
		Abort("Couldn't start PWM%d - %s", pin, strerror(errno));
	}
}

/**
 * Deactivate a PWM pin
 * @param pin - The syfs pin number
 */
void PWM_Stop(int pin)
{
	if (pin < 0 || pin >= PWM_NUM_PINS)
	{
		Abort("Invalid PWM pin number %d specified.", pin);
	}

	if (pwrite(g_pwm[pin].fd_run, "0", 1, 0) != 1)
	{
		Abort("Couldn't stop PWM %d - %s", pin, strerror(errno));
	}
}

/**
 * Read an ADC value
 * @param id - The ID of the ADC pin to read
 * @returns - The reading of the ADC channel
 */
int ADC_Read(int id)
{
	char adc_str[ADC_DIGITS] = {0};

	if (pread(g_adc[id].fd_value, adc_str, ADC_DIGITS-1, 0) == -1)
	{
		Log(LOGERR, "ADC %d read failed: %s", id, strerror(errno));
		return 0;
	}

	return strtol(adc_str, NULL, 10);
}