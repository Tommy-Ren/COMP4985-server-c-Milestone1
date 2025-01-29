//
// Created by Kiet & Tommy on 12/1/25.
//

#ifndef FILETOOLS_H
#define FILETOOLS_H

#include <stdbool.h>

/**
 * Contains the data of a file,
 * including name, contents, and content length.
 */
struct fileData;

/**
 * @brief Returns a new fileData struct from a file path.
 * @param filePath The path to the file.
 * @return struct * fileData
 */
struct fileData *getFileDataFromFilePath(const char *filePath);

/**
 * @brief Returns a new fileData struct from the params.
 * @param fileNameLength The length of the file name.
 * @param fileName The file name.
 * @param contentLength The length of the file content.
 * @param content The file content.
 * @return fileData struct
 */
struct fileData *initializeFileDataStruct(int fileNameLength, const char *fileName, long contentLength, const char *content);

/**
 * @brief Prints a fileData struct to the console.
 * @param fileData The struct to print.
 */
void printFileDataStruct(const struct fileData *fileData);

/**
 * @brief Appends text to a given file.
 * @param filePath The full file path.
 * @param text The text to append.
 */
void appendTextToFile(const char *filePath, const char *text);

#endif    // FILETOOLS_H
