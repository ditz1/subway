#version 330

in vec3 vertexPosition;

uniform mat4 matModel;
uniform mat4 lightVP;

void main()
{
    gl_Position = lightVP*matModel*vec4(vertexPosition, 1.0);
}
