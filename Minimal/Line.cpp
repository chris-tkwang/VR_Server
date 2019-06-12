#include "Line.h"
#include <iostream>

Line::Line()
{
	toWorld = glm::mat4(1.0f);
	color = glm::vec3(0.0f, 1.0f, 0.0f);

	// Create array object and buffers
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
}

Line::~Line()
{
	// Delete previously generated buffer
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
}

void Line::draw(GLint shaderProgram, glm::mat4 Projection, glm::mat4 View) {

	// Set Line Width
	glLineWidth(20.0f);

	GLuint uAmbient = glGetUniformLocation(shaderProgram, "material.ambient");
	GLuint uDiffuse = glGetUniformLocation(shaderProgram, "material.diffuse");

	glUniform3f(uAmbient, color.x, color.y, color.z);
	glUniform3f(uDiffuse, color.x, color.y, color.z);
	

	uProjection = glGetUniformLocation(shaderProgram, "projection");
	uModel = glGetUniformLocation(shaderProgram, "model");
	uView = glGetUniformLocation(shaderProgram, "view");
	// Now send these values to the shader program
	glUniformMatrix4fv(uProjection, 1, GL_FALSE, &Projection[0][0]);
	glUniformMatrix4fv(uModel, 1, GL_FALSE, &View[0][0]);
	glUniformMatrix4fv(uView, 1, GL_FALSE, &toWorld[0][0]);

	// Now draw the cube. 
	glBindVertexArray(VAO);
	// Tell OpenGL to draw with Lines
	glDrawArrays(GL_LINES, 0, 2);
	// Unbind the VAO
	glBindVertexArray(0);
}

void Line::update(glm::vec3 p1, glm::vec3 p2)
{
	

	GLfloat vertices[2][3];
	// Start Point
	vertices[0][0] = p1.x;
	vertices[0][1] = p1.y;
	vertices[0][2] = p1.z;
	// End Point
	vertices[1][0] = p2.x;
	vertices[1][1] = p2.y;
	vertices[1][2] = p2.z;

	// Bind the Vertex Array Object (VAO) first
	glBindVertexArray(VAO);

	// Bind a VBO to it as a GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// glBufferData populates the most recently bound buffer with data starting at the 3rd argument and ending after
	// the 2nd argument number of indices. How does OpenGL know how long an index spans? Go to glVertexAttribPointer.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// Enable the usage of layout location 0 (check the vertex shader to see what this is)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,// This first parameter x should be the same as the number passed into the line "layout (location = x)" in the vertex shader. In this case, it's 0. Valid values are 0 to GL_MAX_UNIFORM_LOCATIONS.
		3, // This second line tells us how any components there are per vertex. In this case, it's 3 (we have an x, y, and z component)
		GL_FLOAT, // What type these components are
		GL_FALSE, // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
		3 * sizeof(GLfloat), // Offset between consecutive indices. Since each of our vertices have 3 floats, they should have the size of 3 floats in between
		(GLvoid*)0); // Offset of the first vertex's component. In our case it's 0 since we don't pad the vertices array with anything.

	// Unbind the currently bound buffer 
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Unbind the VAO 
	glBindVertexArray(0);
}

