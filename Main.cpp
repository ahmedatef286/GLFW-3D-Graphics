#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

int main() {

	glfwInit();//initialize library

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);//tell glfw what major version of open gl we are using with it 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);//tell glfw what minor version of open gl we are using with it 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);//tell glfw what profile? (collection of functions) of open gl we want to use

	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Test Test" , NULL , NULL);//create a window object (yes a window is a class in glfw, awesome)

	glfwMakeContextCurrent(window); // tell opengl to use the window object we created

	glfwDestroyWindow(window);
	glfwTerminate();//end execution
	return 0;
}