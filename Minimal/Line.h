#ifndef _LINE_H_
#define _LINE_H_

#define GLFW_INCLUDE_GLEXT
#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#else
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>
// Use of degrees is deprecated. Use radians instead.
#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Line
{
public:
	Line();
	~Line();

	glm::mat4 toWorld;

	glm::vec3 color;

	void draw(GLint shaderProgram, glm::mat4 Projection, glm::mat4 View);
	void update(glm::vec3 p1, glm::vec3 p2);

	// These variables are needed for the shader program
	GLuint VBO, VAO;
	GLuint uProjection, uModel, uView;
	
};

// Define the coordinates and indices needed to draw the cube. Note that it is not necessary
// to use a 2-dimensional array, since the layout in memory is the same as a 1-dimensional array.
// This just looks nicer since it's easy to tell what coordinates/indices belong where.

#endif

