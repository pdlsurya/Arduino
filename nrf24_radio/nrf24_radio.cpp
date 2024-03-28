#include <SPI.h>
#include "nrf24_radio.h"
#include <Arduino.h>


void NRF24_RADIO::write_register(uint8_t reg_address, uint8_t* reg_value, uint8_t byte_cnt)
{
	digitalWrite(CS_pin, LOW);
	SPI.transfer(reg_address | W_REGISTER);
	for(uint8_t i=0; i<byte_cnt;i++)
	   SPI.transfer(reg_value[i]);
	digitalWrite(CS_pin, HIGH);
}

void NRF24_RADIO::read_register(uint8_t reg_address, uint8_t* reg_value)
{
	digitalWrite(CS_pin, LOW);
	SPI.transfer(reg_address| R_REGISTER);
	*reg_value=SPI.transfer(0x00);

	digitalWrite(CS_pin, HIGH);
}

void NRF24_RADIO::clear_irq_flags(uint8_t flags)
{
	nrf24_reg.REG_STATUS|=flags;
	write_register(RADIO_STATUS, &nrf24_reg.REG_STATUS,1);
}

uint8_t NRF24_RADIO::read_status() //Read Radio Status register
{
	uint8_t radio_status;
	digitalWrite(CS_pin, LOW);
	radio_status=SPI.transfer(NOP);
    digitalWrite(CS_pin, HIGH);

    return radio_status;
}

bool NRF24_RADIO::nrf24_tx(uint8_t* buffer, uint8_t length)
{
	digitalWrite(CS_pin, LOW);
	SPI.transfer(W_TX_PAYLOAD);

	for(uint8_t i=0;i<length;i++)
	   SPI.transfer(buffer[i]);
	for(uint8_t i=0;i<32-length;i++)
		 SPI.transfer(0x00);
	digitalWrite(CS_pin, HIGH);
    
	digitalWrite(CE_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(CE_pin, LOW);

	while(!((read_status() >>TX_DS) & 0x01))
	{
		
		if((read_status() >> MAX_RT)& 0x01)
		{
			clear_irq_flags(((1<<TX_DS)|(1<<RX_DR)|(1<<MAX_RT)));
			flush_tx();
			flush_rx();
			//lcd.set_cursor(1,0);
			Serial.println("retries over");
			//lcd.write_string("Retry timeout!");
			return false;
		}	

	}
	    clear_irq_flags(((1<<TX_DS)|(1<<RX_DR)|(1<<MAX_RT)));
	   // Serial.println("tx success");
		return true;
}

void NRF24_RADIO::nrf24_tx_no_ack(uint8_t* buffer, uint8_t length)
{
	digitalWrite(CS_pin, LOW);
	SPI.transfer(W_TX_PAYLOAD_NO_ACK);

	for(uint8_t i=0;i<length;i++)
	   SPI.transfer(buffer[i]);
	for(uint8_t i=0;i<32-length;i++)
		 SPI.transfer(0x00);
	digitalWrite(CS_pin, HIGH);
    
	digitalWrite(CE_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(CE_pin, LOW);

     while(!((read_status() >>TX_DS) & 0x01));
	clear_irq_flags(((1<<TX_DS)|(1<<RX_DR)|(1<<MAX_RT)));

}

void NRF24_RADIO::set_arc_count(uint8_t count)
{
	if(count>15)
		 count=15;
	nrf24_reg.REG_SETUP_RETR=count;
	write_register(SETUP_RETR, &nrf24_reg.REG_SETUP_RETR,1);

}

void NRF24_RADIO::set_retransmit_delay(uint8_t level )
{
	 
	nrf24_reg.REG_SETUP_RETR|=level<<4;
	write_register(SETUP_RETR, &nrf24_reg.REG_SETUP_RETR,1);
}

bool NRF24_RADIO::nrf24_available(void)
{
    return nrf24_available(NULL);
}


bool NRF24_RADIO::nrf24_available(uint8_t* pipe)
{
	uint8_t rx_pipe;
	
	if((read_status()>>RX_DR) & 0x01)
	{

		 rx_pipe=(read_status()>>1) & 0x07;
		 if(rx_pipe>5)
		     return false;
		else 
		 {
		 	if(pipe)
		 		*pipe=rx_pipe;
		 	return true;
		  }

	}
	else
	    
		return false;
}

void NRF24_RADIO::nrf24_rx(uint8_t* buffer, uint8_t length)
{

   //Read data bytes equal to lenght of receive buffer and store in buffe  

  	digitalWrite(CS_pin, LOW);
    SPI.transfer(R_RX_PAYLOAD);
   for(uint8_t i=0; i<length;i++)
   	 buffer[i]=SPI.transfer(0xFF);
    //read reamaining bytes. no need to store
   for(uint8_t i=0; i<32-length;i++)
   	   SPI.transfer(0xFF);
   digitalWrite(CS_pin, HIGH);

   clear_irq_flags(_BV(RX_DR));
}


void NRF24_RADIO::set_mode(radio_mode_t mode)
{
	switch(mode)
	{
		case TX_MODE:
		nrf24_reg.REG_CONFIG&=0xFE;
		break;
		
		case RX_MODE:
		digitalWrite(CE_pin, HIGH);
		nrf24_reg.REG_CONFIG|=0X01;
		break;

		default:
         break;
	}
	
	write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);
	
}

void  NRF24_RADIO::power_up()
{
    nrf24_reg.REG_CONFIG|=0x01<<1;
    write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);
    delayMicroseconds(750);
}

void  NRF24_RADIO::config_irq_pin(bool mask)
{
	if(mask)
	{
		nrf24_reg.REG_CONFIG|=0x70;
	}

	write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);
}

void  NRF24_RADIO::enable_crc()
{
	nrf24_reg.REG_CONFIG|=0x01<<3;
	write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);
}


void  NRF24_RADIO::disable_crc()
{
    nrf24_reg.REG_CONFIG &=0xF7;
	write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);
} 

void  NRF24_RADIO::set_address_width(uint8_t width)
{
	if(width==3)
       nrf24_reg.REG_SETUP_AW|=0X01;
    else if ( width ==4)
          nrf24_reg.REG_SETUP_AW|=0X02; 
    else if (width==5) 
    	 nrf24_reg.REG_SETUP_AW|=0X03; 
     else
          nrf24_reg.REG_SETUP_AW|=0X03;

      write_register(SETUP_AW, &nrf24_reg.REG_SETUP_AW,1);
}

void  NRF24_RADIO::set_channel(uint8_t channel)
{

	nrf24_reg.REG_RF_CH=channel;
	if(channel >125)
		nrf24_reg.REG_RF_CH=125;

	write_register(RF_CH, &nrf24_reg.REG_RF_CH,1);
}

void  NRF24_RADIO::set_data_rate(nrf24_data_rate_t data_rate)
{
	switch(data_rate)
	{
		case DR_1MBPS:
		   nrf24_reg.REG_RF_SETUP |=0x00;
		   break;

		case DR_2MBPS:
		   nrf24_reg.REG_RF_SETUP |=0x08;
		   break;
		case DR_250KBPS:
		     nrf24_reg.REG_RF_SETUP |=0x20;
		     break;
		 default:
		   break;    
	}

	write_register(RF_SETUP, &nrf24_reg.REG_RF_SETUP,1);
}

void  NRF24_RADIO::set_tx_power(nrf24_tx_power_t power_level)
{
	switch(power_level)
	{
      case TX_POWER_MAX:
        nrf24_reg.REG_RF_SETUP|=0X06;
        break;

      case TX_POWER_MODERATE:
         nrf24_reg.REG_RF_SETUP|=0x04;
         break;
      case TX_POWER_MIN:
         nrf24_reg.REG_RF_SETUP|=0x00;
         break;
      default:
         break;     
	}
  write_register(RF_SETUP, &nrf24_reg.REG_RF_SETUP,1);

}

void NRF24_RADIO::set_rx_address(uint8_t pipe, uint8_t* address)
{   
	 enable_rx_pipe(pipe);

	for(uint8_t i=0; i<5;i++)
	{
		 nrf24_reg.REG_RX_ADDR_PX[i]=address[i];
	}
   if(pipe>=0 && pipe <=5)
    {
       if(pipe<2)
          write_register(RX_ADDR_P0+pipe, nrf24_reg.REG_RX_ADDR_PX, 5);	
       else 
      	write_register(RX_ADDR_P0+pipe, nrf24_reg.REG_RX_ADDR_PX, 1);
    }

}
void  NRF24_RADIO::set_tx_address(uint8_t* address)
{
	 set_rx_address(0, address); //for receiving ack packets

     for(uint8_t i=0;i<4;i++)
     	nrf24_reg.REG_TX_ADDR[i]=address[i];
    write_register(TX_ADDR, nrf24_reg.REG_TX_ADDR,4);

    
}

void  NRF24_RADIO::enable_dynamic_payload_length()
{
	nrf24_reg.REG_FEATURE|=0x04;
	write_register(FEATURE, &nrf24_reg.REG_FEATURE,1);

	nrf24_reg.REG_DYNPD|=0x3F;
	write_register(DYNPD, &nrf24_reg.REG_DYNPD,1);
}

void NRF24_RADIO::disable_dynamic_payload_length()
{
 nrf24_reg.REG_FEATURE&=0xFB;
 write_register(FEATURE, &nrf24_reg.REG_FEATURE,1);

 nrf24_reg.REG_DYNPD&=0x00;
 write_register(DYNPD, &nrf24_reg.REG_DYNPD,1);
}

 void NRF24_RADIO::set_crc_encoding(uint8_t byte_cnt)
 {
 	if(byte_cnt>0 && byte_cnt<=2)
 	{
 	    nrf24_reg.REG_CONFIG|=(byte_cnt-1)<<2;
 	    write_register(CONFIG, &nrf24_reg.REG_CONFIG,1);

 	}
 	else
 		return;

 }

void NRF24_RADIO::set_payload_size(uint8_t size)
{
nrf24_reg.REG_RX_PW_PX=size;
  for(uint8_t i=0; i<6;i++)
  {
  	write_register(RX_PW_P0+i, &nrf24_reg.REG_RX_PW_PX, 1);
  }

}

void NRF24_RADIO::set_auto_ack(bool set_value)
{
	if(set_value)
	
		nrf24_reg.REG_EN_AA |=0x3F;
	else
		nrf24_reg.REG_EN_AA &=0x00;

	write_register(EN_AA, &nrf24_reg.REG_EN_AA,1);
	
}

void NRF24_RADIO::enable_rx_pipe(uint8_t pipe)
{
   nrf24_reg.REG_EN_RXADDR |=0x01 << pipe;
   write_register(EN_RXADDR, &nrf24_reg.REG_EN_RXADDR,1);

}

void NRF24_RADIO::flush_tx()
{
    digitalWrite(CS_pin, LOW);
	SPI.transfer(FLUSH_TX);
	digitalWrite(CS_pin, HIGH);
}

void NRF24_RADIO::flush_rx()
{
    digitalWrite(CS_pin, LOW);
	SPI.transfer(FLUSH_RX);
	digitalWrite(CS_pin, HIGH);
}


uint8_t NRF24_RADIO::get_rx_payload_size()
{
	uint8_t rx_pl_size;
	read_register(R_RX_PL_WID, &rx_pl_size);
	if(rx_pl_size>32)
		 flush_rx();
	return rx_pl_size;
}

void NRF24_RADIO::enable_dynamic_ack()
{
	nrf24_reg.REG_FEATURE |=0x01;
	write_register(FEATURE, &nrf24_reg.REG_FEATURE,1);
}

void NRF24_RADIO::enable_ack_payload()
{
	nrf24_reg.REG_FEATURE |=0x01<<1;
	write_register(FEATURE, &nrf24_reg.REG_FEATURE,1);
}


void NRF24_RADIO::init()
{
	SPI.begin();
	SPI.setDataMode(SPI_MODE0);
	pinMode(CE_pin, OUTPUT);
	pinMode(CS_pin, OUTPUT);
	digitalWrite(CS_pin, HIGH);
	memset(&nrf24_reg,0,sizeof(nrf24_reg));
    
	delay(5);

	//Default setup
    config_irq_pin(true);
	disable_dynamic_payload_length();
	enable_crc();
	set_crc_encoding(2);
	set_address_width(4);
	set_payload_size(32);
    set_arc_count(0);
    set_auto_ack(false);
    set_retransmit_delay(0);
    set_data_rate(DR_1MBPS);
    enable_rx_pipe(0);
    enable_rx_pipe(1);
	flush_tx();
	flush_rx();
	clear_irq_flags(_BV(TX_DS)|_BV(RX_DR)|_BV(MAX_RT));

	power_up();
}

