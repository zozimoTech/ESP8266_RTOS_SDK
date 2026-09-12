
# systemMonitorLab
- Versión README 1.3
- Version Firmware v1.3
- Autor: Ing. Cristian Aranda
- Contacto: zozimotech@gmail.com

# Resumen
Este proyecto implementa un nodo de temperatura, humedad y presión, basado en el ESP8266 y los sensores AHT10, BMP280 y DHT22. Envía las mediciones a un servidor de desarrollo propio, denominado systemMonitorServer, mediante WebSocket. El nodo está diseñado para ser configurado fácilmente a través de un menú de configuración y permite la calibración futura de los sensores. También incluye en hardware y el diseño 3D de una carcasa para proteger el nodo y los sensores.

## 1. Requisitos

Antes de comenzar, debe contar con:

- Una placa ESP8266 compatible con este proyecto.
- Un sensor DHT22.
- Un sensor AHT10.
- Un sensor BMP280.
- Un cable USB de datos para conectar la placa a la computadora.
- El repositorio del ESP8266 RTOS SDK descargado desde ZozimoTech (solicitar acceso a zozimotech@gmail.com)
- El toolchain del ESP8266 instalado y disponible en el entorno.
- Python y las dependencias indicadas por el SDK instaladas.
- Acceso a la red Wi-Fi configurada para el nodo.
- Un servidor WebSocket disponible en la red local con IP privada o en VPS con IP publica.
- Python 3.10.20 instalado.

Este proyecto usa el sistema de compilacion basado en `make`. Los
comandos deben ejecutarse desde la carpeta del proyecto, no desde la raiz del
SDK. Para mas detalle ver [ESP8266_RTOS_SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html).

## 2. Ubicacion del proyecto

El proyecto debe estar dentro del SDK o debe tener correctamente definida la
variable `IDF_PATH`. Una ubicacion recomendada es:

```text
ESP8266_RTOS_SDK/
└── workspace/
    └── systemMonitorLab/
```

Entre a la carpeta del proyecto:

```bash
cd /ruta/al/ESP8266_RTOS_SDK/workspace/systemMonitorLab
```

Compruebe que el SDK este configurado:

```bash
echo $IDF_PATH
```

El resultado debe apuntar a la carpeta del ESP8266 RTOS SDK. Si es necesario,
configure el entorno, adaptando las rutas a su instalacion:

```bash
export IDF_PATH="$HOME/esp/ESP8266_RTOS_SDK"
export PATH="$HOME/esp/xtensa-lx106-elf/bin:$PATH"
```

Si el SDK incluye un script de exportacion, tambien puede ejecutarlo:

```bash
source "$IDF_PATH/export.sh"
```

## 3. Preparar Python y sus dependencias

Este proyecto utiliza Python 3.10.20. El SDK incluye las dependencias
necesarias en:

```text
$IDF_PATH/requirements.txt
```

En Linux es recomendable crear un entorno virtual para no mezclar las
dependencias del SDK con las del sistema. Desde la carpeta del proyecto,
ejecute:

```bash
python3.10 -m venv .venv
source .venv/bin/activate
python --version
python -m pip install --upgrade pip
python -m pip install -r "$IDF_PATH/requirements.txt"
```

La salida de `python --version` debe indicar Python 3.10.20 o una version
compatible con la instalacion del proyecto.

Cada vez que abra una terminal nueva, active el entorno antes de trabajar:

```bash
cd /ruta/al/ESP8266_RTOS_SDK/workspace/systemMonitorLab
source .venv/bin/activate
```

Para salir del entorno virtual:

```bash
deactivate
```

### Hacer que `python` apunte a Python 3

Algunas herramientas del SDK invocan el comando `python`. En Linux puede crear
un alias temporal en `~/.bashrc` o `~/.profile`:

```bash
alias python='python3.10'
```

Si en su sistema el ejecutable se llama `python3` en lugar de `python3.10`,
utilice:

```bash
alias python='python3'
```

Luego cargue nuevamente la configuracion:

```bash
source ~/.bashrc
```

Si necesita que el enlace exista para todo el sistema, puede crear un enlace
simbolico. Esta alternativa modifica `/usr/bin`, por lo que requiere permisos
de administrador y debe usarse solo si no existe ya otro comando `python`:

```bash
sudo ln -s /usr/bin/python3 /usr/bin/python
```

El alias o el enlace solo resuelven el nombre del comando. El entorno virtual
sigue siendo recomendable porque mantiene separadas las versiones de los
paquetes requeridos por el SDK.

## 4. Conectar el nodo por USB

Conecte la placa ESP8266 mediante un cable USB de datos. Identifique el puerto
serie asignado por Linux:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

Los puertos mas habituales son `/dev/ttyUSB0` o `/dev/ttyACM0`. Si no aparece
ningun puerto, revise el cable, el conversor USB-serie y los permisos del
usuario sobre el dispositivo.

## 5. Configurar el firmware

Desde la carpeta `systemMonitorLab`, ejecute:

```bash
make menuconfig
```

En el menu configure los siguientes valores.

### System Monitor Configuration

- `Node number`: numero del nodo, entre `0` y `255`.
- `WiFi SSID`: nombre de la red Wi-Fi.
- `WiFi password`: contraseña de la red Wi-Fi.
- `IP Version`: seleccione `IPV4` o `IPV6`.
- `IPV4 Address`: direccion IP del servidor si eligio IPv4.
- `IPV6 Address`: direccion IPv6 del servidor si eligio IPv6.
- `Port`: puerto TCP del servidor WebSocket.
- `Sample Rate`: intervalo de muestreo de los sensores, en segundos.
- `Transmission Rate`: intervalo de transmision al servidor, en segundos.

El intervalo de muestreo debe ser menor que el intervalo de transmision. Por
ejemplo:

```text
Sample Rate       = 10 segundos
Transmission Rate = 60 segundos
```

### Calibration Parameters

El menu `Calibration Parameters` contiene los valores que se utilizaran en el
futuro para calibrar las mediciones mediante una funcion lineal:

```text
valor_calibrado = pendiente * valor_medido + offset
```

Configure, si corresponde:

- `Pressure offset` y `Pressure slope`.
- `Temperature offset` y `Temperature slope`.
- `Humidity offset` y `Humidity slope`.

En esta version los parametros quedan guardados como configuracion y estan
disponibles como macros `CONFIG_APP_CALIBRATION_*`, pero todavia no se aplican
a las mediciones de los sensores.

### Serial Flasher Options

- Configure `Default serial port` con el puerto detectado, por ejemplo:
  `/dev/ttyUSB0`.
- Verifique la velocidad de monitor serie configurada por el proyecto.

Guarde los cambios y salga de `menuconfig`. La configuracion queda guardada en
el archivo `sdkconfig` del proyecto.

## 6. Compilar el firmware

Compile desde la carpeta del proyecto:

```bash
make
```

Para una compilacion limpia, si fuera necesario:

```bash
make clean
make
```

Si el SDK informa que faltan paquetes de Python, instale las dependencias del
SDK en el entorno correspondiente y vuelva a ejecutar el comando de
compilacion.

## 7. Grabar el ESP8266

Con la placa conectada por USB y el puerto configurado, ejecute:

```bash
make flash
```

Este comando compila lo necesario y graba el firmware en el ESP8266. Si el
puerto no fue configurado en `menuconfig`, puede indicarlo en la linea de
comandos:

```bash
make ESPPORT=/dev/ttyUSB0 flash
```

Reemplace `/dev/ttyUSB0` por el puerto real de su equipo.

## 8. Ver la salida del nodo

Para grabar el firmware y abrir el monitor serie en un solo paso:

```bash
make flash monitor
```

Tambien puede abrir el monitor despues de grabar:

```bash
make monitor
```

Para salir del monitor serie, presione:

```text
Ctrl-]
```

En el monitor deberian observarse mensajes de inicio, conexion Wi-Fi,
conexion WebSocket y transmision de mediciones.

## 9. Funcionamiento del nodo

Al iniciar, el nodo realiza estas tareas:

1. Inicializa los sensores y el bus I2C.
2. Se conecta a la red Wi-Fi configurada.
3. Sincroniza la fecha y hora mediante SNTP.
4. Intenta conectarse al servidor WebSocket.
5. Envia las mediciones segun el intervalo configurado.
6. Envia mensajes keep-alive mientras la conexion WebSocket permanece activa.
7. Intenta reconectar si falla la conexion o una transmision.

El numero de nodo se incluye en los mensajes enviados al servidor. El payload
tiene este formato:

```text
{"message":"M;numero_nodo;contador;fecha;hora;temp_dht22;temp_bmp280;humedad_dht22;presion_bmp280;"}
```

## 10. Indicadores LED

Los LED de estado indican la conectividad del nodo:

- Rojo: el nodo no tiene conexion Wi-Fi.
- Naranja: hay conexion Wi-Fi, pero no hay conexion WebSocket.
- Verde: la conexion WebSocket esta activa.
- Destellos amarillos: se transmitio una medicion.

Los GPIO de los LED y el duty de la mezcla de color pueden configurarse desde
`System Monitor Configuration` si los pines de la placa son diferentes.

## 11. Problemas frecuentes

### No aparece el puerto USB

- Use un cable USB de datos.
- Desconecte y vuelva a conectar la placa.
- Revise `dmesg` y vuelva a listar `/dev/ttyUSB*` y `/dev/ttyACM*`.
- Verifique los permisos del usuario sobre el puerto serie.

### `make menuconfig` no muestra Calibration Parameters

Verifique que existan estos archivos:

```text
components/calibration/CMakeLists.txt
components/calibration/component.mk
components/calibration/Kconfig.projbuild
```

El archivo `component.mk` es necesario porque este SDK utiliza el sistema de
componentes antiguo basado en Make.

### El nodo no conecta al servidor

- Verifique el SSID y la contraseña.
- Verifique la version IP seleccionada.
- Verifique la direccion y el puerto del servidor.
- Confirme que el servidor este iniciado y accesible desde la misma red.
- Revise los mensajes de `make monitor`.

### El build informa dependencias Python faltantes

Ejecute la instalacion de dependencias indicada por el SDK y luego vuelva a
compilar. El comando habitual es:

```bash
python -m pip install -r "$IDF_PATH/requirements.txt"
```

Si utiliza un entorno virtual del SDK, active ese entorno antes de ejecutar el
comando.

## 12. Calibracion futura

Los parametros de offset y pendiente ya pueden configurarse desde
`Calibration Parameters`. La tarea pendiente es aplicar esos valores en las
tareas de lectura del BMP280, AHT10 y DHT22.

La calibracion prevista es:

- Temperatura y humedad en una camara de temperatura y humedad.
- Presion en una camara de presion.
- Obtencion del offset y la pendiente para cada magnitud.
- Carga de los valores en `menuconfig`.
- Recompilacion y nuevo flasheo del nodo.

## 13. Hardware y gabinete

El proyecto incluye el diseño completo del nodo ambiente, separado del codigo
del firmware en las carpetas `hardware/` y `case3d/`.

### Diseño electronico

La carpeta `hardware/` contiene la documentacion y los recursos del hardware
del sistema. Incluye, entre otros elementos:

- `hardware/NodoAmbienteSMN/NodoV1`: primera version del diseño del nodo.
- `hardware/NodoAmbienteSMN/NodoV2`: version actual del diseño electronico.
- `hardware/Documentos`: documentacion, hojas de datos e insumos del sistema.
- `hardware/Pruebas`: resultados y graficos de pruebas de los sensores.
- `hardware/3DCase`: recursos asociados al diseño mecanico del nodo.

El hardware electronico fue desarrollado en KiCad. La version actual del
diseno es la v2.0 y corresponde a un PCB modular, pensado para facilitar la
integracion o el reemplazo de los sensores y perifericos del nodo ambiente.
En la carpeta de la version correspondiente se encuentran los archivos del
esquematico, el PCB y los recursos necesarios para revisar o continuar el
diseno.

### Gabinete 3D

El diseño del gabinete se encuentra en `case3d/` y en los recursos relacionados
dentro de `hardware/3DCase/`. El archivo principal actualmente disponible es:

```text
case3d/caseNodo.scad
```

El gabinete fue desarrollado mediante herramientas CAD, utilizando CadQuery y
FreeCAD para el modelado y la preparacion del diseño mecanico. La version
actual del gabinete es la v1.2.

El gabinete esta pensado para alojar el PCB modular, el ESP8266, los sensores
y sus conexiones, manteniendo accesibles los puntos necesarios para el montaje
y la operacion del nodo.
