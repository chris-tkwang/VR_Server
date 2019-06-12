#version 330 core

in vec3 inputPosition;
in vec2 inputTexCoord;
in vec3 inputNormal;

uniform mat4 projection, modelview, normalMat;

out vec3 normalInterp;
out vec3 vertPos;

void main(){
    gl_Position = projection * modelview * vec4(inputPosition, 1.0);
    vec4 vertPos4 = modelview * vec4(inputPosition, 1.0);
    vertPos = vec3(vertPos4) / vertPos4.w;
    normalInterp = vec3(normalMat * vec4(inputNormal, 0.0));
}
