/*
 * mqtt.c
 *
 * Sean-Michael Woerner
 * 1001229459
 *
 * 3/17/2021
 *
 * using information on MQTT version 3.1.1 @ https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "eth0.h"
#include "mqtt.h"
#include "uart0.h"


void printState(state currentState)
{
    putsUart0("\t\r\nCurrent State: ");
    switch(currentState)
    {
    case IDLE:
        putsUart0("IDLE\t\r\n");
        break;
    case SEND_ARP:
        putsUart0("SEND_ARP\t\r\n");
          break;
    case RECV_ARP:
         putsUart0("RECV_ARP\t\r\n");
         break;
    case SEND_SYN:
           putsUart0("SEND_SYN\t\r\n");
           break;
    case RECV_SYN_ACK:
           putsUart0("RECV_SYN_ACK\t\r\n");
           break;
    case FIN_WAIT_1:
           putsUart0("FIN_WAIT_1\t\r\n");
           break;
    case FIN_WAIT_2:
           putsUart0("FIN_WAIT_2\t\r\n");
           break;
    case CLOSED:
              putsUart0("CLOSED\t\r\n");
              break;
    case CONNECT_MQTT:
              putsUart0("CONNECT_MQTT\t\r\n");
              break;
    case CONNACK_MQTT:
              putsUart0("CONNACK_MQTT\t\r\n");
              break;
    case PUBLISH_MQTT:
               putsUart0("PUBLISH_MQTT\t\r\n");
               break;
    case PUBLISH_QOS0_MQTT:
               putsUart0("PUBLISH_QOS0_MQTT\t\r\n");
               break;
    case PUBLISH_QOS1_MQTT:
               putsUart0("PUBLISH_QOS1_MQTT\t\r\n");
               break;
    case SUBSCRIBE_MQTT:
               putsUart0("SUBSCRIBE_MQTT\t\r\n");
               break;
    case SUBACK_MQTT:
               putsUart0("SUBACK_MQTT\t\r\n");
               break;
    case PINGREQ_MQTT:
               putsUart0("PINGREQ_MQTT\t\r\n");
               break;
    case PINGRESP_MQTT:
               putsUart0("PINGRESP_MQTT\t\r\n");
               break;
    case DISCONNECT_MQTT:
               putsUart0("DISCONNECT_MQTT\t\r\n");
               break;
    }
}


// Encodes a string as utf-8
void encodeUtf8(void* packet, uint16_t length, char* string)
{
    uint8_t* tmp = (uint8_t*)packet;
    *(tmp++) = (length >> 8) & 0xFF;
    *(tmp++) = length & 0xFF;
    if(length <= 0 || string == 0)
        return;
    while(*string)
        *(tmp++) = *(string++);
}


uint32_t encodeMqttRemainingLength(uint32_t X, uint8_t* offset)
{
    uint32_t encodedByte = 0;
    uint8_t i = 0;
    do
    {
        encodedByte |= (X & 127) << (i << 3);
        X = X / 128;
        if(X > 0)
            encodedByte |= (128 << (i << 3));
        else
        {
            *offset = i + 1;
            return encodedByte;
        }
        i++;
    }
    while(X > 0);
    return encodedByte;
}

void fillMQTTPacket(uint8_t *packet, packetType type, uint16_t *packetLength)
{

}

void fillMQTTConnectPacket(uint8_t *packet, uint8_t flags, char *clientID, uint16_t clientIDLength, uint16_t *packetLength)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;

    uint8_t offset = 0;
    uint32_t remainingLength = encodeMqttRemainingLength((sizeof(connectVariableHeader) + sizeof(clientIDLength) + clientIDLength), &offset);

    connectVariableHeader* variableHeader = (connectVariableHeader*)(mqttFixedHeader->remainingLength + offset);
    uint8_t* payload = (uint8_t*)variableHeader->data;

    mqttFixedHeader->controlHeader = (uint8_t)MQTT_CONNECT;
    uint8_t i = 0;

    for(i = 0; i < offset; i++)
    {
        mqttFixedHeader->remainingLength[i] = (remainingLength >> (i << 3)) & 0xFF;
    }

        *(packetLength) = 2 + remainingLength;
        // As a test, let's only fill up the connect message
        // The length and protocol name are fixed
        variableHeader->length = htons(4);
        variableHeader->connectMessage[0] = 'M';
        variableHeader->connectMessage[1] = 'Q';
        variableHeader->connectMessage[2] = 'T';
        variableHeader->connectMessage[3] = 'T';
        // v3.1.1
        variableHeader->protocolLevel = PROTOCOL_LEVEL;
        variableHeader->connectFlags = flags;
        // For now, a default value of 100s is enough
        variableHeader->keepAlive = htons(100);

        encodeUtf8(payload, clientIDLength, clientID);
}

bool mqttIsConnack(uint8_t *packet)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;
    connackVariableHeader* variableHeader = (connackVariableHeader*)(mqttFixedHeader->remainingLength + 1);
    if(mqttFixedHeader->controlHeader != (uint8_t)CONNACK || mqttFixedHeader->remainingLength[0] != 2)
    {
        return false;
    }
    if(variableHeader->connectAcknowledgementFlags != 0 || variableHeader->connectReturnCode != 0)
    {
        return false;
    }

    return true;
}


