#pragma once

#include "Value.h"

class Proto;

void printProto(Proto *proto);
void printBytecode(unsigned *bytecode, int size);
void printValue(char *buf, int bufSize, Value a);
