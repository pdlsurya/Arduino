#ifndef _NRF24_NETWORK_H_
#define _NRF24_NETWORK_H_

#include <nrf24_radio.h>
#include <Arduino.h>

typedef void(*nw_evt_handler_t)(uint8_t* data, uint8_t length);

typedef enum{
	NW_MSG_DATA=0, NW_MSG_ACK, NW_MSG_PING, NW_MSG_PING_ACK
}nw_msg_type_t;

typedef struct {
	uint16_t to_node;
	uint16_t from_node;
    nw_msg_type_t msg_type;
} network_header_t;

typedef struct {
	network_header_t header;
	uint8_t length;
	uint8_t payload[26];
} network_packet_t;

typedef struct {
	uint16_t node_address;
	nw_evt_handler_t nw_evt_handler;
	network_packet_t tx_packet;
	network_packet_t rx_packet;

} network_instance_t;

enum nw_mode_t{
    NW_MODE_TX, NW_MODE_RX
};





class NRF24_NETWORK{
public:
    NRF24_NETWORK(NRF24_RADIO* _radio){
        radio=_radio;

    }
     
    void begin(uint16_t node_address, nw_evt_handler_t evt_handler);
    bool update();
    bool send(uint16_t dest_node, uint8_t *message, uint8_t length, nw_msg_type_t msg_type);


private:
    uint16_t mask_check = 0xFFFF;
    uint16_t node_mask;
    const  uint8_t address_pool[7] = {0xc3, 0x3c, 0x33, 0xce, 0x3e, 0xe3, 0xec};
    uint8_t node_physical_address[5] = { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC };
    network_instance_t instance;
    NRF24_RADIO *radio;
    
    void set_mode(nw_mode_t mode);
    void physical_address(uint16_t node, uint8_t *result);
    void setup_address();
    bool is_descendent(uint16_t node);
    uint16_t next_hop_node(uint16_t node);
    uint16_t parent_node();
};




#endif //_NRF24_NETWORK_H_