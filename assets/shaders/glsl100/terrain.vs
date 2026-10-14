#version 100

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform mat4 lightVP;

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
varying vec4 fragLightPos;

void main()
{
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragLightPos = lightVP*vec4(fragPosition, 1.0);
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 0.0)));

    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
