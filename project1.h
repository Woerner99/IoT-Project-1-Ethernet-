#ifndef __PROJECT1__H
#define __PROJECT1__H


//-----------------------------------------------------------------------------
// Serial Communication defines and functions
//-----------------------------------------------------------------------------
#define MAX_CHARS 80
#define MAX_FIELDS 6


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

void reboot()
{
    NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_VECT_RESET;
}



//---------------------------------------------------
// VARIABLES
//---------------------------------------------------


// bool to check if a valid cmd
bool valid2 = false;
// bool to determine if either MQTT or IP address
bool isMQTT = false;


#endif