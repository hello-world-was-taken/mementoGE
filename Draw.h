#ifndef TRIANGLE_DRAWER_H
#define TRIANGLE_DRAWER_H

#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "util/log_error.h"
#include "VertexBuffer.h"
#include "Indexbuffer.h"
#include "Texture.h"
#include "Shader.h"


void setBufferData();
void drawTriangle(GLFWwindow* window);

#endif // TRIANGLE_DRAWER_H
