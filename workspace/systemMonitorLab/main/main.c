/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
#include "transmission.h"
#include "sensorBMP280.h"
#include "configRTC.h"

// Tratar de aplicar esto en un ESP32
void app_main()
{
	i2c_example_master_init();
	i2c_init_mutex();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    Se inicia la conexion Wifi a partir de example connect en C:\esp\ESP8266_RTOS_SDK\examples\common_components\protocol_examples_common
    ESP_ERROR_CHECK(example_connect());

    // Inicializa la cola utilizada para revisar el estado de de la conexi�n con el servidor WEBsocket
    connectionInfoQueue = xQueueCreate(1, sizeof(connectionInfo)); // Cola para un solo entero
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    xTaskCreate(keep_alive_task, "keep_alive_task", 2048, NULL, 4, NULL);
    xTaskCreate(aht10_task, "aht10", 2048, NULL, 7, NULL);
	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 8, NULL );
	xTaskCreate( bmp280_task, "bmp280", 2048, NULL, 4, NULL );
	xTaskCreate(sntp_set_rtc_task, "sntp_set_rtc_task", 2048, NULL, 10, NULL);
}
