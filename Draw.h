#ifndef TRIANGLE_DRAWER_H
#define TRIANGLE_DRAWER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

struct Vertex {
    float x, y, z;
};

void setBufferData();
void drawTriangle(GLFWwindow* window);

#endif // TRIANGLE_DRAWER_H
