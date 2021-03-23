/* Updated Terminal Interface
 *
 * Sean-Michael Woerner 1001229459
 * CSE 4342 - 001
 * 02/15/2021
 */

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"

// Terminal char and field variables
#define MAX_CHARS 80
#define MAX_FIELDS 6
#define DEBUG

// LED Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3

// Command checking
bool valid = false;
bool isEnter =  false;


//-----------------------------------------------------------------------------
// Structs
//-----------------------------------------------------------------------------

typedef struct USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];

} USER_DATA;


//-----------------------------------------------------------------------------
// External methods
//-----------------------------------------------------------------------------

// Blocking function that writes a serial character when the UART buffer is not full
extern void putcUart0(char c);

// Blocking function that writes a string when the UART buffer is not full
extern void putsUart0(char* str);

// Blocking function that returns with serial data once the buffer is not empty
extern char getcUart0();



//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
void initTerminal()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, system clock of 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    enablePort(PORTF);

    // Configure LED pins
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(BLUE_LED);

    //setPinValue(GREEN_LED,1);
}


// Get characters from user interface, processes special chars (like <backspace>), and writes resultant string to buffer
void getsUart0(USER_DATA* data)
{
    int count = 0;
    char c;

    while(1)
    {

        while(1)
        {

            c = getcUart0();     // converts char into ascii value integer

            // LED test to make sure it's taking input
            //setPinValue(GREEN_LED,1);

            // convert all upper-case letters to lower-case
            /*
            if( (c >= 65) && (c<=90))
            {
                c = c + 32;
            }
             */

        // is c = to backspace?
        if (c == 127)
        {
            if (count > 0)
            {
                count -- ;          // decrement count if c is = to bkspc and count is greater than 0
            }

            break;                  // go back to get the next char
        }
        else
        {
            // is c = carriage return or line feed?
            if (c == 13)
            {
                putcUart0('\n');
                data->buffer[count] = 0;
                isEnter = true;             // bool to stop ethernet loop and enter user command
                return;
            }
            else
            {
                // is c >= 32 ? (ASCII < 32 values are things we do not want to print, only > 32)
                if (c >= 32)
                {
                    data->buffer[count] = c;
                    count++;

                    // is count equal to max chars?
                    if(count == MAX_CHARS)
                    {
                        return;     // return since buffer is full
                    }

                }
            }
        }




        }
    }



}


// Take string buffer from function above and process string in-place and returns info about parsed fields in fieldCount, fieldPosition, and fieldType
void parseFields(USER_DATA* data)
{

    int count = 0;
    int fieldCount = 0;
    char temp;
    int offset_counter = 0;

    while(1)
    {
        temp = data->buffer[count];

        // check if a-z or A-Z, record type and also offset
        if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) )
        {
            data->fieldType[fieldCount] = 'a';
            // get offset of string
            offset_counter = count;
            while(1)
            {
                temp = data->buffer[offset_counter];
                if ( (temp >= 65 && temp <= 90) || (temp >= 97 && temp <= 122) )
                {
                    offset_counter ++;
                }
                else
                {

                    // now we have a delimiter to create null in buffer
                    data->buffer[offset_counter] = NULL;
                    break;
                }
            }
            // record @ what index this string begins
            data->fieldPosition[fieldCount] = count;
            // increment count to get past string entered for next field
            count = offset_counter;
            // increment fieldCount
            fieldCount ++;


        }
        // check if number 1-9, record
        else if ( (temp >= 48 && temp <= 57)  )
        {
            data->fieldType[fieldCount] = 'n';
            offset_counter = count;
                       // check if number is more than 1 digit for offset
                       while(1)
                       {
                           temp = data->buffer[offset_counter];
                           if ( (temp >= 48 && temp <= 57) )
                           {
                               offset_counter ++;
                           }
                           else
                           {
                               // now we have a delimiter to create null in buffer
                               data->buffer[offset_counter] = NULL;
                               break;
                           }
                       }
            // record @ what index the number begins
            data->fieldPosition[fieldCount] = count;
            // increment count to skip past number for next field
            count = offset_counter;
            // increment fieldCount
            fieldCount ++;
        }
        // else (it's a delimiter), record a NULL (maybe record in data.buffer)
        else
        {

            data->buffer[count] = NULL;

            // check if next char is NULL, if it is, return. We've reached end of entry
            if (data->buffer[count+1] == NULL)
            {
                data->fieldCount = fieldCount;
                return;
            }

        }


        // increment count
        count ++;

        if ((fieldCount) == MAX_FIELDS)
        {
            // put local field count into struct
            data->fieldCount = fieldCount;
            return;
        }

    }


}



// Return value of field requested if the field number is in range or NULL otherwise
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    char out[MAX_CHARS + 1];
    uint8_t count;
    for (count = data->fieldPosition[fieldNumber]; data->buffer[count] != '\0';
            count++)
    {
        out[count - data->fieldPosition[fieldNumber]] = data->buffer[count];
    }
    out[count - data->fieldPosition[fieldNumber]] = '\0';
    return out;

}



// Return a pointer to the field requested if the field number is in range and the field type is numeric or 0 otherwise
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if (data->fieldType[fieldNumber] == 'n')
        {
            uint8_t count;
            int32_t res = 0;
            for (count = data->fieldPosition[fieldNumber];
                    data->buffer[count] != '\0'; count++)
            {
                res = res * 10 + data->buffer[count] - '0';
            }
            return res;
        }
        else
        {
            return 0;
        }

}



// Return True if the command matches the first field and the number of arguments (excluding the command field) is greater than or equal to the requested number of minimum arguments
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{

    // check for first letter of cmd and min arguemnts
    if( (data->buffer[data->fieldPosition[0]] == *strCommand) && ((data->fieldCount)-1 >= minArguments) )
    {
        // check for second letter of cmd
        if(data->buffer[data->fieldPosition[1]] == *strCommand)
        {
            return true;
        }
    }
    else
    {
        return false;
    }

}


