#include "transmission.h"
#include "lwip/apps/sntp.h"
#include "driver/gpio.h"
#include "configRTC.h"

#define GPIO_OUTPUT_LED_1    5
#define GPIO_OUTPUT_LED_2    12
#define GPIO_OUTPUT_LED_3    13
#define GPIO_OUTPUT_LED_4    15
#define GPIO_OUTPUT_PIN_MASK_TO_SET  ((1ULL<<GPIO_OUTPUT_LED_1) | (1ULL<<GPIO_OUTPUT_LED_2) | (1ULL<<GPIO_OUTPUT_LED_3) | (1ULL<<GPIO_OUTPUT_LED_4) )
// Definicion de la cola (NO inicializar aqui!)
QueueHandle_t connectionInfoQueue;
char payload[300];

void tcp_client_task(void *pvParameters)
{
//    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    connectionInfo connectionData; // Variable para la estructura
    connectionData.socketNumber= -1;
    connectionData.ackConnect = 0;
	int err =0;
    uint32_t counter = 0;
    time_t now;
    struct tm timeinfo;
   	char fecha[] = "15-01-2025";
	char hora[] = "10:10:00";
	int cnt = 0;

    #ifdef CONFIG_EXAMPLE_IPV4
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
	// Configurar los Leds de estado
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO15/16
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_MASK_TO_SET;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	// Obtener el tiempo actual
	time(&now);
	localtime_r(&now, &timeinfo);
	// Calcular cu�ntos segundos faltan para el proximo intervalo
	int seconds_until_next_interval = TRANSMISSION_INTERVAL - (timeinfo.tm_sec % TRANSMISSION_INTERVAL);
	// Esperar hasta el proximo intervalo
	ESP_LOGI(TAG, "Esperando %d segundos para comenzar en el proximo intervalo...", seconds_until_next_interval);
	vTaskDelay(seconds_until_next_interval * 1000 / portTICK_PERIOD_MS);


    while (1) {

//    	if (xQueueReceive(connectionInfoQueue, &connectionData, 0) != pdTRUE) {
//    		connectionData.ackConnect = 0; // Si no hay mensaje en la cola, asume desconectado
//        }
    	if(connectionData.ackConnect != 1){

//    		CAMBIAR DE COLOR LED DUAL EN FUNCION DEL ESTADO DE CONECTIVIDAD CON EL SERVIDOR WEB SOCKET
			while(1){
				connectionData.socketNumber =  socket(addr_family, SOCK_STREAM, ip_protocol);
				if (connectionData.socketNumber < 0) {
					ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
					break;
				}
				ESP_LOGI(TAG, "-------------------------------------------------------------Socket created");
				err = connect(connectionData.socketNumber, (struct sockaddr *)&destAddr, sizeof(destAddr));
				if (err != 0) {
					ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
					connectionData.ackConnect = 0;
					xQueueSend(connectionInfoQueue, &connectionData, 0); // Envia el estado a la cola
					close(connectionData.socketNumber);
//					continue;
				}else{
					ESP_LOGI(TAG, "Successfully connected");
					connectionData.ackConnect = 1;
					xQueueSend(connectionInfoQueue,&connectionData, 0); // Envia el estado a la cola
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
						xQueueSend(connectionInfoQueue, &connectionData, 0); // Envia el estado a la cola
						break;
					}else{
						ESP_LOGI(TAG, "Envie correctamente el header de WEB SOCKET\r\n");
					}

					break;
				}
			}
    	}
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
		sprintf(payload,"------------Datos:\"M;%d;%d;%s;%s;%s;%s;%s;%s;",NUMERO_DE_NODO,counter,fecha,hora,temp_string_dht22,temp_string_bmp280,rh_string_dht22,pressure_string_bmp280);
		ESP_LOGI(TAG,payload);
    	sprintf(payload,"{\"message\":\"M;%d;%d;%s;%s;%s;%s;%s;%s;\"}",NUMERO_DE_NODO,counter,fecha,hora,temp_string_dht22,temp_string_bmp280,rh_string_dht22,pressure_string_bmp280);

    	if(opTransmitMeasuareWebSocket(payload, &connectionData )==OK){
    		ESP_LOGI(TAG, "Pude enviar sin problemas las mediciones");
//    		ESP_LOGI(TAG, "----------------------The current date/time in Buenos Aires is: %s", strftime_buf);

    	}else{
    		ESP_LOGI(TAG, "No se pudo enviar las mediciones");
    	}
		counter = counter + 1;
		// Esperar hasta el pr�ximo intervalo
		time(&now);
		localtime_r(&now, &timeinfo);
		seconds_until_next_interval = TRANSMISSION_INTERVAL - (timeinfo.tm_sec % TRANSMISSION_INTERVAL);

		ESP_LOGI(TAG, "cnt: %d\n", cnt++);
		gpio_set_level(GPIO_OUTPUT_LED_1, cnt % 2);
		gpio_set_level(GPIO_OUTPUT_LED_2, cnt % 2);
		gpio_set_level(GPIO_OUTPUT_LED_3, cnt % 2);
		gpio_set_level(GPIO_OUTPUT_LED_4, cnt % 2);
        vTaskDelay(seconds_until_next_interval*1000 / portTICK_PERIOD_MS);
    }
//    vTaskDelete(NULL);
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
			return FAIL;
		}

	}else{
		encodeMessage125((uint8_t * )tableData,(uint8_t * )message,sizeof(message));
		err = send(connectionData->socketNumber, message, strlen(tableData)+6, 0);
    	if (err < 0) {
			ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
			connectionData->ackConnect = 0;
			return FAIL;
		}
	}
	return OK;/*OK = 0*/

}


// Implementaci�n de la tarea keep_alive_task (como en la respuesta anterior)
void keep_alive_task(void *pvParameters) {
    connectionInfo receivedData;
	int err =0;

    while (1) {
    	xQueueReceive(connectionInfoQueue, &receivedData, 0);
//        if (xQueueReceive(connectionInfoQueue, &receivedData, 0) == pdTRUE) {
		if (receivedData.ackConnect == 1) {
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
				break;
			}
			// ... (c�digo para enviar mensaje de keep-alive)
		} else {
			ESP_LOGI(TAG, "Keep Alive: Conexion inactiva");
			ESP_LOGI(TAG, "Estado de conexion, dato de la cola: %d", receivedData.ackConnect);
			// Realizar acciones necesarias si la conexi�n est� inactiva
		}
//        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Ejemplo: revisa cada 5 segundos
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
