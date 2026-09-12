#include "sensorBMP280.h"


//char payload2[300];
/* I2C example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "sensorBMP280.h"




/**
 * TEST CODE BRIEF
 *
 * This example will show you how to use I2C module by running two tasks on i2c bus:
 *
 * - read external i2c sensor, here we use a MPU6050 sensor for instance.
 * - Use one I2C port(master mode) to read or write the other I2C port(slave mode) on one ESP8266 chip.
 *
 * Pin assignment:
 *
 * - master:
 *    GPIO14 is assigned as the data signal of i2c master port
 *    GPIO2 is assigned as the clock signal of i2c master port
 *
 * Connection:
 *
 * - connect sda/scl of sensor with GPIO14/GPIO2
 * - no need to add external pull-up resistors, driver will enable internal pull-up resistors.
 *
 * Test items:
 *
 * - read the sensor data, if connected.
 */

//#define I2C_EXAMPLE_MASTER_SCL_IO           2                /*!< gpio number for I2C master clock */
//#define I2C_EXAMPLE_MASTER_SDA_IO           14               /*!< gpio number for I2C master data  */
//#define I2C_EXAMPLE_MASTER_NUM              I2C_NUM_0        /*!< I2C port number for master dev */
//#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
//#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0                /*!< I2C master do not need buffer */

#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                        0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                       0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                             0x0              /*!< I2C ack value */
#define NACK_VAL                            0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                       0x2              /*!< I2C last_nack value */

char temp_string_bmp280[10];
char pressure_string_bmp280[10];




esp_err_t get_calib_param(i2c_port_t i2c_num, bmp280_calib_param_t *calib_param)
{
    int ret;
    uint8_t sensor_data[BMP280_CALIB_DATA_SIZE] = { 0 };

    // Se inicializa el buffer en cero para evitar datos residuales.
    memset(sensor_data, 0, BMP280_CALIB_DATA_SIZE);

    // Se crea un comando de enlace para la comunicaci�n I2C.
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Se inicia la comunicaci�n I2C.
    i2c_master_start(cmd);

    // Se env�a la direcci�n del sensor BMP280 con el bit de escritura.
    i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | WRITE_BIT, ACK_CHECK_EN);

    // Se env�a la direcci�n de inicio de los datos de calibraci�n.
    i2c_master_write_byte(cmd, BMP280_DIG_T1_LSB_ADDR, ACK_CHECK_EN);

    // Se detiene la comunicaci�n I2C.
    i2c_master_stop(cmd);

    // Se env�a el comando al bus I2C y se espera la respuesta.
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);

    // Se elimina el comando I2C ya que ya no es necesario.
    i2c_cmd_link_delete(cmd);

    // Se verifica si la comunicaci�n fue exitosa.
    if (ret != ESP_OK) {
        return ret;  // Retorna el c�digo de error si falla la comunicaci�n.
    }

    // Se crea un nuevo comando de enlace para leer los datos.
    cmd = i2c_cmd_link_create();

    // Se inicia la comunicaci�n I2C.
    i2c_master_start(cmd);

    // Se env�a la direcci�n del sensor BMP280 con el bit de lectura.
    i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | READ_BIT, ACK_CHECK_EN);

    // Se leen BMP280_CALIB_DATA_SIZE bytes desde el sensor BMP280.
    i2c_master_read(cmd, sensor_data, BMP280_CALIB_DATA_SIZE, LAST_NACK_VAL);

    // Se detiene la comunicaci�n I2C.
    i2c_master_stop(cmd);

    // Se env�a el comando al bus I2C y se espera la respuesta.
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);

    // Se elimina el comando I2C ya que ya no es necesario.
    i2c_cmd_link_delete(cmd);

    // Se verifica si la lectura fue exitosa.
    if (ret != ESP_OK) {
        return ret;  // Retorna el c�digo de error si falla la comunicaci�n.
    }

    // Se procesan los datos recibidos y se almacenan en la estructura de calibraci�n.
    calib_param->dig_t1 = (uint16_t) (((uint16_t) sensor_data[BMP280_DIG_T1_MSB_POS] << 8) | ((uint16_t) sensor_data[BMP280_DIG_T1_LSB_POS]));
    calib_param->dig_t2 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_T2_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_T2_LSB_POS]));
    calib_param->dig_t3 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_T3_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_T3_LSB_POS]));

    calib_param->dig_p1 = (uint16_t) (((uint16_t) sensor_data[BMP280_DIG_P1_MSB_POS] << 8) | ((uint16_t) sensor_data[BMP280_DIG_P1_LSB_POS]));
    calib_param->dig_p2 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P2_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P2_LSB_POS]));
    calib_param->dig_p3 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P3_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P3_LSB_POS]));
    calib_param->dig_p4 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P4_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P4_LSB_POS]));
    calib_param->dig_p5 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P5_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P5_LSB_POS]));
    calib_param->dig_p6 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P6_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P6_LSB_POS]));
    calib_param->dig_p7 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P7_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P7_LSB_POS]));
    calib_param->dig_p8 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P8_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P8_LSB_POS]));
    calib_param->dig_p9 = (int16_t) (((int16_t) sensor_data[BMP280_DIG_P9_MSB_POS] << 8) | ((int16_t) sensor_data[BMP280_DIG_P9_LSB_POS]));

    return ret;  // Retorna ESP_OK si todo fue exitoso.
}

esp_err_t readTemperatureBmp280(i2c_port_t i2c_num, double *temperature,bmp280_calib_param_t *calib_param){
	int ret;
	uint8_t sensor_data[6];
	memset(sensor_data, 0, 6);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, BMP280_PRES_MSB_ADDR, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		return ret;
	}

	/*El sensor BMP280 me envia 6 bytes, 3 de presion y 3 de temp*/
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, sensor_data, 6, LAST_NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);


//	ret = i2c_example_master_aht10_read_raw(i2c_num,sensor_data, 6);
	if (ret != ESP_OK) {
		return ret;
	}
	int32_t uncomp_temp = (int32_t) ((((int32_t) (sensor_data[3])) << 12) | (((int32_t) (sensor_data[4])) << 4) | (((int32_t) (sensor_data[5])) >> 4));
//	ESP_LOGI(TAG, "CrudoTemp: %d\n",uncomp_temp);
	//obtenemos la temperatura
	double var1, var2;

	var1 = (((double) uncomp_temp) / 16384.0 - ((double) calib_param->dig_t1) / 1024.0) * ((double) calib_param->dig_t2);
	var2 = ((((double) uncomp_temp) / 131072.0 - ((double) calib_param->dig_t1) / 8192.0) * (((double) uncomp_temp) / 131072.0 - ((double) calib_param->dig_t1) / 8192.0)) * ((double) calib_param->dig_t3);
	calib_param->t_fine = (int32_t) (var1 + var2);
//	ESP_LOGI(TAG, "t_fine que asigno a la variable temp ANTES de corregir: %d\n",calib_param->t_fine);
	//Aplico las correcciones obtenidas a partir de la calibraci�n del sensor
	// double m = 0.972;
	// double b = -0.531;
	//Sin correciones
	double m = 1;
	double b = 0;
	*temperature = ((var1 + var2) / 5120.0);
	*temperature = (*temperature * m) + b;
	calib_param->t_fine = (int32_t) (*temperature * 5120);
	if(*temperature < -40){
		*temperature = -40;
	}else if(*temperature > 85){
		*temperature = 85;
	}
//	ESP_LOGI(TAG, "t_fine que asigno a la variable LUEGO de corregir: %d\n",calib_param->t_fine);
//	ESP_LOGI(TAG, "TempActualBMP: %.2f\n",*temperature);
	return ret;

}

esp_err_t readPressureBmp280(i2c_port_t i2c_num, double *pressure, bmp280_calib_param_t *calib_param){
	int ret;
	uint8_t sensor_data[6];
	memset(sensor_data, 0, 6);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, BMP280_PRES_MSB_ADDR, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		return ret;
	}

	/*El sensor BMP280 me envia 6 bytes, 3 de presion y 3 de temp*/
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, sensor_data, 6, LAST_NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);


//	ret = i2c_example_master_aht10_read_raw(i2c_num,sensor_data, 6);
	if (ret != ESP_OK) {
		return ret;
	}
	uint32_t uncomp_pres = (int32_t) ((((uint32_t) (sensor_data[0])) << 12) | (((uint32_t) (sensor_data[1])) << 4) | ((uint32_t) sensor_data[2] >> 4));

//	ESP_LOGI(TAG, "t_fine que le paso a la presion: %d\n",calib_param->t_fine);

//	Obtenemos la presion
	double var1, var2;
	var1 = ((double) calib_param->t_fine / 2.0) - 64000.0;
	var2 = var1 * var1 * ((double) calib_param->dig_p6) / 32768.0;
	var2 = var2 + var1 * ((double) calib_param->dig_p5) * 2.0;
	var2 = (var2 / 4.0) + (((double) calib_param->dig_p4) * 65536.0);
	var1 = (((double)calib_param->dig_p3) * var1 * var1 / 524288.0 + ((double)calib_param->dig_p2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0) * ((double) calib_param->dig_p1);

	*pressure = (1048576.0 - (double)uncomp_pres);

	if (var1 < 0 || var1 > 0)
	{
		*pressure = (*pressure - (var2 / 4096.0)) * 6250.0 / var1;
		var1 = ((double)calib_param->dig_p9) * (*pressure) * (*pressure) / 2147483648.0;
		var2 = (*pressure) * ((double)calib_param->dig_p8) / 32768.0;
		*pressure = *pressure + (var1 + var2 + ((double)calib_param->dig_p7)) / 16.0;
	}
	else
	{
		*pressure = 0;
	}
//	ESP_LOGI(TAG, "Temp: %.2f\n",*pressure);
	return ret;


}

//static esp_err_t i2c_example_master_init()
//{
//    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
//    i2c_config_t conf;
//    conf.mode = I2C_MODE_MASTER;
//    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
//    conf.sda_pullup_en = 1;
//    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
//    conf.scl_pullup_en = 1;
//    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
//    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
//    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
//    return ESP_OK;
//}
static esp_err_t writeCommandToBmp280(i2c_port_t i2c_num, uint8_t reg_address, uint8_t *data, size_t data_len){
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, BMP280_I2C_ADDR_PRIM << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
	i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;

}

static esp_err_t i2c_example_master_bmp280_init(i2c_port_t i2c_num)
{
    uint8_t cmd_data;

    // Se define el comando de reinicio para el sensor BMP280.
    cmd_data = BMP280_SOFT_RESET_CMD;

    // Se env�a el comando de reinicio al sensor en la direcci�n correspondiente.
    ESP_ERROR_CHECK(writeCommandToBmp280(i2c_num, BMP280_SOFT_RESET_ADDR, &cmd_data, 1));

    // Configuraci�n del registro de control de medici�n:
    // - BMP280_OS_2X para temperatura (2x oversampling)
    // - BMP280_OS_16X para presi�n (16x oversampling)
    // - BMP280_NORMAL_MODE para operar en modo normal
    cmd_data = BMP280_NORMAL_MODE | (BMP280_OS_16X << 2) | (BMP280_OS_2X << 5);

    // Se imprime el valor del comando de configuraci�n (para depuraci�n).
    printf("dato %d", cmd_data);

    // Se escribe la configuraci�n en el registro de control de medici�n.
    ESP_ERROR_CHECK(writeCommandToBmp280(i2c_num, BMP280_CTRL_MEAS_ADDR, &cmd_data, 1));

    // Configuraci�n del registro de configuraci�n:
    // - BMP280_SPI3_WIRE_DISABLE para deshabilitar SPI de 3 hilos
    // - BMP280_FILTER_COEFF_2 para aplicar un coeficiente de filtrado de 2
    // - BMP280_ODR_125_MS para establecer una tasa de actualizaci�n de 125 ms
    cmd_data = BMP280_SPI3_WIRE_DISABLE | (BMP280_FILTER_COEFF_2 << 2) | (BMP280_ODR_125_MS << 5);

    // Se escribe la configuraci�n en el registro de configuraci�n del sensor.
    ESP_ERROR_CHECK(writeCommandToBmp280(i2c_num, BMP280_CONFIG_ADDR, &cmd_data, 1));

    return ESP_OK; // Se retorna ESP_OK si la inicializaci�n fue exitosa.
}




void bmp280_task(void *arg)
{
    double temp;
    double pressure;
    bmp280_calib_param_t calib_param;
    int ret1, ret2;
    i2c_example_master_bmp280_init(I2C_EXAMPLE_MASTER_NUM);
    get_calib_param(I2C_EXAMPLE_MASTER_NUM,&calib_param);
    while(1){
    	if (xSemaphoreTake(i2c_mutex, (TickType_t)portMAX_DELAY) == pdTRUE){
//			i2c_example_master_bmp280_init(I2C_EXAMPLE_MASTER_NUM);
//			get_calib_param(I2C_EXAMPLE_MASTER_NUM,&calib_param);
	//    	Siempre para la presion primero se lee la temperatura, luego la presion
			ret2 = readTemperatureBmp280(I2C_EXAMPLE_MASTER_NUM,&temp,&calib_param);
			floatToString((float)temp,temp_string_bmp280,2);

			ret1 = readPressureBmp280(I2C_EXAMPLE_MASTER_NUM,&pressure,&calib_param);
//			agregue un 1hpa a la presion corregida en temperatura
			// floatToString((float)((pressure/100)+1),pressure_string_bmp280,2);
			//Sin correciones para la presion
			floatToString((float)((pressure/100)),pressure_string_bmp280,2);
			if (ret1 == ESP_OK && ret2 == ESP_OK) {
				ESP_LOGI(TAG, "*******************\n");
				ESP_LOGI(TAG, "Temp_BMP280: %s",temp_string_bmp280);
				ESP_LOGI(TAG, "Press_BMP280: %s",pressure_string_bmp280);
			} else {
				ESP_LOGE(TAG, "No ack, sensor not connected...skip...\n");
			}
			xSemaphoreGive(i2c_mutex);
			vTaskDelay(1000*CONFIG_APP_SAMPLE_RATE_SENSORS / portTICK_RATE_MS);
    	}

    }

    i2c_driver_delete(I2C_EXAMPLE_MASTER_NUM);
}


