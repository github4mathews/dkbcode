#include "stringHandlingServices.h"

void StringHandlingServices::str_replace(char *src, char *oldchars, char *newchars)
{
    int MAX_STRING_LENGTH = 100;
    char *p = strstr(src, oldchars);
    char buf[MAX_STRING_LENGTH];
    do
    {
        if (p)
        {
            memset(buf, '\0', strlen(buf));
            if (src == p)
            {
                strcpy(buf, newchars);
                strcat(buf, p + strlen(oldchars));
            }
            else
            {
                strncpy(buf, src, strlen(src) - strlen(p));
                strcat(buf, newchars);
                strcat(buf, p + strlen(oldchars));
            }
            memset(src, '\0', strlen(src));
            strcpy(src, buf);
        }
    } while (p && (p = strstr(src, oldchars)));
}

int StringHandlingServices::StringSplit(String sInput, char cDelim, String sParams[], int iMaxParams)
{
    int iParamCount = 0;
    int iPosDelim, iPosStart = 0;

    do
    {
        // Searching the delimiter using indexOf()
        iPosDelim = sInput.indexOf(cDelim, iPosStart);
        if (iPosDelim >= (iPosStart + 1))
        {
            // Adding a new parameter using substring()
            sParams[iParamCount] = sInput.substring(iPosStart, iPosDelim);
            iParamCount++;
            // Checking the number of parameters
            if (iParamCount >= iMaxParams)
            {
                return (iParamCount);
            }
            iPosStart = iPosDelim + 1;
        }
    } while (iPosDelim >= 0);
    if (iParamCount < iMaxParams)
    {
        // Adding the last parameter as the end of the line
        sParams[iParamCount] = sInput.substring(iPosStart);
        iParamCount++;
    }
    return (iParamCount);
}
