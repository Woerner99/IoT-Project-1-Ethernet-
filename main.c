/* Main.c for IoT Project 1
 *
 * Sean-Michael Woerner 1001229459
 * CSE 4352-001
 * 02/15/2021

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

Target Platform: EK-TM4C123GXL Evaluation Board
 Target uC:       TM4C123GH6PM
 System Clock:    40 MHz

 Hardware configuration:
  Red LED:
   PF1 drives an NPN transistor that powers the red LED
  Green LED:
   PF3 drives an NPN transistor that powers the green LED
 UART Interface:
   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
   Configured to 115,200 baud, 8N1
 Ethernet Module:
  ENC28J60 Module with HanRun HR911105A 19/15 Ethernet Port
  Pin usage is shown on ethernet.c
 */

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "eeprom.h"
#include "eth0.h"
#include "gpio.h"
#include "mqtt.h"
#include "project1.h"
#include "spi0.h"
#include "uart0.h"
#include "wait.h"

#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3


//-----------------------------------------------------------------------------
// External methods
//-----------------------------------------------------------------------------

// Ethernet methods
extern void initEthernet();
extern void displayConnectionInfo();
extern void etherSetMacAddress(uint8_t mac0, uint8_t mac1, uint8_t mac2, uint8_t mac3, uint8_t mac4, uint8_t mac5);
extern void etherDisableDhcpMode();
extern void etherSetIpAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
extern void etherSetIpGatewayAddress(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
extern void etherInit(uint16_t mode);
extern bool etherIsDataAvailable();
extern bool etherIsOverflow();
extern uint16_t etherGetPacket(etherHeader *ether, uint16_t maxSize);
extern bool etherIsArpRequest(etherHeader *ether);
extern bool etherIsArpResponse(etherHeader *ether);
extern void etherSendArpResponse(etherHeader *ether);
extern void etherSendArpRequest(etherHeader *ether, uint8_t ip[]);
extern bool etherIsUdp(etherHeader *ether);
extern uint8_t* etherGetUdpData(etherHeader *ether);
extern void etherSendUdpResponse(etherHeader *ether, uint8_t* udpData, uint8_t udpSize);
extern bool etherIsIp(etherHeader *ether);
extern bool etherIsIpUnicast(etherHeader *ether);
extern bool etherIsPingRequest(etherHeader *ether);
extern void etherSendPingResponse(etherHeader *ether);
extern socket fillSocket(etherHeader *ether, uint8_t destIP[]);
extern void sendTCP(etherHeader *ether, socket *s, uint16_t flags, uint32_t sequenceNumber, int32_t ackNumber,
             uint8_t *options, uint8_t optionsLength, uint16_t dataLength);
extern bool etherIsTcp(etherHeader* ether);
extern uint16_t htons(uint16_t value);
extern uint32_t htonl(uint32_t value);

// MQTT methods
extern void fillMQTTConnectPacket(uint8_t *packet, uint8_t flags, char *clientID, uint16_t clientIDLength,
                                  uint16_t *packetLength);
extern bool mqttIsConnack(uint8_t *packet);
extern void fillMQTTPacket(uint8_t *packet, packetType type, uint16_t *packetLength);
extern void fillMQTTPublishPacket(uint8_t *packet, char *topic, uint16_t packetID, uint8_t qos, char *payload, uint16_t *packetLength);
extern void printState(state mqttState);


// Terminal Interface Methods
extern void initTerminal();
extern char getcUart0();
extern void getsUart0(USER_DATA* data);
extern void parseFields(USER_DATA* data);
extern bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);
extern int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
extern char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
extern bool kbhitUart0();

// EEPROM methods
extern void initEeprom();
extern void writeEeprom(uint16_t add, uint32_t data);
extern uint32_t readEeprom(uint16_t add);
extern void flashEeprom();
extern void storeIP(bool mqtt, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
extern bool startupCheckIP();
extern bool startupCheckMQTT();
extern void getIPfromEEPROM(bool isMQTT, uint8_t *IP0, uint8_t *IP1,uint8_t *IP2, uint8_t *IP3);

// Wait methods
extern void waitMicrosecond(uint32_t us);

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    //Begin initialization of Hardware
    initTerminal();
    initEeprom();
    initUart0();
    setUart0BaudRate(115200, 40e6);
    initEthernet();

    // Create instance of USER_DATA for Terminal Interface input
    USER_DATA data;
    clearBuffer(&data);

    // Create instances of ether, IP, and TCP headers
    // the IP and TCP headers will be received from MQTT
    uint8_t buffer[MAX_PACKET_SIZE];
    etherHeader *ethData = (etherHeader*) buffer;
    ipHeader *ipReceieved = (ipHeader*)ethData->data;
    tcpHeader *tcpReceived = (tcpHeader*)ipReceieved->data;

    // Create an instance of the socket 's' to later be filled after getting ARP response
    socket s;


//-----------------------------------------------------------------------------
// Read from EEPROM if IP and MQTT addresses are available and set them
//-----------------------------------------------------------------------------
    // ip values returned by EEPROM after found
    uint8_t IP0, IP1, IP2, IP3;

    // connect flag before reading MQTT address from EEPROM
    bool connect = false;

    // Set MAC address
    etherSetMacAddress(2,3,4,5,6,111);
    etherDisableDhcpMode();

    // read from EEPROM and set IP address saved in EEPROM
    if(startupCheckIP())
    {
        getIPfromEEPROM(false, &IP0, &IP1, &IP2, &IP3);
        etherSetIpAddress(IP0, IP1, IP2, IP3);
        //etherSetIpAddress(192, 168, 1, 111); // Default IP address (assigned from class)
        etherSetIpGatewayAddress(IP0, IP1, IP2, IP3);
    }
    else  // if EEPROM empty, write default address and set
    {
        storeIP(false,192, 168, 1, 111);
        etherSetIpAddress(192,168,1,111);
        etherSetIpGatewayAddress(192, 168, 1, 111);
    }

    etherSetIpSubnetMask(255, 255, 255, 0);
    etherInit(ETHER_UNICAST | ETHER_BROADCAST | ETHER_HALFDUPLEX);

    // MQTT address, store in EEPROM as default if nothing is set
    uint8_t destIP[] = {192,168,1,70};
    if(startupCheckMQTT())
    {
        getIPfromEEPROM(true, &IP0, &IP1, &IP2, &IP3);
        destIP[0] = IP0;
        destIP[1] = IP1;
        destIP[2] = IP2;
        destIP[3] = IP3;
        // try to connect since we have MQTT address
        connect = true;
    }
    else
    {
        storeIP(true,destIP[0],destIP[1],destIP[2],destIP[3]);
        // try to connect since we have MQTT address
        connect = true;
    }
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

    // Flash LED
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);



    putsUart0("\t\r\n\n=======================================================\t\r\n");
    putsUart0("IoT Project 1: MQTT Client Implementation\t\r\n");
    putsUart0("Author: Sean-Michael Woerner\t\r\n");
    putsUart0("=======================================================\t\r\n");
    putsUart0("for more information type 'help'\t\r\n");
    putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd

    char *cmd;          // first data field
    char *cmd2;         // second data field
    char *topic;        // topic to subscribe/publish to
    char *data_msg;     // message to be published with given topic

    state mqttState = IDLE;
    uint16_t size = 0;              // size of payload
    uint8_t optionsLength = 4;
    uint8_t options[] = {0x02, 0x04, 0x05, 0xB4, 0x00};

    while(true)
    {

        if(kbhitUart0())
        {
            getsUart0(&data);                        // Get string from user
            cmd = getFieldString(&data, 0);         // save first field as the command just incase (I was getting a bug earlier)


            // if user hit's enter, process command
            if(isEnter)
            {
                // reset flag
                isEnter = false;
                parseFields(&data);                      // Parse the fields from user input
                //putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd

            //-----------------------------------------------------------------------------
            // COMMANDS FOR USER
            //-----------------------------------------------------------------------------
                // "clear": clear the terminal screen
                if(strCompare(cmd, "clear"))
                {
                    clearScreen();
                    clearBuffer(&data);
                }



                if(strCompare(cmd, "connect"))
                {
                    connect = true;
                    clearBuffer(&data);
                }

                if(strCompare(cmd, "disconnect"))
                {
                    mqttState = DISCONNECT_MQTT;
                    clearBuffer(&data);
                }

                if(strCompare(cmd, "flash"))
                {
                    flashEeprom();
                    clearBuffer(&data);
                }

                // "help": list available commands and their functions
                if(strCompare(cmd, "help"))
                {
                    putsUart0("Showing list of available terminal commands:\t\r\n");
                    putsUart0("--------------------------------------------\t\r\n");
                    putsUart0("(1)reboot----------------------Restarts the microcontroller\t\r\n");
                    putsUart0("(2)status----------------------Displays the IP and MQTT address, the MQTT connection state, and the TCP FSM state\t\r\n");
                    putsUart0("(3)set IP <w.x.y.z>------------Sets the IP address and stores this address persistently in EEPROM\t\r\n");
                    putsUart0("(4)set MQTT <w.x.y.z>----------Sets the IP address of the MQTT broker and stores this address persistently in EEPROM\t\r\n");
                    putsUart0("(5)publish <TOPIC> <DATA>------Publishes a topic and associated data to the MQTT broker\t\r\n");
                    putsUart0("(6)subscribe <TOPIC>-----------Subscribes to a topic\t\r\n");
                    putsUart0("(7)unsubscribe <TOPIC>---------Unsubscribes from a topic\t\r\n");
                    putsUart0("(8)connect---------------------Sends a connect message to the MQTT broker\t\r\n");
                    putsUart0("(9)disconnect------------------Disconnects from the MQTT broker\t\r\n");
                    putsUart0("(10)flash----------------------Flash the EEPROM and erase all contents\t\r\n");


                    clearBuffer(&data);
                }


                // publish <TOPIC> <DATA> to MQTT broker
                if(isCommand(&data, "publish",2))
                {
                    topic = getFieldString(&data, 1);
                    data_msg = getFieldString(&data, 2);

                    mqttState = PUBLISH_MQTT;
                    clearBuffer(&data);
                }


                // *FIXME* goes to initial start of program but gets stuck in ResetISR()
                if(strCompare(cmd, "reboot"))
                {
                    putsUart0("\t\r\nRebooting System ...\t\r\n");
                    //reboot();
                    clearBuffer(&data);
                }



                // set IP address or MQTT address and store to EEPROM
                if(isCommand(&data, "set", 5))
                {
                    cmd2 = getFieldString(&data, 1);
                  if(strCompare(cmd2, "IP"))
                  {
                      isMQTT = false;
                      uint8_t ip0 = getFieldInteger(&data, 2);
                      uint8_t ip1 = getFieldInteger(&data, 3);
                      uint8_t ip2 = getFieldInteger(&data, 4);
                      uint8_t ip3 = getFieldInteger(&data, 5);
                      // save to EEPROM here using the 4 ints
                      storeIP(isMQTT,ip0, ip1, ip2, ip3);
                      // set IP address for connection
                      etherSetIpAddress(ip0, ip1, ip2, ip3);
                      etherSetIpGatewayAddress(ip0, ip1, ip2, ip3);
                  }


                  if(strCompare(cmd2, "MQTT"))
                  {
                      isMQTT = true;
                      uint8_t ip0 = getFieldInteger(&data, 2);
                      uint8_t ip1 = getFieldInteger(&data, 3);
                      uint8_t ip2 = getFieldInteger(&data, 4);
                      uint8_t ip3 = getFieldInteger(&data, 5);
                      // save to EEPROM here using the 4 ints
                      storeIP(isMQTT,ip0, ip1, ip2, ip3);
                      destIP[0] = ip0;
                      destIP[1] = ip1;
                      destIP[2] = ip2;
                      destIP[3] = ip3;
                  }

                     clearBuffer(&data);

                }

                // Displays IP and MQTT addresses, MQTT connection state and TCP FSM state
                if(strCompare(cmd, "status"))
                {
                    listCommands();
                    putsUart0("\t\r\n\n");
                    displayConnectionInfo();
                    printState(mqttState);

                    clearBuffer(&data);
                }
                putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd
            }
        }

        // check to see if user wants to connect which will begin traversing through state machine
        if(connect)
        {
            connect = false;            // reset the flag
            mqttState = SEND_ARP;    // begin to connect by sending ARP message to MQTT
        }


        // Process the states for sending info:
        switch(mqttState)
        {
        case SEND_ARP:
            etherSendArpRequest(ethData,destIP);
            mqttState = RECV_ARP;
            break;
        case SEND_SYN:
            sendTCP(ethData, &s, 0x6000|SYN, seqNumber, ackNumber, 0, 0, 0);
            mqttState = RECV_SYN_ACK;
            break;
        case CONNECT_MQTT:
            fillMQTTConnectPacket(tcpReceived->data, CLEAN_SESSION, "test", 4, &size);
            sendTCP(ethData, &s, 0x5000 | PSH | ACK, seqNumber, ackNumber, 0, 0, size);
            mqttState = CONNACK_MQTT;

        case PUBLISH_MQTT:
            fillMQTTPublishPacket(tcpReceived->data, topic, packetID, qos, data_msg,  &size);
            sendTCP(ethData, &s, 0x5000 | PSH | ACK, seqNumber, ackNumber, 0, 0, size);
            switch(qos)
            {
            case QOS0:
                mqttState = PUBLISH_QOS0_MQTT;
                break;
            case QOS1:
                mqttState = PUBLISH_QOS1_MQTT;
                break;
            case QOS2:
                //mqttState = PUBLISH_QOS2_MQTT;
                break;
            }

        case DISCONNECT_MQTT:
            fillMQTTPacket(tcpReceived->data, DISCONNECT, &size);
            sendTCP(ethData, &s, 0x5000 | PSH | FIN | ACK, seqNumber, ackNumber, 0, 0, size);
            mqttState = FIN_WAIT_1;
            break;

        case CLOSED:
            // Reset variable and turn off BLUE LED
            putsUart0("\t\r\nCONNECTION CLOSED!\t\r\n");
            setPinValue(BLUE_LED,0);
            ackNumber = 0;
            seqNumber = 200;
            size = 0;
            connect = false;
            mqttState = IDLE;
            break;
        }

        // Begin to listen for data
        if(etherIsDataAvailable())
        {

            if (etherIsOverflow())
            {
                setPinValue(RED_LED, 1);
                waitMicrosecond(100000);
                setPinValue(RED_LED, 0);
            }


            etherGetPacket(ethData, MAX_PACKET_SIZE);

            if (etherIsArpRequest(ethData))
            {
                etherSendArpResponse(ethData);
            }


            // Process the states for receiving data from MQTT
            switch(mqttState)
            {
            case RECV_ARP:
                if(etherIsArpResponse(ethData))
                {
                    putsUart0("\t\r\nARP response received\t\r\n\n>");
                    // Record info to socket for sending
                    s = fillSocket(ethData,destIP);
                    mqttState = SEND_SYN;
                }
                break;
            case RECV_SYN_ACK:
                if(etherIsIp(ethData) && etherIsTcp(ethData))
                {

                    if((htons(tcpReceived->offsetFields) & SYN) && (htons(tcpReceived->offsetFields) & ACK))
                    {
                        seqNumber++;
                        ackNumber = htonl(tcpReceived->sequenceNumber) + 1;
                        sendTCP(ethData, &s, 0x5000 | ACK, seqNumber, ackNumber, 0, 0, 0);
                        mqttState = CONNECT_MQTT;
                        setPinValue(BLUE_LED, 1);
                        putsUart0("\t\r\nESTABLISHED STATE\t\r\n\n>");
                    }
                }

            case CONNACK_MQTT:
                /*
                if(!mqttIsConnack(tcpReceived->data))
                {
                    putsUart0("\t\r\nCONNACK MQTT error!\t\r\n");
                    mqttState = CONNACK_MQTT;
                }
                */
                seqNumber += size;
                ackNumber = htonl(tcpReceived->sequenceNumber) + 4;
                sendTCP(ethData, &s, 0x5000 | ACK, seqNumber, ackNumber, 0, 0, 0);
                mqttState = IDLE;
                break;

            case PUBLISH_QOS0_MQTT:
                seqNumber += size;
                break;
            case PUBLISH_QOS1_MQTT:
                /*
                if(!mqttIsPuback(receivedTcpHeader->data, packetID))
                  {
                      putsUart0("State: PUBLISH_QOS1_MQTT error\n");
                      mqttState = PUBLISH_QOS1_MQTT;
                  }
                  */
                seqNumber += size;
                ackNumber = htonl(tcpReceived->sequenceNumber) + 4;
                sendTCP(ethData, &s, 0x5000 | ACK, seqNumber, ackNumber, 0, 0, 0);
                mqttState = IDLE;

            case FIN_WAIT_1:
                // Handle IP datagram
               if(etherIsIp(ethData) && etherIsTcp(ethData))
               {
                   // Check if this is the ACK of FIN
                   if(htons(tcpReceived->offsetFields) & ACK)
                   {
                       mqttState = FIN_WAIT_2;
                   }
                   else
                   {
                       putsUart0("\t\r\nFIN_WAIT_1 error! \t\r\n");
                   }
               }
               break;
            case FIN_WAIT_2:
                // Handle IP datagram
                if(etherIsIp(ethData) && etherIsTcp(ethData))
                {
                    // Check if FIN, ACK received
                    if((htons(tcpReceived->offsetFields) & FIN) && (htons(tcpReceived->offsetFields) & ACK))
                    {
                        seqNumber += size + 1;
                        ackNumber = htonl(tcpReceived->sequenceNumber) + 1;
                        sendTCP(ethData, &s, 0x5000 | ACK, seqNumber, ackNumber, 0, 0, 0);
                        waitMicrosecond(200000);
                        mqttState = CLOSED;
                    }
                    else
                    {
                        putsUart0("\t\r\nFIN_WAIT_2 error! \t\r\n");
                    }
                }
                break;






            }















        }










    } // END OF MAIN WHILE LOOP

    while(true);
}










