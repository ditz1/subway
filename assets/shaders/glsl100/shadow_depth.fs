#version 100

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

// depth from the sun, encoded into rgb so it works without depth texture support
void main()
{
    vec3 encoded = fract(gl_FragCoord.z*vec3(1.0, 255.0, 65025.0));
    encoded.xy -= encoded.yz/255.0;
    gl_FragColor = vec4(encoded, 1.0);
}
