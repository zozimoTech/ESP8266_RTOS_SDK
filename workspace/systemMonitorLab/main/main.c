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
#include "esp_event.h"
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
#include "status_led.h"
#include "system_state.h"
#include "wifi_manager.h"

// Tratar de aplicar esto en un ESP32
void app_main()
{
	i2c_example_master_init();
	i2c_init_mutex();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Crea la cola de estados que usan Wi-Fi, WebSocket y los LED. */
    ESP_ERROR_CHECK(system_state_init());

#if CONFIG_APP_STATUS_LED_ENABLE
    /* La tarea queda esperando cambios y controla los dos canales PWM. */
    xTaskCreate(status_led_task, "status_led", 2048, NULL, 6, NULL);
#endif
    /* Inicia Wi-Fi; la conexión continúa de forma asíncrona mediante eventos. */
    ESP_ERROR_CHECK(wifi_manager_start());

    // Inicializa la cola utilizada para revisar el estado de de la conexi�n con el servidor WEBsocket
    // Cola existente usada por las tareas de transmision del WebSocket.
    connectionInfoQueue = xQueueCreate(1, sizeof(connectionInfo));
    // Crea una cola de eventos para despertar el keep-alive solo cuando cambia la conexion.
    keepAliveControlQueue = xQueueCreate(1, sizeof(connectionInfo));
    // Crea el estado inicial que indica que todavia no existe conexion WebSocket.
    connectionInfo initialConnectionData = {
        // ackConnect vale cero hasta completar correctamente la conexion.
        .ackConnect = 0,
        // -1 indica que todavia no hay un socket valido asociado.
        .socketNumber = -1
    };
    // Guarda el estado inicial para que ambas tareas puedan leerlo desde el comienzo.
    xQueueOverwrite(connectionInfoQueue, &initialConnectionData);
    xTaskCreate(websocket_client_task, "websocket_client", 4096, NULL, 5, NULL);
    xTaskCreate(keep_alive_task, "keep_alive_task", 2048, NULL, 4, NULL);
    xTaskCreate(aht10_task, "aht10", 2048, NULL, 7, NULL);
	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 8, NULL );
	xTaskCreate( bmp280_task, "bmp280", 2048, NULL, 4, NULL );
	xTaskCreate(sntp_set_rtc_task, "sntp_set_rtc_task", 2048, NULL, 10, NULL);
}
