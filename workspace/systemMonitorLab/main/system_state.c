#include "system_state.h"

QueueHandle_t systemStateQueue;

/* Inicializa la cola donde los módulos publican el estado de conectividad. */
esp_err_t system_state_init(void)
{
    /* Evita crear una segunda cola si la inicialización se llama dos veces. */
    if (systemStateQueue != NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Una cola de longitud uno permite reemplazar el estado anterior. */
    systemStateQueue = xQueueCreate(1, sizeof(system_state_t));
    if (systemStateQueue == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* El sistema comienza sin conexión hasta recibir un evento de IP. */
    system_state_publish(SYSTEM_STATE_NO_WIFI);

    return ESP_OK;
}

/* Escribe el último estado; no bloquea aunque la cola ya esté llena. */
void system_state_publish(system_state_t state)
{
    if (systemStateQueue != NULL) {
        xQueueOverwrite(systemStateQueue, &state);
    }
}
