//
// Created by Kiet & Tommy on 12/1/25.
//

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <stdbool.h>

/**
 * @brief Standard struct for HTTP requests.
 */
typedef struct
{
    /** @brief The method, e.g. GET, POST, HEAD */
    char *method;

    /** @brief The requested file path. */
    char *path;

    /** @brief The protocol, e.g. HTTP/1.1 */
    char *protocol;
} HTTPRequest;

/**
 * @brief Initializes a new HTTP request struct from a string.
 * @param string The string to parse.
 * @return request
 */
HTTPRequest *initializeHTTPRequestFromString(const char *string);

/**
 * @brief Initializes a new HTTP request struct.
 * @param method The method, e.g. GET, POST, HEAD
 * @param path The path, e.g. ./index.html
 * @param protocol The protocol, e.g. 1.0, 1.1
 * @return struct HTTPRequest
 */
HTTPRequest initializeHTTPRequest(const char *method, const char *path, const char *protocol);

/**
 * @brief Strips the given string of return characters.
 * @param string The string to strip.
 * @return stripped
 */
char *stripHTTPRequestReturnCharacters(const char *string);

/**
 * @brief Returns true if the given HTTP method is valid.
 * Returns false if it is not.
 * @param method The HTTP request
 * @return true or false
 */
bool isValidHTTPMethod(const char *method);

/**
 * @brief Prints the values of an HTTPRequest struct.
 * @param request The struct to print.
 */
void printHTTPRequestStruct(const HTTPRequest *request);

#endif    // HTTPREQUEST_H
