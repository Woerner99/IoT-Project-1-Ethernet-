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


// From 1.5.3 of MQTT document, format strings into UTF-8 for easier processing
void encodeUtf8(void *packet, uint16_t length, char *string)
{
    uint8_t* tmp = (uint8_t*)packet;
    *(tmp++) = (length >> 8) & 0xFF;
    *(tmp++) = length & 0xFF;

    if(length <= 0 || string == 0)
    {
        return;
    }
    while(*string)
    {
        *(tmp++) = *(string++);
    }
}

uint16_t getPayloadSize(etherHeader* ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    tcpHeader* tcp = (tcpHeader*)ip->data;
    uint16_t size = (htons(ip->length) - ((ip->revSize & 0xF) << 2)) - ((htons(tcp->offsetFields) >> 12) << 2);
    return size;
}

// Get the length of the requested string
uint16_t strLen(const char* str)
{
    uint8_t i = 0;
    while(str[i])
    {
        i++;
    }
    return i;
}


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


// From 2.2.3 Remaining Length Section of MQTT documentation.
// encodes the remaining length. After some discussion in the lab, the
// algorithm changed slightly (maybe an error in the documentation? not sure)
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

// Fills an MQTT packet that will be sent using MQTT protocol
void fillMQTTPacket(uint8_t *packet, packetType type, uint16_t *packetLength)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;
    mqttFixedHeader->controlHeader = (uint8_t)type;
    mqttFixedHeader->remainingLength[0] = 0;
    *(packetLength) = 2 + mqttFixedHeader->remainingLength[0];
}

// Fills MQTT packet for connecting to MQTT
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

        // Fill variable header to match MQTT protocol
        *(packetLength) = 2 + remainingLength;
        variableHeader->length = htons(4);
        variableHeader->connectMessage[0] = 'M';
        variableHeader->connectMessage[1] = 'Q';
        variableHeader->connectMessage[2] = 'T';
        variableHeader->connectMessage[3] = 'T';
        variableHeader->protocolLevel = PROTOCOL_LEVEL;
        variableHeader->connectFlags = flags;
        // Keep alive for one minute
        variableHeader->keepAlive = htons(60);

        encodeUtf8(payload, clientIDLength, clientID);
}

// Fills MQTT packet that tries to publish to a topic with data or (payload)
void fillMQTTPublishPacket(uint8_t *packet, char *topic, uint16_t packetID, uint8_t qos, char *payload, uint16_t *packetLength)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;
    mqttFixedHeader->controlHeader = (uint8_t)PUBLISH | qos;

    uint8_t offset = 0;
    uint32_t remainingLength = (sizeof(uint16_t) + strLen(topic) + sizeof(uint16_t) + strLen(payload)) + ((packetID > 0) ? sizeof(uint16_t) : 0);
    remainingLength = encodeMqttRemainingLength(remainingLength, &offset);

    uint8_t i = 0;
    for(i = 0; i < offset; i++)
    {
        mqttFixedHeader->remainingLength[i] = (remainingLength >> (i << 3)) & 0xFF;
    }

    *(packetLength) = 2 + remainingLength;
    encodeUtf8((mqttFixedHeader->remainingLength + offset), strLen(topic), topic);

    uint8_t* tmp = 0;
    if(packetID > 0)
    {
        tmp = (mqttFixedHeader->remainingLength + offset) + sizeof(uint16_t) + strLen(topic);
        encodeUtf8(tmp, packetID, 0);
    }

    // Add payload at end of packet
    tmp = (mqttFixedHeader->remainingLength + offset) + (sizeof(uint16_t) + strLen(topic)) + ((packetID > 0) ? sizeof(uint16_t) : 0);
    encodeUtf8(tmp, strLen(payload), payload);
}

// Fills MQTT packet for subscribing to a topic
void fillMQTTSubscribePacket(uint8_t *packet, uint16_t packetID, char *topic, uint8_t qos, uint16_t *packetLength)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;
    mqttFixedHeader->controlHeader = (uint8_t)SUBSCRIBE;

    uint8_t offset = 0;
    uint32_t remainingLength = sizeof(packetID) + (sizeof(uint16_t) + strLen(topic)) + sizeof(qos);
    *packetLength = 2 + remainingLength;
    remainingLength = encodeMqttRemainingLength(remainingLength, &offset);

    uint8_t i = 0;
    for(i = 0; i < offset; i++)
    {
        mqttFixedHeader->remainingLength[i] = (remainingLength >> (i << 3)) & 0xFF;
    }

    uint8_t* tmp = mqttFixedHeader->remainingLength + offset;
    encodeUtf8(tmp, packetID, 0);
    tmp += sizeof(uint16_t);

    // Add payload at end of packet
    encodeUtf8(tmp, strLen(topic), topic);
    tmp += sizeof(uint16_t) + strLen(topic);
    *tmp = qos;
}

// Gets the SUBACK payload after subscribing
uint8_t getSubackPayload(uint8_t* packet)
{
    fixedHeader* mqttFixedHeader = (fixedHeader*)packet;
    uint8_t* payload = (mqttFixedHeader->remainingLength + 1) + sizeof(uint16_t);
    return (((*payload) >> 2) & 31);
}

// Boolean to detrmine if received packet is a CONNACK after sending MQTT CONNECT
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


