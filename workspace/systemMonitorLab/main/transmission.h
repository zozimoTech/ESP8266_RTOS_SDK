#ifndef _TRANSMISSION_H_
#define _TRANSMISSION_H_

/*==================[inclusions]=============================================*/

#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/queue.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "sensorAHT10.h"
#include "sensorDHT22.h"
#include "sensorBMP280.h"
#include "common.h"
#include "lwip/apps/sntp.h"

#ifdef CONFIG_EXAMPLE_IPV4
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#else
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#endif

#define PORT CONFIG_EXAMPLE_PORT
/*==================[macros]=================================================*/



/*==================[typedef]================================================*/

// Define una estructura para el estado de la conexi�n y el socket
typedef struct {
    int ackConnect; // 1: conectado, 0: desconectado
    int socketNumber;     // N�mero del socket o -1 si no hay socket
} connectionInfo;





/*==================[external data declaration]==============================*/

//const char *TAG = "example";
//const char *payload = "Message from ESP32 ";
//const char *payload2[300];
// Declaraci�n externa de la cola
extern QueueHandle_t connectionInfoQueue;

/*==================[external functions declaration]=========================*/
extern void encodeMessage126(uint8_t * buf, uint8_t * message,size_t message_len);
extern void encodeMessage125(uint8_t * buf, uint8_t * message,size_t message_len);
extern bool opTransmitMeasuareWebSocket(char * tableData,connectionInfo * connectionData);
extern void tcp_client_task(void *pvParameters);
extern void keep_alive_task(void *pvParameters);

/*==================[end of file]============================================*/
#endif
