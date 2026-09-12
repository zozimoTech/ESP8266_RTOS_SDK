#include "sensorAHT10.h"


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

#include "driver/i2c.h"



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

#define AHT10_SENSOR_ADDR                	0x38             /*!< slave address for AHT10 sensor */
#define AHT10_INIT_CMD             			0xE1			 /*!< initialization command for AHT10/AHT15 */
#define AHT10_CMD_START                   	0xAC             /*!< Command to set measure mode */
#define AHT10_NORMAL_CMD           0xA8  //normal cycle mode command, no info in datasheet!!!
#define AHT10_SOFT_RESET_CMD       0xBA  //soft reset command

#define AHT10_INIT_NORMAL_MODE     0x00  //enable normal mode
#define AHT10_INIT_CYCLE_MODE      0x20  //enable cycle mode
#define AHT10_INIT_CMD_MODE        0x40  //enable command mode
#define AHT10_INIT_CAL_ENABLE      0x08  //load factory calibration coeff


#define AHT10_DATA_MEASURMENT_CMD  0x33  //no info in datasheet!!! my guess it is DAC resolution, saw someone send 0x00 instead
#define AHT10_DATA_NOP             0x00  //no info in datasheet!!!


#define AHT10_MEASURMENT_DELAY     80    //at least 75 milliseconds
#define AHT10_POWER_ON_DELAY       40    //at least 20..40 milliseconds
#define AHT10_CMD_DELAY            350   //at least 300 milliseconds, no info in datasheet!!!
#define AHT10_SOFT_RESET_DELAY     20    //less than 20 milliseconds

#define AHT10_FORCE_READ_DATA      true  //force to read data
#define AHT10_USE_READ_DATA        false //force to use data from previous read
#define AHT10_ERROR                0xFF  //returns 255, if communication error is occurred


#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN                        0x1              /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS                       0x0              /*!< I2C master will not check ack from slave */
#define ACK_VAL                             0x0              /*!< I2C ack value */
#define NACK_VAL                            0x1              /*!< I2C nack value */
#define LAST_NACK_VAL                       0x2              /*!< I2C last_nack value */

char temp_string_aht10[10];
char rh_string_aht10[10];

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
static esp_err_t i2c_example_master_aht10_read_raw(i2c_port_t i2c_num, uint8_t *data, size_t data_len){
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, AHT10_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, AHT10_CMD_START, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, AHT10_DATA_MEASURMENT_CMD, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, AHT10_DATA_NOP, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		return ret;
	}
	/*El sensor ATH10 me envia 6 bytes, state-humidityData-humidityData-humidityTemperatura-TemperatureData-TemperatureData*/
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, AHT10_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
	i2c_master_read(cmd, data, data_len, LAST_NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;

}

esp_err_t readTemperature(i2c_port_t i2c_num, float *temperature){
	int ret;
	uint8_t sensor_data[6];
	memset(sensor_data, 0, 6);
	ret = i2c_example_master_aht10_read_raw(i2c_num,sensor_data, 6);
	if (ret != ESP_OK) {
		return ret;
	}
	uint32_t temp = ((uint32_t)(sensor_data[3] & 0x0F) << 16) | ((uint16_t)sensor_data[4] << 8) | sensor_data[5]; //20-bit raw temperature data
//	ESP_LOGI(TAG, "CrudoTemp: %d\n",temp);
	*temperature = (float)temp * 0.000191 - 50;
//Aplico las correcciones obtenidas a partir de la calibraci�n del sensor de temp
	double m = 0.978;
	double b = 0.405;
	*temperature = *temperature * m + b;

	if(*temperature < -40){
		*temperature = -40;
	}else if(*temperature > 85){
		*temperature = 85;
	}



//	ESP_LOGI(TAG, "Temp: %.2f\n",*temperature);
	return ret;

}

esp_err_t readHumidity(i2c_port_t i2c_num, float *rh){
	int ret;
	uint8_t sensor_data[6];
	memset(sensor_data, 0, 6);
	ret = i2c_example_master_aht10_read_raw(i2c_num,sensor_data, 6);
	if (ret != ESP_OK) {
		return ret;
	}
	uint32_t rawData = (((uint32_t)sensor_data[1] << 16) | ((uint16_t)sensor_data[2] << 8) | (sensor_data[3])) >> 4; //20-bit raw humidity data
//	ESP_LOGI(TAG, "CrudoRH: %d\n",rawData);

	*rh= (float)rawData * 0.000095;

//Aplico las correcciones obtenidas a partir de la calibraci�n del sensor de RH
	double m = 1.054;
	double b = -4.818;
	*rh = *rh * m + b;

	if (*rh < 0)   *rh = 0;
	if (*rh > 100) *rh = 100;
//	ESP_LOGI(TAG, "RH: %d\n",*rh);

	return ret;

}

static esp_err_t setNormalMode(i2c_port_t i2c_num){
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, AHT10_NORMAL_CMD, ACK_CHECK_EN);
	i2c_master_write_byte(cmd,AHT10_DATA_NOP, ACK_CHECK_EN);
	i2c_master_write_byte(cmd,AHT10_DATA_NOP, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;

}

static esp_err_t i2c_example_master_aht10_init(i2c_port_t i2c_num)
{
//    vTaskDelay(AHT10_SOFT_RESET_DELAY / portTICK_RATE_MS);
//    i2c_example_master_init();
    setNormalMode(i2c_num);
    return ESP_OK;

}



void aht10_task(void *arg)
{
    float temp;
    float rh;
    int ret1, ret2;
//    i2c_example_master_aht10_init(I2C_EXAMPLE_MASTER_NUM);

    while(1){
    	if (xSemaphoreTake(i2c_mutex, (TickType_t)portMAX_DELAY) == pdTRUE) {
    		i2c_example_master_aht10_init(I2C_EXAMPLE_MASTER_NUM);
			ret1 = readHumidity(I2C_EXAMPLE_MASTER_NUM,&rh);
			floatToString(rh,rh_string_aht10,2);
			ret2 = readTemperature(I2C_EXAMPLE_MASTER_NUM,&temp);
			floatToString(temp,temp_string_aht10,2);
			if (ret1 == ESP_OK && ret2 == ESP_OK) {
				ESP_LOGI(TAG, "*******************\n");
				ESP_LOGI(TAG, "Temp_ATH10: %s",temp_string_aht10);
				ESP_LOGI(TAG, "RH_ATH10: %s",rh_string_aht10);
			} else {
				ESP_LOGE(TAG, "No ack, sensor not connected...skip...\n");
			}
			xSemaphoreGive(i2c_mutex);
			vTaskDelay(1000*CONFIG_APP_SAMPLE_RATE_SENSORS / portTICK_RATE_MS);
    	}

    }

    i2c_driver_delete(I2C_EXAMPLE_MASTER_NUM);
}


