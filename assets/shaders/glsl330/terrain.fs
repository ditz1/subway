#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in vec4 fragLightPos;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform vec3 sunDir;        // direction the light travels
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform sampler2D shadowMap;   // packed depth as seen from the sun
uniform float shadowTexel;

// Output fragment color
out vec4 finalColor;

// how much of the sun reaches this fragment, 0 in full shadow
float SunVisibility(float diffuse)
{
    vec3 p = fragLightPos.xyz/fragLightPos.w*0.5 + 0.5;
    if (p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0 || p.z > 1.0) return 1.0;

    float bias = 0.0005 + 0.0012*(1.0 - diffuse);
    float lit = 0.0;
    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            vec2 offset = (vec2(float(x), float(y)) - 0.5)*shadowTexel;
            float depth = dot(texture(shadowMap, p.xy + offset).rgb, vec3(1.0, 1.0/255.0, 1.0/65025.0));
            if (p.z - bias <= depth) lit += 0.25;
        }
    }
    return lit;
}

void main()
{
    vec4 base = texture(texture0, fragTexCoord)*colDiffuse*fragColor;
    vec3 normal = normalize(fragNormal);

    // sun plus sky light, which reaches upward faces more than downward ones
    float diffuse = max(dot(normal, -sunDir), 0.0);
    vec3 ambient = ambientColor*(0.75 + 0.25*normal.y);
    vec3 lit = base.rgb*(ambient + sunColor*diffuse*SunVisibility(diffuse));

    // fade into the sky with distance
    float fog = clamp((length(viewPos - fragPosition) - fogStart)/(fogEnd - fogStart), 0.0, 1.0);
    fog = fog*fog*(3.0 - 2.0*fog);

    finalColor = vec4(mix(lit, fogColor, fog), base.a);
}
