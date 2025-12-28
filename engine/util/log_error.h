#pragma once

#include <iostream>
#include <unistd.h>
#include <GL/glew.h>

void glClearError();
bool glCheckError(const char* functionName, const char* file, int line);
void checkAlError(const char *msg);
void printPWD();