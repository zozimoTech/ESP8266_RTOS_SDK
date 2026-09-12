#include "status_led.h"

#include "driver/pwm.h"
#include "esp_log.h"

#include "system_state.h"

#define STATUS_LED_PWM_PERIOD_US 1000

#if CONFIG_APP_STATUS_LED_ENABLE

static const char *TAG = "status_led";

/* Convierte el estado lógico en duty PWM para los canales rojo y verde. */
static void apply_state(system_state_t state)
{
    uint32_t duties[2];

    switch (state) {
        /* Rojo al 100%, verde apagado: no hay Wi-Fi. */
        case SYSTEM_STATE_NO_WIFI:
            duties[0] = 1000;
            duties[1] = 0;
            break;

        /* Rojo al 100% y verde parcial: mezcla visual naranja. */
        case SYSTEM_STATE_WIFI_ONLY:
            duties[0] = 1000;
            duties[1] = CONFIG_APP_LED_ORANGE_GREEN_DUTY * 10; // El valor de dutty sera de 0 a 100 en la configuracion de menuconfig, por eso multiplicamos por 10 para que sea de 0 a 1000
            break;

        /* Verde al 100%, rojo apagado: WebSocket conectado. */
        case SYSTEM_STATE_WEBSOCKET_CONNECTED:
            duties[0] = 0;
            duties[1] = 1000;
            break;

        default:
            duties[0] = 0;
            duties[1] = 0;
            break;
    }
    
    ESP_ERROR_CHECK(pwm_set_duties(duties)); // Actualiza los duty cycles de los canales PWM
    ESP_ERROR_CHECK(pwm_start()); // Inicia el PWM con los nuevos duty cycles
}

/* Tarea que espera eventos de estado y actualiza el PWM sin hacer polling. */
void status_led_task(void *pvParameters)
{
    /* Canal PWM 0 controla rojo y canal PWM 1 controla verde. */
    const uint32_t pins[2] = {
        CONFIG_APP_LED_RED_GPIO,
        CONFIG_APP_LED_GREEN_GPIO
    };
    uint32_t initial_duties[2] = { 1000, 0 };
    float initial_phases[2] = { 0.0f, 0.0f };
    system_state_t state;

    /* Inicializa ambos canales con un periodo de 1 kHz. */
    ESP_ERROR_CHECK(pwm_init(
        STATUS_LED_PWM_PERIOD_US, initial_duties, 2, pins));
    ESP_ERROR_CHECK(pwm_set_phases(initial_phases));
    ESP_ERROR_CHECK(pwm_start());

    /* portMAX_DELAY deja la tarea dormida hasta que llegue un nuevo estado. */
    while (1) {
        if (xQueueReceive(systemStateQueue, &state, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Connectivity state changed: %d", state);
            apply_state(state);
        }
    }
}

#else

/* Si los LED están deshabilitados, la tarea termina inmediatamente. */
void status_led_task(void *pvParameters)
{
    (void) pvParameters;
    vTaskDelete(NULL);
}

#endif
