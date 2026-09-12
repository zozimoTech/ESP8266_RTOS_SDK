#include "wifi_manager.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"

#include "system_state.h"

static const char *TAG = "wifi_manager";

/* Marca que no hay red y solicita una nueva conexión después de una pérdida. */
static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    system_state_publish(SYSTEM_STATE_NO_WIFI);
    ESP_LOGW(TAG, "Wi-Fi disconnected, reconnecting");
    ESP_ERROR_CHECK(esp_wifi_connect()); // Inicia la reconexión asíncrona; el evento de IP se activará al obtener IP.
}

/* Marca que la estación ya obtuvo IP y está disponible para el WebSocket. */
static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    system_state_publish(SYSTEM_STATE_WIFI_ONLY);
    ESP_LOGI(TAG, "Wi-Fi connected and IP acquired");
}

/* Configura el modo estación, registra eventos e inicia la conexión asíncrona. */
// Esta funcion realiza la gestion de la conexion Wi-Fi, lo primero que hace es inicializar la interfaz TCP/IP, luego inicializa el driver Wi-Fi, registra los eventos de desconexion y de obtencion de IP, configura el SSID y la contraseña, establece el almacenamiento en RAM, establece el modo STA (modo estación, como un cliente Wi-Fi), configura la interfaz Wi-Fi y finalmente inicia la conexión Wi-Fi.    
esp_err_t wifi_manager_start(void)
{
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = { 0 };

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL));

    strncpy((char *) wifi_config.sta.ssid,
            CONFIG_APP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *) wifi_config.sta.password,
            CONFIG_APP_WIFI_PASSWORD, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "Connecting to Wi-Fi SSID: %s", CONFIG_APP_WIFI_SSID);
    return ESP_OK;
}
