#ifndef NRF24_BLE_H
#define NRF24_BLE_H

#include "nrf24_radio.h"
#include <Arduino.h>
#include "timer-api.h"

#define NRF_TEMPERATURE_SERVICE_UUID		0x1809
#define NRF_BATTERY_SERVICE_UUID			0x180F
#define NRF_DEVICE_INFORMATION_SERVICE_UUID 0x180A
#define NRF_HEART_RATE_SERVICE_UUID         0X180D

typedef struct{
uint8_t header;
uint8_t payload_length;

uint8_t mac_address[6];

uint8_t payload[24];

}adv_pdu_t;

typedef struct 
{
	uint8_t length;
	uint8_t type;
	uint8_t data[];
}adv_data_t;

typedef struct 
{ 	
  uint16_t uuid;
  uint8_t data;
	
}ble_service_data_t;

typedef struct 
{
	uint8_t flags;
	char* adv_name;
	ble_service_data_t* ble_service_data;

}ble_config_t;

typedef enum
{

BLE_MODE_ADVERTISE, BLE_MODE_LISTEN	
}ble_mode_t;

typedef struct 
{
	uint8_t current_freq_index;
	adv_pdu_t pdu_buffer;
	uint8_t pl_size;
	ble_config_t *config_data;	
	ble_mode_t ble_mode;
}nrf24_ble_instance_t;

class NRF24_BLE{
	
public:
	static NRF24_BLE* isr_handler;
	NRF24_BLE(NRF24_RADIO* _radio, ble_mode_t mode)
	{
		radio=_radio;
		ble_instance.current_freq_index=0;
		ble_instance.pl_size=0;	
		isr_handler=this;
		ble_instance.ble_mode=mode;
     }
   static  void invoke_isr();
   void begin(ble_config_t *ble_config_t);
   void update_service_data(ble_service_data_t *service_data);
   void adv_listen();
   bool advertise();
 void hop_channel();
private:
	
 NRF24_RADIO* radio;
 nrf24_ble_instance_t ble_instance;	
 void compute_crc(uint8_t len, uint8_t* res);
 void whiten_data(uint8_t len);
 void assemble_pdu();
 void add_adv_data(uint8_t size, uint8_t type, uint8_t* data);
 void reverse_bit_order(uint8_t length);
 
 void handle_timer_isr();
};

#endif //NRF24_BLE_H