#include "nrf24_ble.h"
#include <string.h>
#include <stdint.h>
#include <Arduino.h> 
#include <myOled.h>


const uint8_t channel[3]   = {37,38,39};  // logical BTLE channel number (37-39)
const uint8_t frequency[3] = { 2,26,80};  // physical frequency (2400+x MHz)
const uint8_t access_address[4] ={ 0x71,0x91, 0x7D,0x6B};
NRF24_BLE* NRF24_BLE::isr_handler=0;
uint8_t y;
 uint8_t refresh;
uint8_t device_index;
bool match;
uint8_t device_id[4];

uint32_t get_device_sum(uint8_t* device_name) //Device name digest
{
  return uint32_t(device_name[6]);
}


void NRF24_BLE::reverse_bit_order(uint8_t length)//Reverse bit order of a byte
{

uint8_t *temp=(uint8_t*)&ble_instance.pdu_buffer;

while(length--)
{ 
	uint8_t res=0;
    
       if(*temp &0x80) res|=0x01;
       if(*temp &0x40) res|=0x02;
       if(*temp &0x20) res|=0x04;
       if(*temp &0x10) res|=0x08;
       if(*temp &0x08) res|=0x10;
       if(*temp &0x04) res|=0x20;
       if(*temp &0x02) res|=0x40;
       if(*temp &0x01) res|=0x80;

    *(temp++)=res;

}

}

void NRF24_BLE::add_adv_data(uint8_t data_size, uint8_t type, uint8_t* data)//Add advertisement data into Advertising PDU
{
  adv_data_t* ptr=(adv_data_t*)(ble_instance.pdu_buffer.payload+ble_instance.pl_size);
  ptr->length=data_size+1;
  ptr->type=type;
  for(uint8_t i=0; i<data_size; i++)
  	  ptr->data[i]=data[i];

  ble_instance.pl_size+=data_size+2;
}


void NRF24_BLE::begin(ble_config_t* config_data)
{
  initOled(); 
  radio->init();
  radio->set_address_width(4);
  radio->set_auto_ack(false);
  radio->set_arc_count(0);
  radio->set_retransmit_delay(0);
  radio->set_tx_power(TX_POWER_MAX);
  radio->set_tx_address(access_address);
  radio->set_rx_address(0,access_address);
  radio->disable_crc();
  radio->set_data_rate(DR_1MBPS);
  radio->set_channel(frequency[ble_instance.current_freq_index]);
  radio->power_up();
  timer_init_ISR_10Hz(TIMER_DEFAULT);
  if(config_data!=NULL)
         ble_instance.config_data=config_data;
  
}


void NRF24_BLE::hop_channel() //Frequency hopping spread spectrum technique
{
	ble_instance.current_freq_index++;
	if(ble_instance.current_freq_index>=sizeof(channel))
		ble_instance.current_freq_index=0;
	radio->set_channel(frequency[ble_instance.current_freq_index]);
}

void NRF24_BLE::update_service_data(ble_service_data_t *service_data)
{

	ble_instance.config_data->ble_service_data=service_data;
	
}


void NRF24_BLE::assemble_pdu()
{
	ble_instance.pl_size=0;
 
    //add adv flasgs 
    add_adv_data(1,0x01,&ble_instance.config_data->flags);

    //add complete local name
    add_adv_data(strlen(ble_instance.config_data->adv_name),0x09,ble_instance.config_data->adv_name);
   
    //add service data
    uint8_t* service_data=(uint8_t*)ble_instance.config_data->ble_service_data;
	add_adv_data(3, 0x16, service_data);

	ble_instance.pdu_buffer.mac_address[0]=0xfd;
	ble_instance.pdu_buffer.mac_address[1]=0x5c;
	ble_instance.pdu_buffer.mac_address[2]=0x1a;
	ble_instance.pdu_buffer.mac_address[3]=0xA8;
	ble_instance.pdu_buffer.mac_address[4]=0xef;
	ble_instance.pdu_buffer.mac_address[5]=0xca;
    ble_instance.pl_size+=6;
    
    ble_instance.pdu_buffer.header=0x22;
	ble_instance.pdu_buffer.payload_length=ble_instance.pl_size;

    compute_crc(ble_instance.pl_size+2, ble_instance.pdu_buffer.payload+ble_instance.pl_size-6);
    
    whiten_data(ble_instance.pl_size+5);

    reverse_bit_order(ble_instance.pl_size+5);

}


bool NRF24_BLE::advertise()
{
	assemble_pdu();
	
	if(ble_instance.pl_size>27)
		 return false;

    radio->set_mode(TX_MODE);
    radio->nrf24_tx((uint8_t*)&ble_instance.pdu_buffer, ble_instance.pl_size+5);

     return true;
}


void NRF24_BLE::whiten_data(uint8_t length) //Data whitening
{
	uint8_t *temp= (uint8_t*)&ble_instance.pdu_buffer;
	uint8_t lfsr=channel[ble_instance.current_freq_index];
	 lfsr|=0x40;
	
  while(length--)
   {
      uint8_t res=0;
	  for(uint8_t i=0;i<8;i++)
	   {
		res|=(lfsr&0x01)<<i;

		if(lfsr &0x01)
		{
		   lfsr>>=1;
		   lfsr ^=0x44;

	   }
		else
			lfsr>>=1;
	}

	*(temp++)^=res;
	
 }
}



void NRF24_BLE::compute_crc( uint8_t len, uint8_t* crc_buf) //CRC computation
{
 uint8_t* data = (uint8_t*)&ble_instance.pdu_buffer;
	
	uint32_t crc=0xAAAAAA;

	while(len--)
	{
		uint8_t byte=*(data++);
		for(uint8_t i=0; i<8; i++)
		{
			if((byte & 0x01)^(crc&0x01))
			{

              crc>>=1;
              crc^=0xDA6000;

			}
			else
			crc>>=1;
		byte>>=1;
		}

	}
	uint8_t* ptr=(uint8_t*)&crc;
	for(uint8_t i=0;i<3;i++)
		 crc_buf[i]=ptr[i];
}

void NRF24_BLE::adv_listen() //Listern to advertising packets
{ 
  
	if(!(ble_instance.ble_mode==BLE_MODE_LISTEN))
		 return;



	radio->set_mode(RX_MODE);
    
    uint8_t name_len=0;
    uint8_t payload_size=0;
   if(radio->nrf24_available())
     {
     

       Serial.println();
       Serial.println("BLE device available");
     
     radio->nrf24_rx((uint8_t*)&ble_instance.pdu_buffer, 32);
     reverse_bit_order(32);
     whiten_data(32);
     payload_size=ble_instance.pdu_buffer.payload_length-6;//exclude mac address length for payload
     if(payload_size>21)
     {
     	Serial.println("ERROR:oversized payload");
     	return;
     }

     uint8_t received_crc[3];
     for(uint8_t i=0;i<3;i++)
     	 received_crc[i]=ble_instance.pdu_buffer.payload[payload_size+i];
     uint8_t  crc[3];
     compute_crc(payload_size+8, crc);

     for (uint8_t i = 0; i < 3;i++)
     {
     	if(crc[i]!=received_crc[i])
     	{
     		Serial.println("ERROR:CRC failed");
     	    return;
     	}
       
     }
    Serial.println("CRC OK!");

    Serial.print("MAC address:->");
     for(uint8_t i=0; i<6;i++)
     {
     	Serial.print(ble_instance.pdu_buffer.mac_address[5-i], HEX); 
     	if(i!=5)
     	  Serial.print(":");
     }
     Serial.println();

    for(uint8_t i=0; i<payload_size;i++)
    {
      
    	if(ble_instance.pdu_buffer.payload[i]==0x09)
    	{
        
       
        char dev_name[]="";
    		name_len=ble_instance.pdu_buffer.payload[i-1]-1;
    		Serial.print("Device name:->");
         
         memcpy(dev_name, &ble_instance.pdu_buffer.payload[i+1],name_len);
           Serial.println((char*)dev_name);
          
         for(uint8_t i=0;i<4;i++)
         {
          if(get_device_sum(dev_name)==device_id[i])
          {
             match=true;
             Serial.println("matched");
              break;
          }
           
           
         }

         if(match==false)
         {
          
          Serial.println("not");
          device_id[device_index]=get_device_sum(dev_name);
          device_index++;
          if(device_index>4)
             device_index=0;
          printString(dev_name,0,y,6);
          y+=16;
          Serial.println("refresh");
          
         }

         refresh=refresh+1;
         if(refresh==15)
            {
              Serial.print("refresh");
              refresh=0;
            clearDisplay();
            memset(device_id,0,sizeof(device_id));
              y=0;
              device_index=0;
            }  
         match=false;
            
              memset(dev_name,0,name_len);
               Serial.println(); 

                break;

      }
        if(i==payload_size-1)
        	Serial.println("Name not available");
 
  }
  
 }

}

void NRF24_BLE::handle_timer_isr()
 {
   if(ble_instance.ble_mode == BLE_MODE_ADVERTISE)
   {
     if(!advertise())
 
          Serial.println("NRF24_BLE advertisement failure");
       hop_channel();
   }
}

static void  NRF24_BLE::invoke_isr()
{
	if(isr_handler)
	 
	      isr_handler->handle_timer_isr();
}
void timer_handle_interrupts(int timer){

 	NRF24_BLE::invoke_isr();

 }

 
