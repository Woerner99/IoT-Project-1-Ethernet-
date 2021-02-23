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
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "eeprom.h"
#include "gpio.h"
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

// Terminal Interface Methods
extern void initTerminal();
extern char getcUart0();
extern void getsUart0(USER_DATA* data);
extern void parseFields(USER_DATA* data);
extern bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);
extern int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
extern char* getFieldString(USER_DATA* data, uint8_t fieldNumber);

// EEPROM methods
extern void initEeprom();
extern void writeEeprom(uint16_t add, uint32_t data);
extern uint32_t readEeprom(uint16_t add);
extern void flashEeprom();
extern void storeIP(bool mqtt, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3);
extern void startupCheck();


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

    USER_DATA data;
    clearBuffer(&data);

    // Read the EEPROM to check for stored IP or MQTT addresses
    startupCheck();

    putsUart0("=======================================================\t\r\n");
    putsUart0("IoT Project 1: MQTT Client Implementation\t\r\n");
    putsUart0("Author: Sean-Michael Woerner\t\r\n");
    putsUart0("=======================================================\t\r\n");
    putsUart0("for more information type 'help'\t\r\n");


    while(true)
    {

        valid2 = false;                               // make false to check for correct commands
        putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd
        getsUart0(&data);                        // Get string from user
        parseFields(&data);                      // Parse the fields from user input


        //-----------------------------------------------------------------------------
        // COMMANDS FOR USER
        //-----------------------------------------------------------------------------

        // "clear": clear the terminal screen
        if(isCommand(&data, "clear", 0))
        {
            clearScreen();
            valid2 = true;
            clearBuffer(&data);
        }

        if(isCommand(&data, "flash",0))
        {
            flashEeprom();
            valid2 = true;
            clearBuffer(&data);
        }

        // "help": list available commands and their functions
        if(isCommand(&data, "help", 0))
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


            valid2 = true;
            clearBuffer(&data);
        }




        // *FIXME* goes to initial start of program but gets stuck in ResetISR()
        if(isCommand(&data, "reboot", 0))
        {
            putsUart0("\t\r\nRebooting System ...\t\r\n");
            //reboot();
            valid2 = true;
            clearBuffer(&data);
        }



        // set IP address or MQTT address and store to EEPROM
        if(isCommand(&data, "set", 5))
        {
            char *cmd = getFieldString(&data, 1);
          if(strCompare(cmd, "IP"))
          {
              isMQTT = false;
              uint8_t ip0 = getFieldInteger(&data, 2);
              uint8_t ip1 = getFieldInteger(&data, 3);
              uint8_t ip2 = getFieldInteger(&data, 4);
              uint8_t ip3 = getFieldInteger(&data, 5);
              // save to EEPROM here using the 4 ints
              storeIP(isMQTT,ip0, ip1, ip2, ip3);
          }


          if(strCompare(cmd, "MQTT"))
          {
              isMQTT = true;
              uint8_t ip0 = getFieldInteger(&data, 2);
              uint8_t ip1 = getFieldInteger(&data, 3);
              uint8_t ip2 = getFieldInteger(&data, 4);
              uint8_t ip3 = getFieldInteger(&data, 5);
              // save to EEPROM here using the 4 ints
              storeIP(isMQTT,ip0, ip1, ip2, ip3);
          }
             valid2 = true;
             clearBuffer(&data);

        }

        // Displays IP and MQTT addresses, MQTT connection state and TCP FSM state
        if(isCommand(&data, "status",0))
        {
            listCommands();
            valid2 = true;
            clearBuffer(&data);
        }



    } // END OF MAIN WHILE LOOP

    while(true);
}









