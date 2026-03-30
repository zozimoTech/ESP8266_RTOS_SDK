
# Proyecto: systemMonitorLab

(See the README.md file in the upper level 'examples' directory for more information about examples.)

Este proyecto implementa un sistema de monitoreo ambiental basado en el ESP8266 utilizando el SDK de RTOS. La aplicación recopila datos de múltiples sensores (temperatura, humedad y presión) y los transmite periódicamente a un servidor remoto a través de una conexión WebSocket.

## Características Principales

*   **Múltiples Sensores:** Lee datos de los siguientes sensores:
    *   **DHT22:** Temperatura y humedad.
    *   **AHT10:** Temperatura y humedad (a través de I2C).
    *   **BMP280:** Presión barométrica y temperatura (a través de I2C).
*   **Conectividad WiFi:** Se conecta a una red WiFi para acceder a internet.
*   **Sincronización de Tiempo:** Utiliza el protocolo SNTP para obtener la fecha y hora actual y la ajusta a la zona horaria local (ART).
*   **Transmisión de Datos:** Establece una conexión WebSocket con un servidor para enviar los datos de los sensores en tiempo real.
*   **Multitarea:** Utiliza FreeRTOS para gestionar tareas concurrentes para cada sensor, la transmisión de datos y tareas de mantenimiento como el `keep-alive` de la conexión.
*   **Gestión de Bus I2C:** Emplea un mutex para garantizar un acceso seguro y concurrente al bus I2C por parte de los sensores que lo utilizan (AHT10 y BMP280).

## Detalles de la Implementación

### Servidor WebSocket
La aplicación está configurada para conectarse a un servidor WebSocket en la siguiente dirección:
`ws://sc-web.local/ws/environment-monitoring-system-server/`

### Formato de los Datos
Los datos de los sensores se envían en un payload JSON con el siguiente formato:
`{"message":"M;{NUMERO_DE_NODO};{contador};{fecha};{hora};{temp_dht22};{temp_bmp280};{rh_dht22};{pressure_bmp280};"}`

*   `NUMERO_DE_NODO`: Identificador del dispositivo.
*   `contador`: Contador de mensajes enviados.
*   `fecha` y `hora`: Obtenidas del RTC sincronizado por SNTP.
*   `temp_dht22`, `temp_bmp280`, `rh_dht22`, `pressure_bmp280`: Valores leídos de los sensores.

## Hardware Required

*   Placa de desarrollo basada en ESP8266.
*   Sensor DHT22.
*   Sensor AHT10.
*   Sensor BMP280.

## Configure the project

```
make menuconfig
```

Set following parameter under Serial Flasher Options:

* Set `Default serial port`.

Set following parameters under Example Configuration Options:

* Set `WiFi SSID` of the Router (Access-Point).

* Set `WiFi Password` of the Router (Access-Point).

* Set `IP version` of example to be IPV4 or IPV6.

* Set `IPV4 Address` in case your chose IP version IPV4 above.

* Set `IPV6 Address` in case your chose IP version IPV6 above.

* Set `Port` number that represents remote port the example will connect to.

## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


## Troubleshooting

Start server first, to receive data sent from the client (application).
