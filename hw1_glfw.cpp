/****************************************************************************/
/* This is a simple demo program written for CSE167 by Ravi Ramamoorthi     */
/* This program corresponds to the final OpenGL lecture on shading.         */
/* Modified September 2016 by Hoang Tran to exclusively use modern OpenGL   */
/*                                                                          */
/* This program draws some simple geometry, a plane with four pillars       */
/* textures the ground plane, and adds in a teapot that moves               */
/* Lighting effects are also included with fragment shaders                 */
/* The keyboard function should be clear about the keystrokes               */
/* The mouse can be used to zoom into and out of the scene                  */
/****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#ifndef GLAD_GL_IMPLEMENTATION
	#define GLAD_GL_IMPLEMENTATION
	#include <glad/gl.h>
#endif

#ifndef GLFW_INCLUDE_NONE
	#define GLFW_INCLUDE_NONE
	#include <GLFW/glfw3.h>
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
	#define STB_IMAGE_IMPLEMENTATION
	#include "stb_image.h"
#endif

//This is something that stb image defines in its own include guard, so I should be able to test for it
#ifndef INCLUDE_STB_IMAGE_WRITE_H
	#define STB_IMAGE_WRITE_IMPLEMENTATION
	#include "stb_image_write.h"
#endif

// Use of degrees is deprecated. Use radians for GLM functions
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//#include "./freeimage/FreeImage.h"
#include <iomanip>

#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// settings
int mouseoldx, mouseoldy; // For mouse motion
int windowWidth = 500, windowHeight = 500; //Width/Height of OpenGL window

GLdouble eyeloc = 2.0; // Where to look from; initially 0 -2, 2
GLfloat teapotloc = -0.5; // ** NEW ** where the teapot is located
GLfloat rotamount = 0.0; // ** NEW ** amount to rotate teapot by
GLint animate = 0; // ** NEW ** whether to animate or not
GLuint vertexshader, fragmentshader, shaderprogram; // shaders
GLuint projectionPos, modelviewPos, colorPos; // Locations of uniform variables
glm::mat4 projection, modelview; // The mvp matrices themselves
glm::mat4 identity(1.0f); // An identity matrix used for making transformation matrices


GLubyte woodtexture[256][256][3]; // ** NEW ** texture (from grsites.com)
GLuint texNames[1]; // ** NEW ** texture buffer
GLuint istex;  // ** NEW ** blend parameter for texturing
GLuint islight; // ** NEW ** for lighting
GLint texturing = 1; // ** NEW ** to turn on/off texturing
GLint lighting = 1; // ** NEW ** to turn on/off lighting

/* Variables to set uniform params for lighting fragment shader */
GLuint light0dirn;
GLuint light0color;
GLuint light1posn;
GLuint light1color;
GLuint ambient;
GLuint diffuse;
GLuint specular;
GLuint shininess;

#include "shaders.h"
#include "geometry3.h"

float rValue = 0.0f, gValue = 0.0f, bValue = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void coverApp()
{
	//std::cout << "\x1B[H";    // Codigo para colocar el cursor en el canto superior izquierdo

// Clear the terminal screen on :
// windows , macOS or Unix-like systems
#ifdef _WIN32
    // For Windows
    std::system("cls");
#else
    // For macOS and Unix-like systems
    std::system("clear");
#endif
	
	std::cout << "\x1B[H";    // Codigo para colocar el cursor en el canto superior izquierdo
	//std::cout << "\n";          // Codigo para dar un salto de linea
    std::cout << "\x1B[3;34m";         // Mostrar el siguiente texto en modo de letra italico "[3;" y color azul "[ ;34m"	
	std::cout << "/***************************************************/" << std::endl; 
	std::cout << "\x1B[m";             // Resetear color a valor por defecto
	std::cout << "\x1B[31;5;88mUniversidad Nacional Mayor de San Marcos \x1B[m" << std::endl; 
	std::cout << "\x1B[33;7;88mDoctorado \x1B[m" << std::endl; 
	std::cout << "Curso Topicos Avanzados en Computacion" << std::endl; 
	std::cout << "\x1B[38;5;46mProf. D.Sc. Manuel Eduardo Loaiza Fernandez \x1B[m" << std::endl; 
	std::cout << "Semestre 2025 - I" << std::endl; 
	std::cout << "\x1B[3;34m";         // Mostrar el siguiente texto en modo de letra italico "[3;" y color azul "[ ;34m"	
	std::cout << "/***************************************************/" << std::endl;
	
	std::cout << "\x1B[m";             // Resetear color a valor por defecto 
}

/* New helper transformation function to transform vector by modelview */
void transformvec(const GLfloat input[4], GLfloat output[4]) 
{
	glm::vec4 inputvec(input[0], input[1], input[2], input[3]);
	glm::vec4 outputvec = modelview * inputvec;

	output[0] = outputvec[0];
	output[1] = outputvec[1];
	output[2] = outputvec[2];
	output[3] = outputvec[3];
}

// Treat this as a destructor function. Delete any dynamically allocated memory here
void deleteBuffers() 
{
	glDeleteVertexArrays(numobjects + ncolors, VAOs);
	glDeleteVertexArrays(1, &teapotVAO);
	glDeleteBuffers(numperobj*numobjects + ncolors, buffers);
	glDeleteBuffers(3, teapotbuffers);
}

void display(void)
{
	// clear all pixels  
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// draw white polygon (square) of unit length centered at the origin
	// Note that vertices must generally go counterclockwise
	// Change from the first program, in that I just made it white.
	// The old OpenGL code of using glBegin... glEnd no longer appears. 
	// The new version uses vertex buffer objects from init.   
	
	glUniform1i(islight, 0); // Turn off lighting (except on teapot, later)
	glUniform1i(istex, texturing);

	// Draw the floor
	// Start with no modifications made to the model matrix
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3f(colorPos, 1.0f, 1.0f, 1.0f); // The floor is white
	drawtexture(FLOOR, texNames[0]); // Texturing floor 
	glUniform1i(istex, 0); // Other items aren't textured 

	//glUniform1i(islight, 1); // Turn off lighting (except on teapot, later)

	// Now draw several cubes with different transforms, colors
	// We now maintain a stack for the modelview matrices. Changes made to the stack after pushing
	// are discarded once it is popped.
	pushMatrix(modelview);
	
	// 1st pillar 
	// This function builds a new matrix. It doesn't actually modify the passed in matrix.
	// Consequently, we need to assign this result to modelview.
	modelview = modelview * glm::translate(identity, glm::vec3(-0.4, -0.4, 0.0));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3fv(colorPos, 1, _cubecol[0]);
	drawcolor(CUBE, 0);
	popMatrix(modelview);

	// 2nd pillar 
	pushMatrix(modelview);
	modelview = modelview * glm::translate(identity, glm::vec3(0.4, -0.4, 0.0));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3fv(colorPos, 1, _cubecol[1]);
	drawcolor(CUBE, 1);
	popMatrix(modelview);

	// 3rd pillar 
	pushMatrix(modelview);
	modelview = modelview * glm::translate(identity, glm::vec3(0.4, 0.4, 0.0));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3fv(colorPos, 1, _cubecol[2]);
	drawcolor(CUBE, 2);
	popMatrix(modelview);

	// 4th pillar 
	/*pushMatrix(modelview);
	modelview = modelview * glm::translate(identity, glm::vec3(-0.4, 0.4, 0.0));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3fv(colorPos, 1, _cubecol[3]);
	drawcolor(CUBE, 3);
	popMatrix(modelview);*/

	// Draw a teapot
	/* New for Demo 3; add lighting effects */
	{
		const GLfloat one[] = { 1, 1, 1, 1 };
		const GLfloat medium[] = { 0.5, 0.5, 0.5, 1 };
		const GLfloat small[] = { 0.2f, 0.2f, 0.2f, 1 };
		const GLfloat high[] = { 100 };
		const GLfloat zero[] = { 0.0, 0.0, 0.0, 1.0 };
		const GLfloat light_specular[] = { 1, 0.5, 0, 1 };
		const GLfloat light_specular1[] = { 0, 0.5, 1, 1 };
		const GLfloat light_direction[] = { 0.5, 0, 0, 0 }; // Dir light 0 in w 
		const GLfloat light_position1[] = { 0, -0.5, 0, 1 };

		GLfloat light0[4], light1[4];

		// Set Light and Material properties for the teapot
		// Lights are transformed by current modelview matrix. 
		// The shader can't do this globally. 
		// So we need to do so manually.  
		transformvec(light_direction, light0);
		transformvec(light_position1, light1);

		glUniform3fv(light0dirn, 1, light0);
		glUniform4fv(light0color, 1, light_specular);
		glUniform4fv(light1posn, 1, light1);
		glUniform4fv(light1color, 1, light_specular1);
		// glUniform4fv(light1color, 1, zero) ; 

		glUniform4fv(ambient, 1, small);
		glUniform4fv(diffuse, 1, medium);
		glUniform4fv(specular, 1, one);
		glUniform1fv(shininess, 1, high);

		// Enable and Disable everything around the teapot 
		// Generally, we would also need to define normals etc. 
		// In the old OpenGL code, GLUT defines normals for us. The glut teapot can't
		// be drawn in modern OpenGL, so we need to load a 3D model for it. The normals
		// are defined in the 3D model file.
		glUniform1i(islight, lighting); // turn on lighting only for teapot. 
	}

	//  ** NEW ** Put a teapot in the middle that animates 
	glUniform3f(colorPos, 0.0f, 1.0f, 1.0f);
	
	//  ** NEW ** Put a teapot in the middle that animates
	pushMatrix(modelview);
	modelview = modelview * glm::translate(identity, glm::vec3(teapotloc, 0.0, 0.0));

	//  The following two transforms set up and center the teapot 
	//  Remember that transforms right-multiply the modelview matrix (top of the stack)
	modelview = modelview * glm::translate(identity, glm::vec3(0.0, 0.0, 0.1));
	modelview = modelview * glm::rotate(glm::mat4(1.0f), rotamount * glm::pi<float>() / 180.0f, glm::vec3(0.0, 0.0, 1.0));
	modelview = modelview * glm::rotate(identity, glm::pi<float>() / 2.0f, glm::vec3(1.0, 0.0, 0.0));
	float size = 0.235f; // Teapot size
	modelview = modelview * glm::scale(identity, glm::vec3(size, size, size));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	drawteapot();
	popMatrix(modelview);

	// 4th pillar 
	pushMatrix(modelview);
	modelview = modelview * glm::translate(identity, glm::vec3(-0.4, 0.4, 0.0));
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	glUniform3fv(colorPos, 1, _cubecol[3]);
	drawcolortriangle(CUBE, 3);
	popMatrix(modelview);


	// Does order of drawing matter? 
	// What happens if I draw the ground after the pillars? 
	// I will show this in class.

	// glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]); 
	// drawobject(FLOOR) ; 

	// don't wait!  
	// start processing buffered OpenGL routines 


	//glutSwapBuffers();
	//glFlush();
}

// ** NEW ** in this assignment, is an animation of a teapot 
// Hitting p will pause this animation; see keyboard callback
void animation(void) 
{
	teapotloc = teapotloc + 0.0025;
	rotamount = rotamount + 0.25;
	if (teapotloc > 0.5) teapotloc = -0.5;
	if (rotamount > 360.0) rotamount = 0.0;
}

void moveTeapot() 
{
	rotamount = 45.0;
	teapotloc = -0.05;
}

// Defines a Mouse callback to zoom in and out 
// This is done by modifying gluLookAt         
// The actual motion is in mousedrag           
// mouse simply sets state for mousedrag       
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{	
	if (button == GLFW_MOUSE_BUTTON_LEFT) 
	{
		if (action == GLFW_PRESS) 
		{
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			mouseoldx = xpos; 
			mouseoldy = ypos; // so we can move wrt x , y 
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{ 
		// Reset gluLookAt
		eyeloc = 2.0;
		modelview = glm::lookAt(glm::vec3(0, -eyeloc, eyeloc), glm::vec3(0, 0, 0), glm::vec3(0, 1, 1));
		// Send the updated matrix to the shader
		glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
	}
}

void mousedrag(int x, int y) 
{
	int yloc = y - mouseoldy;    // We will use the y coord to zoom in/out
	eyeloc += 0.005*yloc;         // Where do we look from

	if (eyeloc < 0)
	{
		eyeloc = 0.0;
	}
		
	mouseoldy = y;

	/* Set the eye location */
	modelview = glm::lookAt(glm::vec3(0, -eyeloc, eyeloc), glm::vec3(0, 0, 0), glm::vec3(0, 1, 1));
	// Send the updated matrix to the shader
	glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);

}

void printHelp() 
{
	coverApp();
	
	std::cout << "\nAvailable commands:\n"
		<< "press 'h' to print this message again.\n"
		<< "press Esc to quit.\n"
		<< "press 'o' to save a screenshot to \"./screenshot.png\".\n"
		<< "press 'i' to move teapot into position for HW0 screenshot.\n"
		<< "press 'p' to start/stop teapot animation.\n"
		<< "press 't' to turn texturing on/off.\n"
		<< "press 's' to turn shading on/off.\n";
}

void swap_rgb( unsigned char* in_rgb, unsigned char* out_rgb)
{
	unsigned char r,g,b;
	r = in_rgb[0];
	g = in_rgb[1];
	b = in_rgb[2];
	
	in_rgb[0] = out_rgb[0];
	in_rgb[1] = out_rgb[1];
	in_rgb[2] = out_rgb[2];
	
	out_rgb[0] = r;
	out_rgb[1] = g;
	out_rgb[2] = b;
}

void saveScreenshot() 
{
	int pix = windowWidth * windowHeight;
	//BYTE *pixels = new BYTE[3 * pix];
	unsigned char *pixels = new unsigned char[3 * pix];
	glReadBuffer(GL_FRONT);
	//glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	
	int idx_begin{0} , idx_end{0};
	int fil_counter{0}, size_buffer{0};
	
	size_buffer = (pix*3)/2;
	
	for(int x=0; x < size_buffer; x+=3)
	{
		idx_begin = x;
		
		if( x%windowWidth == 0 )
		{
			fil_counter++;
			idx_end = 3*windowWidth*( windowHeight - fil_counter);
		}
		else
		{
			idx_end+=3;
		}
		
		swap_rgb( &(pixels[idx_begin]),  &(pixels[idx_end]));
	}
	
	// Save an image of framebuffer
	int retVal = stbi_write_png("screenshot.png", windowWidth, windowHeight, 3, pixels, windowWidth * 3);
	std::cout << "Saving screenshot: screenshot.png\n";
	
	delete[] pixels;
}

/* Reshapes the window appropriately */
void reshape(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	// Think about the rationale for this choice for glm::perspective 
	// What would happen if you changed near and far planes? 
	projection = glm::perspective(30.0f / 180.0f * glm::pi<float>(), (GLfloat)w / (GLfloat)h, 1.0f, 10.0f);
	glUniformMatrix4fv(projectionPos, 1, GL_FALSE, &projection[0][0]);
}

void checkOpenGLVersion() 
{
	const char *version_p = (const char *)glGetString(GL_VERSION);
	float version = 0.0f;

	if (version_p != NULL)
		version = atof(version_p);

	if (version < 3.1f) {
		std::cout << std::endl << "*****************************************" << std::endl;

		if (version_p != NULL) {
			std::cout << "WARNING: Your OpenGL version is not supported." << std::endl;
			std::cout << "We detected version " << std::fixed << std::setprecision(1) << version;
			std::cout << ", but at least version 3.1 is required." << std::endl << std::endl;
		}
		else {
			std::cout << "WARNING: Your OpenGL version could not be detected." << std::endl << std::endl;
		}

		std::cout << "Please update your graphics drivers BEFORE posting on the forum. If this" << std::endl
			<< "doesn't work, ensure your GPU supports OpenGL 3.1 or greater." << std::endl;

		std::cout << "If you receive a 0xC0000005: Access Violation error, this is likely the reason." << std::endl;

		std::cout << std::endl;

		std::cout << "Additional OpenGL Info:" << std::endl;
		std::cout << "(Please include with support requests)" << std::endl;
		std::cout << "GL_VERSION: ";
		std::cout << glGetString(GL_VERSION) << std::endl;
		std::cout << "GL_VENDOR: ";
		std::cout << glGetString(GL_VENDOR) << std::endl;
		std::cout << "GL_RENDERER: ";
		std::cout << glGetString(GL_RENDERER) << std::endl;

		std::cout << std::endl << "*****************************************" << std::endl;
		std::cout << std::endl << "Select terminal and press <ENTER> to continue." << std::endl;
		std::cin.get();
		std::cout << "Select OpenGL window to use commands below." << std::endl;
	}
}

void init( std::string &out )
{
	//Warn students about OpenGL version before 0xC0000005 error
	checkOpenGLVersion();

	printHelp();

	/* select clearing color 	*/
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* initialize viewing values  */
	projection = glm::mat4(1.0f); // The identity matrix
	modelview = glm::lookAt(glm::vec3(0, -eyeloc, eyeloc), glm::vec3(0, 0, 0), glm::vec3(0, 1, 1));

	std::string base_path_file, vertex_file, fragment_file;

#ifdef __APPLE__
	base_path_file = out + "/glfw-master/OwnProjects/hw0-glfw";
#else
	base_path_file = out + "\\glfw-master\\OwnProjects\\hw0-glfw";
#endif


	std::cout<<"\n Path of shader and image data files is: "<< base_path_file<<std::endl;

#ifdef __APPLE__
	vertex_file = base_path_file + "/shaders/light.vert.glsl";
	fragment_file = base_path_file + "/shaders/light.frag.glsl";
#else
	vertex_file = base_path_file + "\\shaders\\light.vert.glsl";
	fragment_file = base_path_file + "\\shaders\\light.frag.glsl";
#endif
	
	std::cout<<"base path: "<<base_path_file<<std::endl;
	
	// Initialize the shaders
#ifdef __unix__         
	vertexshader = initshaders(GL_VERTEX_SHADER, vertex_file.c_str());
	fragmentshader = initshaders(GL_FRAGMENT_SHADER, fragment_file.c_str());

#elif defined(_WIN32) || defined(WIN32) 
	vertexshader = initshaders(GL_VERTEX_SHADER, vertex_file.c_str());
	fragmentshader = initshaders(GL_FRAGMENT_SHADER, fragment_file.c_str());
#else
	vertexshader = initshaders(GL_VERTEX_SHADER, vertex_file.c_str());
	fragmentshader = initshaders(GL_FRAGMENT_SHADER, fragment_file.c_str());
#endif

	std::cout<<"vertex file: "<<vertex_file<<std::endl;
	std::cout<<"fragment file: "<<fragment_file<<std::endl;

	GLuint program = glCreateProgram();
	shaderprogram = initprogram(vertexshader, fragmentshader);
	GLint linked;
	glGetProgramiv(shaderprogram, GL_LINK_STATUS, &linked);

	// * NEW * Set up the shader parameter mappings properly for lighting.
	islight = glGetUniformLocation(shaderprogram, "islight");
	light0dirn = glGetUniformLocation(shaderprogram, "light0dirn");
	light0color = glGetUniformLocation(shaderprogram, "light0color");
	light1posn = glGetUniformLocation(shaderprogram, "light1posn");
	light1color = glGetUniformLocation(shaderprogram, "light1color");
	ambient = glGetUniformLocation(shaderprogram, "ambient");
	diffuse = glGetUniformLocation(shaderprogram, "diffuse");
	specular = glGetUniformLocation(shaderprogram, "specular");
	shininess = glGetUniformLocation(shaderprogram, "shininess");

	// Get the positions of other uniform variables
	projectionPos = glGetUniformLocation(shaderprogram, "projection");
	modelviewPos = glGetUniformLocation(shaderprogram, "modelview");
	colorPos = glGetUniformLocation(shaderprogram, "color");

	// Now create the buffer objects to be used in the scene later
	glGenVertexArrays(numobjects + ncolors, VAOs);
	glGenVertexArrays(1, &teapotVAO);
	glGenBuffers(numperobj * numobjects + ncolors + 1, buffers); // 1 extra buffer for the texcoords
	glGenBuffers(3, teapotbuffers);

	// Initialize texture
	std::string texture_file;
#ifdef __APPLE__
	texture_file = base_path_file + "/textures/wood.ppm";
#else
	texture_file = base_path_file + "\\textures\\wood.ppm";
#endif
	
#ifdef __unix__         
	inittexture(texture_file.c_str(), shaderprogram);
#elif defined(_WIN32) || defined(WIN32) 
	inittexture(texture_file.c_str(), shaderprogram);
#else
	inittexture(texture_file.c_str(), shaderprogram);
#endif
	std::cout<<"\nTexture file in: "<< texture_file << std::endl;

	// Initialize objects
	initobject(FLOOR, (GLfloat *)floorverts, sizeof(floorverts), (GLfloat *)floorcol, sizeof(floorcol), (GLubyte *)floorinds, sizeof(floorinds), GL_TRIANGLES);
	initcubes(CUBE, (GLfloat *)cubeverts, sizeof(cubeverts), (GLubyte *)cubeinds, sizeof(cubeinds), GL_TRIANGLES);

	// Initialize teapot file path
	std::string path_to_teapot_obj;
#ifdef __APPLE__
	path_to_teapot_obj = base_path_file + "/models/teapot.obj";
#else
	path_to_teapot_obj = base_path_file + "\\models\\teapot.obj";
#endif
	
	loadteapot(path_to_teapot_obj);
	std::cout<<"\nModel 3D file in: "<< path_to_teapot_obj << std::endl;

	// Enable the depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // The default option
}

int main(int argc, char** argv)
{
	// Set path to search files to run this demo
	fs::path p = fs::current_path();
	int levels_path = -1;
	fs::path p_current;
	p_current = p.parent_path();

#ifdef __APPLE__
	std::cout<<"\n********!!!!Apple MacOS system detected!!!!******** \n";
	levels_path = 4;
#else
	std::cout<<"\n********!!!!Windows or Linux system detected!!!!******** \n";
	levels_path = 1;
#endif

	for (int i = 0; i < levels_path; i++)
	{
		p_current = p_current.parent_path();
	}
	
	std::string vs_path, fs_path;

	std::stringstream ss;
	ss << std::quoted(p_current.string());
	std::string out;
	ss >> std::quoted(out);

	std::cout << "\nCurrent path: " << out << "\n";

	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	// Requests the type of buffers (Single, RGB).
	// Think about what buffers you would need...

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Simple Demo with Shaders", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);


	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if( !gladLoadGL(glfwGetProcAddress) )
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	init( out ); // Always initialize first

	glViewport(0, 0, (GLsizei)windowWidth, (GLsizei)windowHeight);
	
	// Think about the rationale for this choice for glm::perspective 
	// What would happen if you changed near and far planes? 
	projection = glm::perspective(30.0f / 180.0f * glm::pi<float>(), 
		(GLfloat)windowWidth / (GLfloat)windowHeight, 1.0f, 10.0f);
	glUniformMatrix4fv(projectionPos, 1, GL_FALSE, &projection[0][0]);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// render
		// ------
		glClearColor(rValue, gValue, bValue, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (animate)
		{
			animation();
		}

		// input
		// -----
		display();

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();

	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();

	return 0;   /* ANSI C requires main to return int. */
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// Defines what to do when various keys are pressed 
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	windowWidth = width;
	windowHeight = height;
	glViewport(0, 0, (GLsizei)windowWidth, (GLsizei)windowHeight);

	// Think about the rationale for this choice for glm::perspective 
	// What would happen if you changed near and far planes? 
	projection = glm::perspective(30.0f / 180.0f * glm::pi<float>(), (GLfloat)windowWidth / (GLfloat)windowHeight, 1.0f, 10.0f);
	glUniformMatrix4fv(projectionPos, 1, GL_FALSE, &projection[0][0]);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
	{
		if (rValue == 0)
		{
			rValue = 1;
		}
		else
		{
			rValue = 0;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
	{
		if (gValue == 0)
		{
			gValue = 1;
		}
		else
		{
			gValue = 0;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
	{
		if (bValue == 0)
		{
			bValue = 1;
		}
		else
		{
			bValue = 0;
		}
	}
	
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
	{
		printHelp();
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
	{
		saveScreenshot();
	}
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
	{
		moveTeapot();
		eyeloc = 2.0f;
		// Immediately update the modelview matrix
		modelview = glm::lookAt(glm::vec3(0, -eyeloc, eyeloc), glm::vec3(0, 0, 0), glm::vec3(0, 1, 1));
		// Send the updated matrix to the shader
		glUniformMatrix4fv(modelviewPos, 1, GL_FALSE, &(modelview)[0][0]);
		texturing = 1;
		lighting = 1;
		animate = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		animate = !animate;
	}

	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	{
		texturing = !texturing;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		lighting = !lighting;
	}
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}