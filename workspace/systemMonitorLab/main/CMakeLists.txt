# Los módulos nuevos implementan el estado global y la salida PWM de los LED.
set(COMPONENT_SRCS "transmission.c" "sensorBMP280.c" "system_state.c" "status_led.c" "wifi_manager.c" "main.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
