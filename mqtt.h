/*
 * mqtt.h
 *
 * Sean-Michael Woerner
 * 1001229459
 *
 * 3/17/2021
 *
 * using information on MQTT version 3.1.1 @ https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
 *
 */

#ifndef MQTT_H_
#define MQTT_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Definitions of mqtt variables
//-----------------------------------------------------------------------------

// ***ALL NAMING CONVENTIONS AND VALUES COME FORM MQTT DOCUMENTATION***

// Protocol level flag for version 3.1.1
#define PROTOCOL_LEVEL 0x04

// Sizes
#define MAX_TOPIC_NAME_SIZE     10
#define MAX_MESSAGE_SIZE        60

// Connect Flags (bits 0-7)
#define CLEAN_SESSION 2
#define WILL_FLAG 4
#define WILL_RETAIN 32
#define PASSWORD_FLAG 64
#define USERNAME_FLAG 128

// QOS values
#define QOS0                    0
#define QOS1                    2
#define QOS2                    4

// States for the connection state machine
typedef enum _state
{
    IDLE = 0,
    SEND_ARP = 1,
    RECV_ARP = 2,
    SEND_SYN = 3,
    RECV_SYN_ACK = 4,
    FIN_WAIT_1 = 5,
    FIN_WAIT_2 = 6,
    CLOSED = 7,
    // MQTT states
    CONNECT_MQTT = 8,
    CONNACK_MQTT = 9,
    PUBLISH_MQTT = 10,
    PUBLISH_QOS0_MQTT = 11,
    PUBLISH_QOS1_MQTT = 12,
    SUBSCRIBE_MQTT = 13,
    SUBACK_MQTT = 14,
    PINGREQ_MQTT = 15,
    PINGRESP_MQTT = 16,
    DISCONNECT_MQTT = 17
} state;


// Enums following the documentation for MQTT version 3.1.1
typedef enum _packetType
{
    MQTT_CONNECT = 0x10,
    CONNACK = 0x20,
    PUBLISH = 0x30,
    SUBSCRIBE = 0x82,
    SUBACK = 0x90,
    PUBACK = 0x40,
    PINGREQ = 0xC0,
    PINGRESP = 0xD0,
    DISCONNECT = 0xE0
} packetType;

typedef struct _fixedHeader
{
    uint8_t controlHeader;
    uint8_t remainingLength[4];
    uint8_t data[0];
} fixedHeader;


typedef struct _connectVariableHeader
{
    uint16_t length;
    char connectMessage[4];
    uint8_t protocolLevel;
    uint8_t connectFlags;
    uint16_t keepAlive;
    uint8_t data[0];
} connectVariableHeader;


typedef struct _connackVariableHeader
{
    uint8_t connectAcknowledgementFlags;
    uint8_t connectReturnCode;
} connackVariableHeader;






#endif MQTT_H_
