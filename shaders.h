#include <iostream>
#include <string>

#ifndef __INCLUDESHADERS 
#define __INCLUDESHADERS 

#ifndef GLAD_GL_IMPLEMENTATION
	#include <glad/gl.h>
#endif

#ifndef GLFW_INCLUDE_NONE
	#include <GLFW/glfw3.h>
#endif

std::string textFileRead (const char * filename) ;
void programerrors (const GLint program) ;
void shadererrors (const GLint shader) ;
GLuint initshaders (GLenum type, const char * filename) ;
GLuint initprogram (GLuint vertexshader, GLuint fragmentshader) ;

#endif 
