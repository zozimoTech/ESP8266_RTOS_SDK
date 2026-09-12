#ifndef STATUS_LED_H
#define STATUS_LED_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Esta función gestiona el estado de los LED de indicación */
void status_led_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
