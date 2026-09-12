#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Estados que puede mostrar el LED según la conectividad del sistema. */
typedef enum {
    SYSTEM_STATE_NO_WIFI = 0,
    SYSTEM_STATE_WIFI_ONLY,
    SYSTEM_STATE_WEBSOCKET_CONNECTED
} system_state_t;

/* Cola de un elemento: siempre conserva el estado más reciente. */
extern QueueHandle_t systemStateQueue;

/* Crea la cola y registra los eventos de Wi-Fi/IP. */
esp_err_t system_state_init(void);

/* Publica un nuevo estado para que lo procese la tarea de LED. */
void system_state_publish(system_state_t state);

#ifdef __cplusplus
}
#endif

#endif
