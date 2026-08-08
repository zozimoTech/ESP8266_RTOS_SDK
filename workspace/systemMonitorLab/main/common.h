#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "semphr.h"
#include "driver/gpio.h"


#define MAX_PRECISION   (10)
#define I2C_EXAMPLE_MASTER_SCL_IO           2                /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO           14               /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define SAMPLE_RATE_SENSORS  				10				/*In Seconds*/
#define TRANSMISSION_INTERVAL  				60				/*In Seconds*/
#define NUMERO_DE_NODO						0               /* Node sensor number */
#define LOW_LEVEL							0
#define HIGH_LEVEL							1

/*==================[external data declaration]==============================*/
extern char *TAG;
extern char payload[300];
extern char payloadWebSocket[1000];
extern esp_err_t i2c_example_master_init();
extern SemaphoreHandle_t i2c_mutex;

/*==================[function declaration]==============================*/
char* floatToString( float value, char* result, int32_t precision );
extern void i2c_init_mutex();

/*==================[end of file]============================================*/
#endif
