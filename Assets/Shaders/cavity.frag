//#version 330
//
//// Input vertex attributes (from vertex shader)
//in vec2 fragTexCoord;
//
//// Input uniform values
//uniform sampler2D texture1;
//uniform bool flipY;
//
//const float nearPlane = 0.2;
//const float farPlane = 300.0;
//
//// Output fragment color
//out vec4 finalColor;
//
//void main()
//{
//    // Handle potential Y-flipping
//    vec2 texCoord = fragTexCoord;
//    if (flipY) texCoord.y = 1.0 - texCoord.y;
//
//    // Sample depth
//    float depth = texture(texture1, texCoord).r;
//
//    // Linearize depth value
//    float linearDepth = (2.0*nearPlane)/(farPlane + nearPlane - depth*(farPlane - nearPlane));
//
//    // Output final color
//    finalColor = vec4(vec3(linearDepth), 1.0);
//}

#version 330 core

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // The Color Texture
uniform sampler2D texture1; // The Depth Texture

// Raylib default camera clipping planes
const float near = 0.09;
const float far = 200.0;

// This function turns compressed GPU depth into linear distance
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to Normalized Device Coordinates
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
//    // 1. Get the raw GPU depth (This will be 0.999...)
//    float rawDepth = texture(texture1, fragTexCoord).r;
//
//    // 2. Un-curve it!
//    float linearDepth = LinearizeDepth(rawDepth);
//
//    // 3. Right now, linearDepth is the literal distance in meters (e.g. 17.0).
//    // We want to draw it as a color between 0.0 (Black) and 1.0 (White).
//    // Let's divide by 50.0 so anything 50 meters away is white, and 0 meters is black.
//    float visualDepth = linearDepth / 50.0;
//
//    // DEBUG: Draw the Blender-style depth map!
//    finalColor = vec4(visualDepth, visualDepth, visualDepth, 1.0);
//    float myDepth = texture(texture1, fragTexCoord).r;
//
//    // DEBUG: Draw the raw depth value as a greyscale color!
//    finalColor = vec4(myDepth, myDepth, myDepth, 1.0);
    // 1. Get our screen resolution so we can look exactly 1 pixel away
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));

    // 2. Read the raw depth and linearize it!
    float rawDepth = texture(texture1, fragTexCoord).r;
    float myDepth = LinearizeDepth(rawDepth);

    // 3. Look at the pixels North, South, East, and West of us
    float depthN = LinearizeDepth(texture(texture1, fragTexCoord + vec2(0.0, texelSize.y)).r);
    float depthS = LinearizeDepth(texture(texture1, fragTexCoord - vec2(0.0, texelSize.y)).r);
    float depthE = LinearizeDepth(texture(texture1, fragTexCoord + vec2(texelSize.x, 0.0)).r);
    float depthW = LinearizeDepth(texture(texture1, fragTexCoord - vec2(texelSize.x, 0.0)).r);

    // 4. Edge Detection Math (Sobel-style)
    // How different am I from my neighbors?
    float edge = abs(myDepth - depthN) + abs(myDepth - depthS) +
    abs(myDepth - depthE) + abs(myDepth - depthW);

    // 5. Read the actual color of the game
    vec4 baseColor = texture(texture0, fragTexCoord);

    // 6. If we found a steep cliff (an edge), color it black!
    // Tweak the 0.5 value to make the lines thicker or thinner
    if (edge > 0.5)
    {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0); // Draw Black Line
    }
    else
    {
        finalColor = baseColor; // Draw Normal Game Color
    }
}