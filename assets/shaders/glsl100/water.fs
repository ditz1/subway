#version 100

// world positions get large, so use highp where the device has it
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform float time;

void main()
{
    vec4 base = colDiffuse*fragColor;

    // ripples are sampled per half block so the water stays chunky like the terrain
    vec2 cell = floor(fragPosition.xz*2.0)*0.5;
    float ripple = sin(cell.x*0.9 + time*1.3)*sin(cell.y*0.7 - time*1.1)
                 + 0.5*sin((cell.x + cell.y)*1.9 + time*2.1);

    // glancing views reflect more sky
    vec3 toEye = normalize(viewPos - fragPosition);
    float fresnel = pow(1.0 - max(toEye.y, 0.0), 3.0);

    vec3 lit = base.rgb*(ambientColor + sunColor*max(-sunDir.y, 0.0)) + vec3(0.05)*ripple;
    lit = mix(lit, fogColor, fresnel*0.6);

    float fog = clamp((length(viewPos - fragPosition) - fogStart)/(fogEnd - fogStart), 0.0, 1.0);
    fog = fog*fog*(3.0 - 2.0*fog);

    gl_FragColor = vec4(mix(lit, fogColor, fog), mix(base.a, 1.0, max(fresnel*0.6, fog)));
}
