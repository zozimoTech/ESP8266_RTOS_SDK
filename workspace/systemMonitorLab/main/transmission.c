#include "transmission.h"
#include "lwip/apps/sntp.h"
#include "driver/gpio.h"
#include "configRTC.h"
#include "system_state.h"

static const gpio_num_t led_yellow = GPIO_NUM_5;
// Definicion de la cola (NO inicializar aqui!)
QueueHandle_t connectionInfoQueue;
QueueHandle_t keepAliveControlQueue;
char payload[300];

static void publish_connection_state(const connectionInfo *connectionData)
{
	// Conserva el estado actual para que websocket_client_task pueda consultarlo.
	xQueueOverwrite(connectionInfoQueue, connectionData);
	// Publica el cambio para despertar o detener keep_alive_task.
	xQueueOverwrite(keepAliveControlQueue, connectionData);
}

void websocket_client_task(void *pvParameters)
{
//    char rx_buffer[128];
    char addr_str[128]; // Buffer para almacenar la direccion IP del servidor
    int addr_family; // Variable para almacenar la familia de direcciones (IPv4 o IPv6)
    int ip_protocol; // Variable para almacenar el protocolo de IP
    connectionInfo connectionData; // Variable para la estructura
    connectionData.socketNumber= -1; // Inicializa el socketNumber en -1 para indicar que no hay socket
    connectionData.ackConnect = 0; // Inicializa ackConnect en 0 para indicar que no hay conexion
	int err =0;
    uint32_t counter = 0;
    time_t now;
    struct tm timeinfo;
   	char fecha[] = "15-01-2025";
	char hora[] = "10:10:00";

	#ifdef CONFIG_APP_SERVER_IPV4
		struct sockaddr_in destAddr;
		destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
		destAddr.sin_family = AF_INET;
		destAddr.sin_port = htons(PORT);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_TCP;
		inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
    #else // IPV6
		struct sockaddr_in6 destAddr;
		inet6_aton(HOST_IP_ADDR, &destAddr.sin6_addr);
		destAddr.sin6_family = AF_INET6;
		destAddr.sin6_port = htons(PORT);
		destAddr.sin6_scope_id = tcpip_adapter_get_netif_index(TCPIP_ADAPTER_IF_STA);
		addr_family = AF_INET6;
		ip_protocol = IPPROTO_IPV6;
		inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
    #endif
	// Configurar solamente el LED amarillo de actividad.
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	// Los LED rojo y verde son controlados exclusivamente por PWM.
	io_conf.pin_bit_mask = (1ULL << led_yellow);
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	// Obtener el tiempo actual
	time(&now);
	localtime_r(&now, &timeinfo);
	// Calcular cuantos segundos faltan para el proximo intervalo
	int seconds_until_next_interval = CONFIG_APP_TRANSMISSION_RATE - (timeinfo.tm_sec % CONFIG_APP_TRANSMISSION_RATE);
	// Esperar hasta el proximo intervalo para la primera transmision de datos.
	ESP_LOGI(TAG, "Esperando %d segundos para comenzar en el proximo intervalo...", seconds_until_next_interval);
	vTaskDelay(seconds_until_next_interval * 1000 / portTICK_PERIOD_MS);


    while (1) {

	    // Lee el ultimo estado sin retirarlo de la cola, para que keep_alive_task tambien pueda verlo.
	    xQueuePeek(connectionInfoQueue, &connectionData, 0);
    	if(connectionData.ackConnect != 1){

//    		CAMBIAR DE COLOR LED DUAL EN FUNCION DEL ESTADO DE CONECTIVIDAD CON EL SERVIDOR WEB SOCKET
			while(1){
				connectionData.socketNumber =  socket(addr_family, SOCK_STREAM, ip_protocol);
				if (connectionData.socketNumber < 0) {
					ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
					system_state_publish(SYSTEM_STATE_WIFI_ONLY);
					break;
				}
				ESP_LOGI(TAG, "-------------------------------------------------------------Socket created");
				err = connect(connectionData.socketNumber, (struct sockaddr *)&destAddr, sizeof(destAddr));
				if (err != 0) {
					ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
					connectionData.ackConnect = 0;
					system_state_publish(SYSTEM_STATE_WIFI_ONLY);
					// Reemplaza el estado anterior para conservar la desconexion mas reciente.
					publish_connection_state(&connectionData);
					close(connectionData.socketNumber);
//					continue;
				}else{
					ESP_LOGI(TAG, "Successfully connected");
					connectionData.ackConnect = 1; // Actualiza el estado de la conexión a conectado
					// char host[] = "10.10.13.138";
					char host[] = "sc-web.local";
					// uint16_t server_port = 8000; //Se usa cuando aplicamos Debug run server
					uint16_t server_port = 80; //Se usa para produccion con nginx y daphne server
					char path[] = "/ws/environment-monitoring-system-server/";
					char key[] = "x3JJHMbDL1EzLkh9GBhXDw==";
					char header[256];
					sprintf(header,	"GET %s HTTP/1.1\r\n"
									"Host: %s:%d\r\n"
									"Upgrade: websocket\r\n"
									"Connection: Upgrade\r\n"
									"Sec-WebSocket-Key: %s\r\n"
									"Sec-WebSocket-Version: 13\r\n"
									"\r\n", path, host, server_port, key);

			
					err = send(connectionData.socketNumber, header, strlen(header), 0);
					if (err < 0) {
						ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
						connectionData.ackConnect = 0; //Ponemos en cero el ack para que intente conectar otra vez
						system_state_publish(SYSTEM_STATE_WIFI_ONLY);
						// Reemplaza el estado anterior para ordenar una futura reconexion.
						publish_connection_state(&connectionData);
						break;
					}else{
						ESP_LOGI(TAG, "Envie correctamente el header de WEB SOCKET\r\n");
						// Despierta el keep-alive solo cuando el handshake fue enviado correctamente.
						publish_connection_state(&connectionData);
						system_state_publish(SYSTEM_STATE_WEBSOCKET_CONNECTED);
					}

					break;
				}
			}
    	}if(connectionData.ackConnect == 1)
		{
			ESP_LOGI(TAG, "Numero de Socket %d", connectionData.socketNumber);

	//		Formato de mensaje
	//		M;numNodo;numMed;fecha;hora;temperature;humidity;pressure;
	//		Ejemplo
	//		M;0;0;15-01-2025;10:10:00;30.23;40.24;1000.23;
	//		Obtengo la hora y la fecha
			time(&now);
			localtime_r(&now, &timeinfo);
	//		strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
			strftime(hora, sizeof(hora), "%H:%M:%S", &timeinfo);
			strftime(fecha, sizeof(fecha), "%d-%m-%Y", &timeinfo);
			sprintf(payload,"------------Datos:\"M;%d;%d;%s;%s;%s;%s;%s;%s;",CONFIG_APP_NUMBER_OF_NODE,counter,fecha,hora,temp_string_dht22,temp_string_bmp280,rh_string_dht22,pressure_string_bmp280);
			ESP_LOGI(TAG,payload);
			sprintf(payload,"{\"message\":\"M;%d;%d;%s;%s;%s;%s;%s;%s;\"}",CONFIG_APP_NUMBER_OF_NODE,counter,fecha,hora,temp_string_dht22,temp_string_bmp280,rh_string_dht22,pressure_string_bmp280);

			if(opTransmitMeasuareWebSocket(payload, &connectionData )==OK){
				ESP_LOGI(TAG, "Pude enviar sin problemas las mediciones");
				/* Dos destellos indican que una medicion fue transmitida. */
				for (int blink = 0; blink < 2; ++blink) {
					gpio_set_level(led_yellow, 1);
					vTaskDelay(100 / portTICK_PERIOD_MS);
					gpio_set_level(led_yellow, 0);
					vTaskDelay(100 / portTICK_PERIOD_MS);
				}
	//    		ESP_LOGI(TAG, "----------------------The current date/time in Buenos Aires is: %s", strftime_buf);

			}else{
				ESP_LOGI(TAG, "No se pudo enviar las mediciones");
			}
			counter = counter + 1;
			// Esperar hasta el pr�ximo intervalo
			// time(&now);
			// localtime_r(&now, &timeinfo);
			// seconds_until_next_interval = CONFIG_APP_TRANSMISSION_RATE - (timeinfo.tm_sec % CONFIG_APP_TRANSMISSION_RATE);
			// ESP_LOGI(TAG, "Segundero %d ", timeinfo.tm_sec );
			// ESP_LOGI(TAG, "Esperando %d segundos para la prxima transmisión...", seconds_until_next_interval);
			
			time(&now);
			localtime_r(&now, &timeinfo);
			
			// // Calcular cuantos segundos faltan para el proximo intervalo de transmision
			int seconds_in_hour = timeinfo.tm_min * 60 + timeinfo.tm_sec; // Segundos transcurridos en la hora actual
			int remainder = seconds_in_hour % CONFIG_APP_TRANSMISSION_RATE; // Segundos transcurridos desde el último intervalo de transmisión
			int seconds_until_next_interval = CONFIG_APP_TRANSMISSION_RATE - remainder; // Segundos restantes hasta el próximo intervalo de transmisión
			ESP_LOGI(TAG, "Segundos transcurridos en la hora actual: %d", seconds_in_hour);
			ESP_LOGI(TAG, "Segundos transcurridos desde el ultimo intervalo de transmision: %d", remainder);
			ESP_LOGI(TAG, "Segundos hasta el proximo intervalo de transmision: %d", seconds_until_next_interval);

			// Metodo mas simple y directo para calcular los segundos hasta el proximo intervalo de transmision
			// int seconds_until_next_interval = CONFIG_APP_TRANSMISSION_RATE - (timeinfo.tm_sec % CONFIG_APP_TRANSMISSION_RATE);
			// ESP_LOGI(TAG, "Segundos hasta el proximo intervalo de transmision: %d", seconds_until_next_interval);

			vTaskDelay(seconds_until_next_interval * 1000 / portTICK_PERIOD_MS);
    	}
//    vTaskDelete(NULL);
	}
}

bool opTransmitMeasuareWebSocket(char * tableData,connectionInfo * connectionData){

	uint32_t len = (uint32_t)strlen(tableData);
	int err;
	char message[600];
	if(len > 125){
		encodeMessage126((uint8_t * )tableData,(uint8_t * )message,sizeof(message));

		err = send(connectionData->socketNumber, message, strlen(tableData)+8, 0);
    	if (err < 0) {
			ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
			connectionData->ackConnect = 0;
			// Publica el fallo para que websocket_client_task intente reconectar.
			publish_connection_state(connectionData);
			system_state_publish(SYSTEM_STATE_WIFI_ONLY);
			return FAIL;
		}

	}else{
		encodeMessage125((uint8_t * )tableData,(uint8_t * )message,sizeof(message));
		err = send(connectionData->socketNumber, message, strlen(tableData)+6, 0);
    	if (err < 0) {
			ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
			connectionData->ackConnect = 0;
			// Publica el fallo para que websocket_client_task intente reconectar.
			publish_connection_state(connectionData);
			system_state_publish(SYSTEM_STATE_WIFI_ONLY);
			return FAIL;
		}
	}
	return OK;/*OK = 0*/

}


// Implementaci�n de la tarea keep_alive_task (como en la respuesta anterior)
void keep_alive_task(void *pvParameters) {
	connectionInfo receivedData = {
		.ackConnect = 0,
		.socketNumber = -1
	};
	int err =0;

    while (1) {
		// Se bloquea hasta que websocket_client_task publique una conexion.
		xQueueReceive(keepAliveControlQueue, &receivedData, portMAX_DELAY);
		while (receivedData.ackConnect == 1) {
			ESP_LOGI(TAG, "Keep Alive: Conexion activa, enviando mensaje de keep alive");
			ESP_LOGI(TAG, "Estado de conexion: %d", receivedData.ackConnect);
			ESP_LOGI(TAG, "Numero de Socket: %d", receivedData.socketNumber);
			// Enviar mensaje de keep-alive
			char aux[] = "{\"message\":\"\"}";
			char message[20];
			encodeMessage125((uint8_t * )aux,(uint8_t * )message,sizeof(message));
//				ESP_LOGI(TAG, "MensajeSINCodificar: %s",aux);
//				ESP_LOGI(TAG, "MensajeCodificado: %s",message);
			err = send(receivedData.socketNumber, message, strlen(aux)+6, 0);
			if (err < 0) {
				ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
				receivedData.ackConnect = 0;
				// Publica el fallo para que websocket_client_task intente reconectar.
				system_state_publish(SYSTEM_STATE_WIFI_ONLY);
				publish_connection_state(&receivedData);
			}

			// Espera un cambio de estado durante un segundo antes del siguiente keep-alive.
			if (xQueueReceive(keepAliveControlQueue, &receivedData,
					pdMS_TO_TICKS(10000)) != pdTRUE) {
				receivedData.ackConnect = 1;
		}
		}
    }
}











void encodeMessage126(uint8_t * buf, uint8_t * message,size_t message_len){
	size_t buf_len = strlen((const char*)buf);
	memset(message, 0, message_len);
	message[0] = 0x81; // Opcode 0x1 y datos enmascarados
	//	uint16_taux = ((uint16_t)strlen(aux)) | 0x8000;
	message[1] = 0xFE;
	uint16_t largo = ((uint16_t)buf_len);
	message[2] = (largo & 0xFF00)>>8;
	message[3] = (largo & 0x00FF)>>0;
	uint32_t mask_key = 0x12345678; // Clave de codificaci�n
	//Copio la clave
	message[4] = (mask_key & 0xFF000000)>>24;	//0x12
	message[5] = (mask_key & 0x00FF0000)>>16;	//0x34
	message[6] = (mask_key & 0x0000FF00)>>8;	//0x56
	message[7] = (mask_key & 0x000000FF)>>0;	//0x78
	//	uint32_t mask_key_aux = (mask_key & 0x00FF0000)>>16;
	memcpy(message + 8, buf, buf_len);
	//	message[6] = message[6]^0x12;
	//	message[7] = message[7]^0x34;
	//	// Copiar los datos sin enmascarar
	int value = 4;
	for (int i = 8; i < buf_len + 8; i++) {
	//		uint8_t a = ((uint8_t*)&mask_key)[i % 4];
	//	    message[i] ^= ((uint8_t*)&mask_key)[i % 4]; // Aplicar XOR con la clave de codificaci�n
		message[i] ^= message[value];
		value++;
		if(value > 7){
			value = 4;
		}
	}




}


void encodeMessage125(uint8_t * buf, uint8_t * message, size_t message_len){
		size_t buf_len = strlen((const char*)buf);
		memset(message, 0, message_len);
		message[0] = 0x81; // Opcode 0x1 y datos enmascarados
		//	uint16_taux = ((uint16_t)strlen(aux)) | 0x8000;
		message[1] = ((uint8_t)buf_len) | 0x80; // Longitud de los datos y seteo el bit de enmascaramiento
		uint32_t mask_key = 0x12345678; // Clave de codificaci�n
		//Copio la clave
		message[2] = (mask_key & 0xFF000000)>>24;	//0x12
		message[3] = (mask_key & 0x00FF0000)>>16;	//0x34
		message[4] = (mask_key & 0x0000FF00)>>8;	//0x56
		message[5] = (mask_key & 0x000000FF)>>0;	//0x78
	//	uint32_t mask_key_aux = (mask_key & 0x00FF0000)>>16;
		memcpy(message + 6, buf, buf_len);
	//	message[6] = message[6]^0x12;
	//	message[7] = message[7]^0x34;
	//	// Copiar los datos sin enmascarar
		int value = 2;
		for (int i = 6; i < buf_len + 6; i++) {
	//		uint8_t a = ((uint8_t*)&mask_key)[i % 4];
	//	    message[i] ^= ((uint8_t*)&mask_key)[i % 4]; // Aplicar XOR con la clave de codificaci�n
			message[i] ^= message[value];
			value++;
			if(value > 5){
				value = 2;
			}
		}


}
