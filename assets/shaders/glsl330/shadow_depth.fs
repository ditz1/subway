#version 330

out vec4 finalColor;

// depth from the sun, encoded into rgb so it works without depth texture support
void main()
{
    vec3 encoded = fract(gl_FragCoord.z*vec3(1.0, 255.0, 65025.0));
    encoded.xy -= encoded.yz/255.0;
    finalColor = vec4(encoded, 1.0);
}
