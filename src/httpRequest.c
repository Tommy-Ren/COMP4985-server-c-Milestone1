//
// Created by Kiet & Tommy on 12/1/25.
//

#include "../include/httpRequest.h"
#include "../include/stringTools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_HTTP_REQUEST_TOKENS 3
#define RETURN_CHARACTERS "\\r\\n"

enum HTTPStatusCodes
{
    OK                    = 200,
    BAD_REQUEST           = 400,
    INTERNAL_SERVER_ERROR = 500,
};

bool isValidHTTPMethod(const char *method)
{
    if(method)
    {
        if(strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0 || strcmp(method, "HEAD") == 0)
        {
            return true;
        }
    }
    return false;
}

char *stripHTTPRequestReturnCharacters(const char *string)
{
    StringArray stringArray;

    stringArray = tokenizeString(string, RETURN_CHARACTERS);

    return stringArray.strings[0];
}

HTTPRequest *initializeHTTPRequestFromString(const char *string)
{
    StringArray  stringArrayStruct;
    HTTPRequest *request = malloc(sizeof(HTTPRequest));
    const char  *delim   = " ";

    // If the string is null, exit with error.
    if(!string)
    {
        fprintf(stderr, "!string\n");
        exit(EXIT_FAILURE);
    }

    // Tokenize the duplicated string.
    stringArrayStruct = tokenizeString(string, delim);

    // After tokenizing the string
    if(stringArrayStruct.numStrings != NUM_HTTP_REQUEST_TOKENS)
    {
        fprintf(stderr, "Incorrect number of tokens: %u\n", stringArrayStruct.numStrings);

        exit(EXIT_FAILURE);
    }

    // Initialize the struct.
    *request = initializeHTTPRequest(stringArrayStruct.strings[0], stringArrayStruct.strings[1], stringArrayStruct.strings[2]);

    return request;
}

HTTPRequest initializeHTTPRequest(const char *method, const char *path, const char *protocol)
{
    HTTPRequest request;
    // If any of the args are null, exit.
    // This will be handled by errors later.
    if(!method)
    {
        fprintf(stderr, "!method\n");
        exit(EXIT_FAILURE);
    }
    else if(!path)
    {
        fprintf(stderr, "!path\n");
        exit(EXIT_FAILURE);
    }
    else if(!protocol)
    {
        fprintf(stderr, "!protocol\n");
        exit(EXIT_FAILURE);
    }

    // Copy strings.
    request.method   = strdup(method);
    request.path     = strdup(path);
    request.protocol = strdup(protocol);

    return request;
}

void printHTTPRequestStruct(const HTTPRequest *request)
{
    if(!request)
    {
        return;
    }
    printf("HTTPRequest: {"
           "\n\tMethod: %s"
           "\n\tPath: %s"
           "\n\tProtocol: %s"
           "\n}\n",
           request->method,
           request->path,
           request->protocol);
}
