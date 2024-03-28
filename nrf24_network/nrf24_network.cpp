/**
 * @file nrf24_network.cpp
 * @author Surya Poudel
 * @brief Network layer driver for NRF24L01+ module. Implements star topology
 * @version 0.1
 * @date 2022-10-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <string.h>
#include <stdint.h>
#include <Arduino.h>
#include <myOled.h>
#include <nrf24_network.h>

void NRF24_NETWORK::physical_address(uint16_t node, uint8_t *result)
{
	uint8_t count = 0;
	while (node)
	{
		result[count] = address_pool[(node % 8)];
		count++;
		node = node / 8;
	}
}

void NRF24_NETWORK::setup_address()
{
	while (mask_check & instance.node_address)
		mask_check <<= 3;

	node_mask = ~mask_check;

	physical_address(instance.node_address, node_physical_address);
}

bool NRF24_NETWORK::is_descendent(uint16_t node)
{
	return ((node & node_mask) == instance.node_address);
}

uint16_t NRF24_NETWORK::next_hop_node(uint16_t node)
{
	uint16_t direct_child_mask = ~((~node _mask) << 3);
	return node & direct_child_mask;
}

uint16_t NRF24_NETWORK::parent_node()
{
	return ((node_mask >> 3) & instance.node_address);
}

void NRF24_NETWORK::set_mode(nw_mode_t mode)
{
	radio->flush_tx();
	radio->flush_rx();
	switch (mode)
	{
	case NW_MODE_TX:
		radio->set_mode(TX_MODE);
		break;

	case NW_MODE_RX:
		radio->set_mode(RX_MODE);
		break;

	default:
		break;
	}
}

bool NRF24_NETWORK::send(uint16_t dest_node, uint8_t *message, uint8_t length, nw_msg_type_t msg_type)
{

	uint8_t next_hop_physical_address[5] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
	uint16_t next_hop;

	// Set packet header and payload
	instance.tx_packet.header.from_node = instance.node_address;
	instance.tx_packet.header.to_node = dest_node;
	instance.tx_packet.header.msg_type = msg_type;
	instance.tx_packet.length = length;
	memcpy(instance.tx_packet.payload, message, length);

	if (is_descendent(dest_node))
	{
		next_hop = next_hop_node(instance.node_address);
	}
	else
	{
		next_hop = parent_node();
	}
	Serial.print("Next hop: 0");
	Serial.println(next_hop, OCT);
	physical_address(next_hop, next_hop_physical_address);
	Serial.print("Next hop physical address=");
	for (uint8_t i = 0; i < 5; i++)
	{
		Serial.print(next_hop_physical_address[i], HEX);
		Serial.print(" ");
	}
	Serial.println();
	radio->set_tx_address(next_hop_physical_address);

	set_mode(NW_MODE_TX);

	if (radio->nrf24_tx((uint8_t *)&instance.tx_packet, length + 6))
	{
		set_mode(NW_MODE_RX);
		return 1;
	}

	return 0;
}

bool NRF24_NETWORK::update()
{
	if (radio->nrf24_available())
	{
		Serial.println("Network data available");
		memset(&instance.rx_packet, 0, 32);
		radio->nrf24_rx((uint8_t *)&instance.rx_packet, 32);
		// if message is for this node
		if (instance.rx_packet.header.to_node == instance.node_address)
		{

			if (instance.rx_packet.header.msg_type == NW_MSG_DATA)
			{
				Serial.print("Incoming message from node:->0");
				Serial.println(instance.rx_packet.header.from_node, OCT);
				instance.nw_evt_handler(instance.rx_packet.payload, instance.rx_packet.length);
				char ack_msg[] = "";
				if (send(instance.rx_packet.header.from_node, (uint8_t *)ack_msg, 0, NW_MSG_ACK))
				{
					Serial.println("ACK Sent Successfully!");
					return 1;
				}
				else
				{
					Serial.println("ACK failed!");
					return 0;
				}
			}
			else if (instance.rx_packet.header.msg_type == NW_MSG_ACK)
			{
				Serial.print("ACK received from node:-> 0");
				Serial.println(instance.rx_packet.header.from_node, OCT);
			}
			return 1;
		}

		else
		{
			Serial.print("Forwarding Message arrived!:->From->To:");
			Serial.print(instance.rx_packet.header.from_node, OCT);
			Serial.print("->");
			Serial.println(instance.rx_packet.header.to_node, OCT);

			uint16_t next_hop;
			uint8_t next_hop_physical_address[5] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC};

			if (is_descendent(instance.rx_packet.header.to_node))
			{
				next_hop = next_hop_node(instance.rx_packet.header.to_node);
			}
			else
			{
				next_hop = parent_node();
			}

			physical_address(next_hop, next_hop_physical_address);
			radio->set_tx_address(next_hop_physical_address);
			set_mode(NW_MODE_TX);

			if (radio->nrf24_tx((uint8_t *)&instance.rx_packet, instance.rx_packet.length + 6))
			{
				// Tx completed, now switch to rx mode;
				set_mode(NW_MODE_RX);
				return 1;
			}
			return 0;
		}
	}
	return 1;
}

void NRF24_NETWORK::begin(uint16_t node_address, nw_evt_handler_t evt_handler)
{

	instance.nw_evt_handler = evt_handler;
	instance.node_address = node_address;
	setup_address();
	radio->init();
	radio->set_rx_address(1, node_physical_address);
	radio->set_channel(76);
	Serial.print("RX address=");
	for (uint8_t i = 0; i < 5; i++)
	{
		Serial.print(node_physical_address[i], HEX);
		Serial.print(" ");
	}
	set_mode(NW_MODE_RX);
}