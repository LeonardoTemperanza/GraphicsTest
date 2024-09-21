
#pragma once

#include "base.h"
#include "lexer.h"

void ParseFile(const char* path, Slice<Token> tokens, Arena* dst);
void ParseIntrospection(Parser* p);
void ParseMemberList(Parser* p, String structName);

void PrintMemberDefinition(String structName, const char* metaType, int numPointers, bool isSlice, bool isString,
                           String name, String niceName, int memberVersion, bool showEditor);
// Converts camel case to regular sentence, like this:
// From: "thisIsAnExampleTest"
// To:   "This Is An Example Test"
String GetNiceNameFromMemberName(String memberName, Arena* dst);
void SetOutput(FILE* file);

// Metaprogram standardized things
void PrintPreambleComment();