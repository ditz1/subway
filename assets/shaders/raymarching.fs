#version 100
precision mediump float;

#define MAX_CUBES 100
#define AA 1   // Anti-aliasing level

// Input vertex attributes
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Uniforms
uniform vec3 viewEye;
uniform vec3 viewCenter;
uniform vec2 resolution;
uniform vec3 lightPos;
uniform int numCubes;
uniform vec3 cubePositions[MAX_CUBES];
uniform vec3 cubeSizes[MAX_CUBES];
uniform vec3 cubeColors[MAX_CUBES];

// Modified map function to handle cube indexing
struct MapResult {
    float dist;
    float material;
    float index;
};

float sdBox(vec3 p, vec3 b)
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float sdPlane(vec3 p)
{
    return p.y;
}

MapResult map(in vec3 pos)
{
    MapResult res;
    res.dist = 1e10;  // Initialize to large distance
    res.material = 1.0;
    res.index = -1.0;
    
    // Check cubes first
    for(int i = 0; i < MAX_CUBES; i++) {
        if(i >= numCubes) break;
        float d = sdBox(pos - cubePositions[i], cubeSizes[i]);
        if(d < res.dist) {
            res.dist = d;
            res.material = 2.0;
            res.index = float(i);
        }
    }
    
    // Check plane last
    float planeDist = sdPlane(pos);
    if(planeDist < res.dist) {
        res.dist = planeDist;
        res.material = 1.0;
        res.index = -1.0;
    }
    
    return res;
}

vec3 calcNormal(in vec3 pos)
{
    vec2 e = vec2(0.001, 0.0);
    float a = map(pos + e.xyy).dist;
    float b = map(pos - e.xyy).dist;
    float c = map(pos + e.yxy).dist;
    float d = map(pos - e.yxy).dist;
    float e1 = map(pos + e.yyx).dist;
    float f = map(pos - e.yyx).dist;
    return normalize(vec3(a-b, c-d, e1-f));
}

// float calcAO(in vec3 pos, in vec3 nor)
// {
//     float occ = 0.0;
//     float sca = 1.0;
//     for(int i = 0; i < 5; i++) {
//         float hr = 0.01 + 0.12*float(i)/4.0;
//         vec3 aopos = nor * hr + pos;
//         float dd = map(aopos).dist;
//         occ += -(dd-hr)*sca;
//         sca *= 0.95;
//     }
//     return clamp(1.0 - 3.0*occ, 0.0, 1.0);
// }

// float calcSoftshadow(in vec3 ro, in vec3 rd, float mint, float tmax)
// {
//     float res = 1.0;
//     float t = mint;
    
//     for(int i = 0; i < 16; i++) {
//         float h = map(ro + rd*t).dist;
//         res = min(res, 8.0*h/t);
//         t += clamp(h, 0.02, 0.10);
//         if(h < 0.001 || t > tmax) break;
//     }
//     return clamp(res, 0.0, 1.0);
// }

vec3 getCubeColor(float index) {
    vec3 col = vec3(1.0);  // Default white
    for(int i = 0; i < MAX_CUBES; i++) {
        if(float(i) == index) {
            col = cubeColors[i];
            break;
        }
    }
    return col;
}

void main()
{
    vec2 p = (-resolution.xy + 2.0*gl_FragCoord.xy)/resolution.y;
    
    // camera
    vec3 ro = viewEye;
    vec3 ta = viewCenter;
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww,vec3(0.0,1.0,0.0)));
    vec3 vv = normalize(cross(uu,ww));
    vec3 rd = normalize(p.x*uu + p.y*vv + 2.0*ww);

    // render
    vec3 col = vec3(5.7, 10.9, 1.0) + rd.y*0.8;
    float t = 0.0;
    MapResult res;
    
    // raymarch
    for(int i = 0; i < 32; i++) {
        vec3 pos = ro + rd * t;
        res = map(pos);
        
        if(res.dist < 0.001 || t > 10.0) break;
        t += res.dist;
    }

    if(t < 20.0) {
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormal(pos);
        vec3 lig = normalize(lightPos);
        
        // material
        vec3 matCol;
        if(res.material < 1.5) {
            matCol = vec3(0.3);  // ground color
        } else {
            matCol = getCubeColor(res.index);
        }

        // simplified lighting
        float dif = clamp(dot(nor, lig), 0.0, 1.0);
        float amb = 0.3;
        
        vec3 lin = vec3(0.0);
        lin += dif * vec3(1.0, 0.8, 0.6);
        lin += amb * vec3(1.0);
        
        col = matCol * lin;
        
        // fog
        col = mix(col, vec3(0.8, 0.9, 1.0), 1.0-exp(-0.0002*t*t*t));
    }

    // gamma
    col = pow(col, vec3(0.4545));
    
    gl_FragColor = vec4(col, 1.0);
}