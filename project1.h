#ifndef __PROJECT1__H
#define __PROJECT1__H



#include "tm4c123gh6pm.h"



//-----------------------------------------------------------------------------
// Serial Communication defines and functions
//-----------------------------------------------------------------------------
#define MAX_CHARS 80
#define MAX_FIELDS 6

#define MAX_PACKET_SIZE 1522


typedef struct USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

} USER_DATA;




//-----------------------------------------------------------------------------
// METHODS FOR MAIN
//-----------------------------------------------------------------------------

// Function that returns true if the entered strings are the same and false otherwise
bool strCompare(char *str1, char *str2)
{
    uint8_t count = 0;
    while (str1[count] != '\0' || str2[count] != '\0')
    {
        if (str1[count] != str2[count])
        {
            return false;
        }
        count++;
    }
    return true;
}

void printMenu()
{
    putsUart0("\t\r\n\n=======================================================\t\r\n");
    putsUart0("IoT Project 1: MQTT Client Implementation\t\r\n");
    putsUart0("Author: Sean-Michael Woerner\t\r\n");
    putsUart0("=======================================================\t\r\n");
    putsUart0("for more information type 'help'\t\r\n");
    putsUart0("\t\r\n\n>");                        // Clear line and new line for next cmd
}

void printHelp()
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

}

void clearScreen()
{
    uint8_t skip;
    for(skip = 0; skip<75; skip++)
    {
        // skip some lines on the terminal
        putsUart0("\t\r\n");
    }

}

// Cleans string buffer for next run in terminal interface loop
// prevents entering a wrong cmd that the user did not intend on entering
// such as set cmd---> status cmd being ran on the same entry
void clearBuffer(USER_DATA* data)
{

    uint8_t i = 0;
    for (i=0; i < MAX_CHARS; i++)
    {
        data->buffer[i] = 0;
    }
    for (i=0; i < MAX_FIELDS; i++)
    {
        data->fieldPosition[i] = 0;
        data->fieldType[i] = 0;
    }
        data->fieldCount = 0;


}



// Reboots the system from the Terminal
void reboot()
{

    NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
}



#endif
