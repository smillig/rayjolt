#version 330 core

// putting it all together; depth, sobel and squigly lines

// interesting breakdown of lots of concepts: https://www.shadertoy.com/view/lsSyRd


// stuff below https://www.shadertoy.com/view/MscSzf

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // color info
uniform sampler2D texture1; // depth info
uniform float time;
uniform vec3 edgeColor;
uniform float noiseAmount;
uniform float errorPeriod;
uniform float errorRange;
uniform int depthLineEnabled;
uniform int sobelEnabled;
uniform int depthViewEnabled;
uniform float depthNear; // depth contrast for near plane (close objects are black)
uniform float depthFar; // depth contrast for far plane (horizon is white)
uniform float sobelKernelSize; // adjust line thickness of sobel
uniform float depthSensitivity; // adjust line thickness of depth
uniform float depthEdgeThres;
uniform float sobelEdgeThres;


// random noise generation
float randomNoise(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43785.5453123);
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * depthNear * depthFar) / (depthFar + depthNear - z * (depthFar - depthNear));
}

float samplef(const int x, const int y, vec2 imageCoord)
{
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));

    vec2 offsetUV = imageCoord + (vec2(x, y) * texelSize * sobelKernelSize);
    vec3 color = texture(texture0, offsetUV).rgb;
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

bool checkEdge(vec2 uv)
{
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));
    float sobelEdge = 0.0;
    if (sobelEnabled == 1)
    {
         // sobel kernel pass
        float horizKern = samplef(-1, -1, uv) * 1.0 + samplef(0, -1, uv) * 2.0 + samplef(1, -1, uv) * 1.0
                        + samplef(-1, 1, uv) * -1.0 + samplef(0, 1, uv) * -2.0 + samplef(1, 1, uv) * -1.0;

        float vertKern = samplef(-1, -1, uv) * 1.0 + samplef(-1, 0, uv) * 2.0 + samplef(-1, 1, uv) * 1.0
                        + samplef(1, -1, uv) * -1.0 + samplef(1, 0, uv) * -2.0 + samplef(1, 1, uv) * -1.0;

        sobelEdge = sqrt(vertKern * vertKern + horizKern * horizKern);
    }

    float depthEdge = 0.0;
    if (depthLineEnabled == 1)
    {
        // depth search 
        float myDepth = LinearizeDepth(texture(texture1, uv).r);
        float depthN = LinearizeDepth(texture(texture1, uv + vec2(0.0, texelSize.y * depthSensitivity)).r);
        float depthS = LinearizeDepth(texture(texture1, uv - vec2(0.0, texelSize.y * depthSensitivity)).r);
        float depthE = LinearizeDepth(texture(texture1, uv + vec2(texelSize.x * depthSensitivity, 0.0)).r);
        float depthW = LinearizeDepth(texture(texture1, uv - vec2(texelSize.x * depthSensitivity, 0.0)).r);

        depthEdge = abs(myDepth - depthN) + abs(myDepth - depthS) + abs(myDepth - depthE) + abs(myDepth - depthW);
    }

    if (depthEdge > depthEdgeThres || sobelEdge > sobelEdgeThres)
    {
        return true; // edge found
    }
    return false; // no edge
}

void main()
{
    if (depthViewEnabled == 1)
    {
         float rawDepth = texture(texture1, fragTexCoord).r;
        float CurDepth = LinearizeDepth(rawDepth) / depthFar;
        finalColor = vec4(CurDepth, CurDepth, CurDepth, 1.0);
    }

    else
    {
        float noise = (randomNoise(fragTexCoord) - 0.5) * noiseAmount;

        // different attemt, generate noise on the UV rather than 3 passes
        vec2 displacedUV = fragTexCoord + vec2(
            errorRange * sin(errorPeriod * fragTexCoord.y + time) + noise,
            errorRange * sin(errorPeriod * fragTexCoord.x + time) + noise
        );

        vec4 baseColor = texture(texture0, fragTexCoord);

        if (checkEdge(displacedUV))
        {
            finalColor = vec4(edgeColor, 1.0);
        }
        else 
        {
            finalColor = baseColor;
        }
    }
    
    // original attempt; much more costly
    // check the edge of all 3 lines
    
    // // creates variance using 3 noisy coords
    // vec2 draws[3];
    // draws[0] = fragTexCoord + vec2(errorRange * sin(errorPeriod * fragTexCoord.y + 0.0)
    //              + noise, errorRange * sin(errorPeriod * fragTexCoord.x + 0.0) + noise);
    // draws[1] = fragTexCoord + vec2(errorRange * sin(errorPeriod * fragTexCoord.y + 1.047)
    //              + noise, errorRange * sin(errorPeriod * fragTexCoord.x + 3.142) + noise);
    // draws[2] = fragTexCoord + vec2(errorRange * sin(errorPeriod * fragTexCoord.y + 2.094)
    //              + noise, errorRange * sin(errorPeriod * fragTexCoord.x + 1.571) + noise);

    // vec4 baseColor = texture(texture0, fragTexCoord);

    // float edge0 = checkEdge(draws[0]) ? 1.0 : 0.0;
    // float edge1 = checkEdge(draws[1]) ? 1.0 : 0.0;
    // float edge2 = checkEdge(draws[2]) ? 1.0 : 0.0;
    // float finalEdge = edge0 * edge1 * edge2;
    
    // finalColor = mix(vec4(edgeColor, 1.0), baseColor, finalEdge);
}