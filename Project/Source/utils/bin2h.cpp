
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "base.h"

void GenerateHeaderFile(char** fileNames, char** varNames, char* outputName);

// Simple program to convert binary files to global
// variables in a header file, to embed things into the executable
// Usage:
// bin2h.exe file1 name1 file2 name2 ... -o file.h
int main(int argCount, char** args)
{
    if(argCount < 3)
    {
        fprintf(stderr, "Error: Incorrect usage. Expecting more than 1 argument.\nUsage: bin2h.exe file1 name1 file2 name2 ... -o file.h\n\n");
        return 1;
    }
    
    // Overallocate for simplicity
    int size = sizeof(char*) * argCount;
    char** fileNames = (char**)malloc(size);
    char** varNames  = (char**)malloc(size);
    memset(fileNames, 0, size);
    memset(varNames, 0, size);
    
    const char* outputFileName = "output.h";
    int numFiles = 0;
    
    // Parse cmd line args
    bool isFileName = true;
    for(int i = 1; i < argCount; ++i)
    {
        if(strcmp(args[i], "-o") == 0)
        {
            if(i >= argCount - 1)
            {
                fprintf(stderr, "Error: Incorrect usage caused by argument %d. Output file name was not specified.\n", i);
                return 1;
            }
            
            ++i;
            outputFileName = args[i];
        }
        else
        {
            if(isFileName)
            {
                if(i >= argCount - 1 || strcmp(args[i+1], "-o") == 0)
                {
                    fprintf(stderr, "Error: Incorrect usage caused by argument %d. File names must be followed the variable name in the header file.\n", i); 
                    return 1;
                }
                
                fileNames[numFiles] = args[i];
            }
            else
            {
                varNames[numFiles] = args[i];
                ++numFiles;
            }
            
            isFileName = !isFileName;
        }
    }
    
    if(numFiles == 0) return 0;
    
    // Generate output file
    FILE* output = fopen(outputFileName, "w");
    defer { fclose(output); };
    if(!output)
    {
        fprintf(stderr, "Error: Could not write to file\n");
        return 1;
    }
    
    fprintf(output, "\n#pragma once\n\n");
    
    for(int i = 0; i < numFiles; ++i)
    {
        // Load contents of file
        FILE* input = fopen(fileNames[i], "rb");
        defer { fclose(input); };
        if(!input)
        {
            fprintf(stderr, "Error: Could not find file '%s'\n", fileNames[i]);
            return 1;
        }
        
        fseek(input, 0, SEEK_END);
        const int inputSize = ftell(input);
        fseek(input, 0, SEEK_SET);
        unsigned char* inputContents = (unsigned char*)malloc(inputSize);
        fread(inputContents, inputSize, 1, input);
        
        // Print to file
        fprintf(output, "/* Embedded file: %s */\n", fileNames[i]);
        fprintf(output, "const unsigned char %s[] = {\n", varNames[i]);
        
        // File contents
        for(int i = 0; i < inputSize; ++i)
        {
            const char* appendix = "";
            if(i < inputSize - 1)
            {
                if((i+1) % 16 == 0) appendix = ",\n";
                else                appendix = ", ";
            }
            
            fprintf(output, "0x%02x%s", inputContents[i], appendix);
        }
        
        fprintf(output, "\n};\n");
        if(i < numFiles-1) fprintf(output, "\n");
    }
    
    return 0;
}
