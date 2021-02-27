// EEPROM functions
// Sean-Michael Woerner


//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "eeprom.h"
#include "gpio.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "wait.h"


uint8_t currentStatus;
// Variables for storing in EEPROM
uint8_t macAddressEEPROM[HW_ADD_LENGTH] = {2,3,4,5,6,7};
uint8_t ipAddressEEPROM[IP_ADD_LENGTH] = {0,0,0,0};
bool isIPset = false;
bool isMQTTset = false;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------


void initEeprom()
{
    SYSCTL_RCGCEEPROM_R = 1;
    _delay_cycles(3);
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

void writeEeprom(uint16_t add, uint32_t data)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    EEPROM_EERDWR_R = data;
    while (EEPROM_EEDONE_R & EEPROM_EEDONE_WORKING);
}

uint32_t readEeprom(uint16_t add)
{
    EEPROM_EEBLOCK_R = add >> 4;
    EEPROM_EEOFFSET_R = add & 0xF;
    return EEPROM_EERDWR_R;
}

// Find the index of the entered name of entry in EEPROM
uint32_t findIndex(char *name)
{

    //this is the current size
    uint32_t size = readEeprom(0);
    uint16_t position = 0;

    //iterate through the eeprom size
    for (position = 0; position < size; position++)
    {
        //the start -> start
        //start is a variable that is changing through
        //each iteration
        int16_t start = (position * ((NAME_LENGTH / 4) + 1)) + 1;

        uint8_t q = 0;
        //init temp
        uint32_t temp[(NAME_LENGTH / 4) + 1];
        //clear temp
        for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
        {
            temp[q] = 0;
        }
        bool isMatching = true;
        //start reading from the eeprom
        //and start putting them in each position
        //question (NAME_LENGTH / 4) + 1
        for (q = 0; q < 4; q++)
        {
            temp[q] = readEeprom(start + q);
        }
        //iterate through the string size
        for (q = 0; q < NAME_LENGTH; q++)
        {
            if ((temp[q / 4] << ((q % 4)) * 8) >> 3 * 8 != name[q])
            {
                //they are not the same
                isMatching = false;
            }
            if (name[q] == '\0')
                break;
        }
        if (isMatching == 1)
        {
            currentStatus = Found;
            return position;
        }

    }
    currentStatus = notFound;
    return notFound;
}


// On startup check if there is already an IP or MQTT saved to the EEPROM
// if so, update the flags to prevent multiple entries of IP or MQTT addresses
bool startupCheck()
{
    char* ip = "IP_Address";
    char* mqtt = "MQTT";

    currentStatus = notFound;
    uint8_t currentIndex = findIndex(ip);
    if(currentStatus == Found)
    {
        isIPset = true;
        // if IP is already stored in eeprom, don't save default IP address
        return true;
    }

    currentStatus = notFound;
    currentIndex = findIndex(mqtt);
    if(currentStatus == Found)
    {
        isMQTTset = true;
    }

}

void eraseName(char *name)
{
    currentStatus = notFound;
    uint8_t currentIndex = findIndex(name);
    if (currentStatus == Found)
    {
        uint32_t size;
        uint16_t start;
        uint8_t it = 0;
        //read the size
        size = readEeprom(0);
        //find the start position using the index of
        //that name using the function
        start = (currentIndex * ((NAME_LENGTH / 4) + 1)) + 1;

        //iterate and find -> writeEeprom();
        for (it = 0; it < (NAME_LENGTH / 4) + 1; it++)
        {
            writeEeprom(start + it, 0);
        }
        putsUart0("\t\r\n**");
        putsUart0(name);
        putsUart0(" has been edited**\t\r\n");
    }
    else if (currentStatus == notFound)
    {
        putsUart0("Entry was not found.\t\r\n");
    }
    else
    {
        putsUart0("Error!\t\r\n");
    }
}

void flashEeprom()
{
    putsUart0("Flashing EEPROM...\t\r\n");
    writeEeprom(0, 0);
    // reset the status of IP and MQTT being stored in the EEPROM since they have now been removed.
    isIPset = false;
    isMQTTset = false;
}

// function to store IP address of Host and MQTT in EEPROM
void storeIP(bool mqtt, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
{
    char* ip_name;
    ipAddressEEPROM[0] = ip0;
    ipAddressEEPROM[1] = ip1;
    ipAddressEEPROM[2] = ip2;
    ipAddressEEPROM[3] = ip3;

    if(mqtt)
    {
        ip_name = "MQTT";
        // put in check to make sure MQTT is not already stored
        // if stored, then erase and rewrite from user entry
        if(isMQTTset)
        {
            eraseName(ip_name);
        }

        isMQTTset = true;
    }
    else
    {
        ip_name = "IP_Address";
        // put in check to make sure IP_Address is not already stored
        // if stored, then erase and rewrite from user entry
        if(isIPset)
        {
            eraseName(ip_name);
        }
        isIPset = true;
    }

    uint32_t listSize = readEeprom(0);
    uint8_t q;
    uint32_t temp[(NAME_LENGTH/4) + 1];
    uint16_t format = (listSize * ((NAME_LENGTH/4)+1)) + 1;

    // clear temp array for eeprom
    for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
    {
        temp[q] = 0;
    }
    // save ip_name "IP_Address" to temp
    // Later on when retrieving the IP address from EEPROM I want to
    // be able to call it by the name "IP_Address"
      for (q = 0; q < (NAME_LENGTH) && ip_name[q] != '\0'; q++)
      {
          temp[q/4] |= ip_name[q] << ((4-1) - (q%4)) * 8;
      }

      /*
      // include ip address number with the name "IP_Address"
      for (q = 0; q < 4; q++)
      {
          temp[(NAME_LENGTH)/4] = ipAddressEEPROM[q];
      }
      */
      temp[(NAME_LENGTH)/4] = (ipAddressEEPROM[0]<<24) | (ipAddressEEPROM[1]<<16) | (ipAddressEEPROM[2]<<8) | ipAddressEEPROM[3];
      // now write to EEPROM
      for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
      {
          writeEeprom(q + format, temp[q]);
      }

      // get to next spot in eeprom for the next entry if needed
      writeEeprom(0, listSize + 1);

      // print to screen for feedback confirmation that the new addresses have been saved in eeprom
      if(mqtt)
      {
          putsUart0("\t\r\n<MQTT has been stored>");
      }
      else
      {
          putsUart0("\t\r\n<IP_Address has been stored>");
      }
}

void learnInstruction(char* name, uint8_t add, uint8_t data)
{
    uint32_t listSize = readEeprom(0);
    uint8_t q;
    uint8_t k;
    uint32_t temp[(NAME_LENGTH/4) + 1];
    uint16_t format = (listSize * ((NAME_LENGTH/4)+1)) + 1;

    putsUart0("Learning Instruction...\t\r\n");

    // clear Temp array for eeprom
    for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
    {
        temp[q] = 0;
    }
    // save name to temp
    for (q = 0; q < (NAME_LENGTH) && name[q] != '\0'; q++)
    {
        temp[q/4] |= name[q] << ((4-1) - (q%4)) * 8;
        k = q;
    }
    for (k ; k < (NAME_LENGTH); k++)
    {
        temp[k/4] |= ipAddressEEPROM[k] << ((4-1) - (q%4)) * 8;
    }
    // include associated address and data with new command
    //temp[(NAME_LENGTH)/4] = (add << 8) | data;

    for (q=0; q < (NAME_LENGTH/4) + 1; q++)
    {
        writeEeprom(q + format, temp[q]);
    }

    // get to next spot in list for next entry in the Eeprom
    writeEeprom(0, listSize + 1);
}

// learn a new rule that does a specific function on a desired hardware device
void learnRule(char* name, char* dev, char* action)
{
    uint32_t listSize = readEeprom(0);
    uint8_t q;
    uint32_t temp[(NAME_LENGTH/4) + 1];
    uint16_t format = (listSize * ((NAME_LENGTH/4)+1)) + 1;

    putsUart0("Learning Rule...\t\r\n");

    // Clear Eeprom
    for (q = 0; q < (NAME_LENGTH/4) + 1; q++)
    {
        temp[q] = 0;
    }
    // save name
    for (q = 0; q < (NAME_LENGTH) && name[q] != '\0'; q++)
    {
        temp[q/4] |= name[q] << ((4-1) - (q%4)) * 8;
    }


    for (q = 0; q < (NAME_LENGTH) && dev[q] != '\0'; q++)
    {
        temp[q/4] |= dev[q] << ((4-1) - (q%4)) * 8;
    }

    for (q = 0; q < (NAME_LENGTH) && action[q] != '\0'; q++)
    {
        temp[q/4] |= action[q] << ((4-1) - (q%4)) * 8;
    }



    // get to next spot in list for next entry in the Eeprom
    writeEeprom(0, listSize + 1);


}

void infoIndex(uint16_t index)
{
    uint32_t size = readEeprom(0);
    uint8_t q = 0;
    char name[NAME_LENGTH];
    uint16_t start = (index * ((NAME_LENGTH / 4) + 1)) + 1;
    //NAME_LENGTH/4 + 1 -> 3+1 => 4
    //below should equal 4
    uint32_t temp[(NAME_LENGTH / 4) + 1];
    //clear the temp to zeros..
    for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
    {
        temp[q] = 0;
    }
    if (index < size)
    {
        for (q = 0; q < (NAME_LENGTH / 4) + 1; q++)
        {
            temp[q] = readEeprom(start + q);
        }
        for (q = 0; q < NAME_LENGTH; q++)
        {
            name[q] = (temp[q / 4] << ((q % 4)) * 8) >> 3 * 8;
        }
        putsUart0("cmd name: ");
        putsUart0(name);
        putsUart0("\t\r\n");

        //now retrieving the address and the data
        uint8_t add = (temp[((NAME_LENGTH / 4) + 1) - 1] << 16) >> 24;
        uint8_t data = (temp[((NAME_LENGTH / 4) + 1) - 1] << 24) >> 24;

        putsUart0("The Address is: ");
        convert2binary(add);
        putsUart0("\t\r\n");
        putsUart0("The Data is: ");
        convert2binary(data);
        putsUart0("\t\r\n");
    }
    else
    {
        putsUart0("Invalid Index\t\r\n");
    }
}


// convert decimal to binary and then print
void convert2binary(uint8_t x)
{
    uint8_t i;
    for (i = 8; i > 0; i--)
    {
        if ((1 << (i - 1)) & x)
        {
            putcUart0('1');
        }
        else
        {
            putcUart0('0');
        }
    }
}

void printIP(uint8_t ip)
{
    char str[10];
    sprintf(str, "%u", ip);
    putsUart0(str);
}




// Function to execute when the user types: "info NAME"
void infoName(char *name)
{
    // first it is ideal that we get the index of said name
    currentStatus = notFound;
    uint8_t currentIndex = findIndex(name);
    if (currentStatus == Found)
    {
        infoIndex(currentIndex);
    }
    else if (currentStatus == notFound)
    {
        putsUart0("The name was not found.\t\r\n");
    }
    else
    {
        putsUart0("Error!\t\r\n");
    }

}



void listCommands()
{
    uint32_t size;
    uint8_t position = 0;
    uint16_t start;
    uint8_t i = 0;
    uint32_t temp[(NAME_LENGTH / 4) + 1];
    char name[NAME_LENGTH];
    size = readEeprom(0);
    putsUart0("\t\r\nSystem Status:");
    putsUart0("\t\r\n-----------------------------------------");

    // If there is no IP or MQTT set, print to terminal when status is called that
    // it is not yet set
    if(!isIPset)
    {
        putsUart0("\t\r\nIP_Address: <Not Set>");
    }
    if(!isMQTTset)
    {
        putsUart0("\t\r\nMQTT: <Not Set>");
    }

    for (position = 0; position < size; position = position + 1)
    {
        start = (position * ((NAME_LENGTH / 4) + 1)) + 1;

        //clear the temp array
        for (i = 0; i < (NAME_LENGTH / 4) + 1; i++)
        {
            temp[i] = 0;
        }
        //this temp holds the extraction of the name

        for (i = 0; i < (NAME_LENGTH / 4) + 1; i++)
        {
            temp[i] = readEeprom(start + i);
        }
        for (i = 0; i < NAME_LENGTH; i++)
        {
            name[i] = (temp[i / 4] << ((i % 4)) * 8) >> 3 * 8;

        }
        if (name[0] != '\0')
        {
            putsUart0("\t\r\n");
            putsUart0(name);
            putsUart0(": ");

            // IP is split up into 4 ints and shifted accordingly to be stored properly
            uint8_t ip0 = (temp[(NAME_LENGTH / 4)]) >> 24;
            uint8_t ip1 = (temp[(NAME_LENGTH / 4)] << 8) >> 24;
            uint8_t ip2 = (temp[(NAME_LENGTH / 4)] << 16) >> 24;
            uint8_t ip3 = (temp[(NAME_LENGTH / 4)] << 24) >> 24;
            printIP(ip0);
            putcUart0('.');
            printIP(ip1);
            putcUart0('.');
            printIP(ip2);
            putcUart0('.');
            printIP(ip3);
        }

    }


}

uint16_t getInfo(char * name){
    uint8_t pos,st;
    uint32_t tempLength;
    pos = findIndex(name);
    st = (pos * 4);
    tempLength = readEeprom(st + 4);
    return ((tempLength << NAME_LENGTH) >> NAME_LENGTH);

}





