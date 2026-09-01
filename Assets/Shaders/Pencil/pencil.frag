#version 330 core

// putting it all together; depth, sobel and squigly lines

// interesting breakdown of lots of concepts: https://www.shadertoy.com/view/lsSyRd


// stuff below https://www.shadertoy.com/view/MscSzf

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // color info
uniform sampler2D texture1; // depth info
uniform float time;

const float near = 0.09; // depth contrast for near plane (close objects are black)
const float far = 100.0; // depth contrast for far plane (horizon is white)

// Wobbly line variables
const vec4 EdgeColor = vec4(0.2, 0.2, 0.25, 1.0);
const float NoiseAmount = 0.001; // default 0.002 - how fuzzy the lines are
const float ErrorPeriod = 5.0; // default 30.0  - how wavy the lines are
const float ErrorRange = 0.0015; // default 0.003 - how offset the different lines are from each other

const float kernelSize = 1.5;       // adjust line thickness of sobel
const float depthSensitivity = 2.0; // adjust line thickness of depth

// random noise generation
float randomNoise(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43785.5453123);
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

vec3 samplef(const int x, const int y, vec2 imageCoord)
{
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));

    vec2 offsetUV = imageCoord + (vec2(x, y) * texelSize * kernelSize);
    return texture(texture0, offsetUV).rgb;
}

float luminance(vec3 c)
{
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

float checkEdge(vec2 uv)
{
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));

    // depth search 
    float myDepth = LinearizeDepth(texture(texture1, uv).r);
    float depthN = LinearizeDepth(texture(texture1, uv + vec2(0.0, texelSize.y * depthSensitivity)).r);
    float depthS = LinearizeDepth(texture(texture1, uv - vec2(0.0, texelSize.y * depthSensitivity)).r);
    float depthE = LinearizeDepth(texture(texture1, uv + vec2(texelSize.x * depthSensitivity, 0.0)).r);
    float depthW = LinearizeDepth(texture(texture1, uv - vec2(texelSize.x * depthSensitivity, 0.0)).r);

    float depthEdge = abs(myDepth - depthN) + abs(myDepth - depthS) + abs(myDepth - depthE) + abs(myDepth - depthW);

    // sobel kernel pass
    vec3 horizKern = samplef(-1, -1, uv) * 1.0 + samplef(0, -1, uv) * 2.0 + samplef(1, -1, uv) * 1.0
                    + samplef(-1, 1, uv) * -1.0 + samplef(0, 1, uv) * -2.0 + samplef(1, 1, uv) * -1.0;

    vec3 vertKern = samplef(-1, -1, uv) * 1.0 + samplef(-1, 0, uv) * 2.0 + samplef(-1, 1, uv) * 1.0
                    + samplef(1, -1, uv) * -1.0 + samplef(1, 0, uv) * -2.0 + samplef(1, 1, uv) * -1.0;

    float sobelEdge = luminance(vertKern * vertKern + horizKern * horizKern);

    if (depthEdge > 0.3 && sobelEdge > 0.15)
    {
        return 0.0; // edge found
    }
    return 1.0; // no edge
}

void main()
{
    float noise = (randomNoise(fragTexCoord) - 0.5) * NoiseAmount;

    // creates variance using 3 noisy coords
    vec2 draws[3];
    draws[0] = fragTexCoord + vec2(ErrorRange * sin(ErrorPeriod * fragTexCoord.y + 0.0)
                 + noise, ErrorRange * sin(ErrorPeriod * fragTexCoord.x + 0.0) + noise);
    draws[1] = fragTexCoord + vec2(ErrorRange * sin(ErrorPeriod * fragTexCoord.y + 1.047)
                 + noise, ErrorRange * sin(ErrorPeriod * fragTexCoord.x + 3.142) + noise);
    draws[2] = fragTexCoord + vec2(ErrorRange * sin(ErrorPeriod * fragTexCoord.y + 2.094)
                 + noise, ErrorRange * sin(ErrorPeriod * fragTexCoord.x + 1.571) + noise);

    // check the edge of all 3 lines
    float edge = checkEdge(draws[0]) * checkEdge(draws[1]) * checkEdge(draws[2]);

    vec4 baseColor = texture(texture0, fragTexCoord);

    finalColor = mix(EdgeColor, baseColor, edge);
}