//
// Created by Kiet & Tommy on 12/1/25.
//

#include "../include/stringTools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int getNumberOfTokens(const char *string, const char *delim)
{
    unsigned int count = 0;
    char        *strcopy;
    const char  *token;
    char        *savePtr;

    // Duplicate the string.
    strcopy = strdup(string);

    // Initialize count to 0;
    token = strtok_r(strcopy, delim, &savePtr);

    // Count until no more tokens.
    while(token)
    {
        count++;
        token = strtok_r(NULL, delim, &savePtr);
    }

    // Free duplicated string.
    free(strcopy);

    return count;
}

StringArray tokenizeString(const char *string, const char *delim)
{
    StringArray result;
    char       *stringCopy;
    const char *token;
    char       *savePtr;
    int         index;

    // Use getNumberOfTokens to determine the size of arrays needed.
    result.numStrings = getNumberOfTokens(string, delim);

    // If number of tokens is zero or less, exit with error.
    if(result.numStrings == 0)
    {
        perror("Number of tokens cannot be less than 1.");
        exit(EXIT_FAILURE);
    }

    result.strings       = (char **)malloc(result.numStrings * sizeof(char *));
    result.stringLengths = (unsigned int *)malloc(result.numStrings * sizeof(unsigned int));

    // Duplicate string for safe tokenization.
    stringCopy = strdup(string);
    index      = 0;

    // Get the first token
    token = strtok_r(stringCopy, delim, &savePtr);

    // Tokenize the string
    while(token != NULL)
    {
        // Duplicate token
        result.strings[index] = strdup(token);
        // Store token length
        result.stringLengths[index] = (unsigned int)strlen(token);
        index++;

        // Get the next token
        token = strtok_r(NULL, delim, &savePtr);
    }

    free(stringCopy);    // Free the duplicated string

    return result;
}

TokenAndStr getFirstToken(const char *string, const char *delim)
{
    TokenAndStr result;
    char       *savePtr;

    // Duplicate the string.
    result.originalStr = strdup(string);

    // Tokenize the string.
    result.token = strtok_r(result.originalStr, delim, &savePtr);

    // Now, the caller is responsible for freeing result.originalStr when done
    // with the token.
    return result;
}

TokenAndStr getLastToken(const char *string, const char *delim)
{
    TokenAndStr result;
    char       *savePtr;
    char       *token;

    // Initialize result to ensure a clean state
    result.originalStr = NULL;
    result.token       = NULL;

    // Duplicate the string.
    result.originalStr = strdup(string);
    if(!result.originalStr)
    {
        // Handle strdup failure, possibly due to memory allocation failure
        return result;
    }

    // Tokenize the string and find the last token.
    for(token = strtok_r(result.originalStr, delim, &savePtr); token != NULL; token = strtok_r(NULL, delim, &savePtr))
    {
        result.token = token;
    }

    // At this point, result.token points to the last token found,
    // and result.originalStr holds the duplicated string that needs to be freed
    // by the caller.
    return result;
}

char *addCharacterToStart(const char *original, const char *toAdd)
{
    // calculate the length of the resulting string
    size_t originalLength     = strlen(original);
    size_t toAddLength        = strlen(toAdd);
    size_t returnStringLength = originalLength + toAddLength + 1;

    // allocate memory for the return string
    char *returnString = (char *)malloc(returnStringLength * sizeof(char));
    if(returnString == NULL)
    {
        perror("Error allocating memory");
        return NULL;
    }

    // copy the 'toAdd' string followed by the 'original' string into the return
    // string
    strcpy(returnString, toAdd);
    strcat(returnString, original);
    return returnString;
}

bool checkIfCharInString(const char *stringToCheck, char toCheck)
{
    const size_t checkLength = strlen(stringToCheck);
    for(size_t i = 0; i < checkLength; i++)
    {
        if(stringToCheck[i] == toCheck)
        {
            return true;
        }
    }
    return false;
}
