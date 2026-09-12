#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "configRTC.h"

#include "lwip/apps/sntp.h"

//Las funciones static solo se pueden usar en este documento y deben ser declaradas antes de llamarlas
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
	// sntp_setservername(0, "69.163.171.181");
	//sntp_setservername(0, "south-america.pool.ntp.org");
	sntp_init();
}

static void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}


void sntp_set_rtc_task(void *arg)
{
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];

//    time(&now);
//    localtime_r(&now, &timeinfo);
////    revisamos que informacion trae de fabrica
//    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//    ESP_LOGI(TAG, "Numero de Socket %d", timeinfo.tm_year);
//
//    // Is time set? If not, tm_year will be (1970 - 1900).
//    if (timeinfo.tm_year < (2016 - 1900)) {
//        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
//        obtain_time();
//    }

    // Set timezone to Eastern Standard Time and print local time
    // setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    // tzset();

    // Set timezone to China Standard Time
    // setenv("TZ", "CST-8", 1);
	// Set timezone to ARGENTINA Standard Time
//    esto puede que se tenga que inicializar despues de obtain_time
    setenv("TZ", "ART+3", 1);
    tzset();
//Podria monitorear a futuro que cuando sea el minuto exacto 15:20:00, ahi realice el envio
    while (1) {
        // update 'now' variable with current time
        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year < (2016 - 1900)) {
//            ESP_LOGE(TAG, "The current date/time error");
            ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
			obtain_time();
        }
//        } else {
//            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//            // ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
////			ESP_LOGI(TAG, "The current date/time in Buenos Aires is: %s", strftime_buf);
//        }

//        ESP_LOGI(TAG, "Free heap size (Cantidad de memoria libre): %d\n", esp_get_free_heap_size());
//        Este podria ser el tiempo de cada cuanto quiero que se resincronice su snpt
        vTaskDelay(5000 / portTICK_RATE_MS);
    }
}

