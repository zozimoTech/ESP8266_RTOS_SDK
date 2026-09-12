#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa la interfaz STA y comienza la conexión Wi-Fi. */
esp_err_t wifi_manager_start(void);

#ifdef __cplusplus
}
#endif

#endif
