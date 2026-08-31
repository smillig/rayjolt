#version 330

// example from shader toy: https://www.shadertoy.com/view/4ss3Dr

// Sobel Kernel - Horizontal
//  1  2  1
//  0  0  0
// -1 -2 -1

// example scenerio of how this works:
// Flat color - no edge - all pixels around sample pixel are the same (or very close)
// top row: (1.0 * 1) + (1.0 * 2) + (1.0 * 1) = 4.0
// bottom row: (1.0 * -1) + (1.0 * -2) + (1.0 * -1) = -4.0
// result: 4.0 + -4.0 = 0.0
// no edge

// Strong change in color - edge detected
// top row: (1.0 * 1) + (1.0 * 2) + (1.0 * 1) = +4.0
// bottom row: (0.0 * -1) + (0.0 * -2) + (0.0 * -1) = 0.0
// result: +4.0 + 0.0 = +4.0
// edge

// Sobel Kernel - Vertical
//  1  0 -1
//  2  0 -2
//  1  0 -1

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // color Texture
uniform sampler2D texture1; // Dept not used for this example but I might see if I can us it later

// helper function to do get pixel at texture space
vec3 samplef(const int x, const int y, vec2 imageCoord)
{
    // translates image space to texile space (pixels of relative spacing)
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));
    // use the passed x and y to offset pixels
	vec2 offsetUV = imageCoord + (vec2(x,y) * texelSize);
    // sample color at said offset
	return texture(texture0, offsetUV).rgb;
}

float luminance(vec3 c)
{
    // normalizes the color to a strong green
	// return dot(c, vec3(0.2126, 0.7152, 0.0722));
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// sobel filter function
vec3 filterf(vec2 imageCoord)
{
    // no need to check -1,0 or 1,0 since they will always be 0
    // checks for horizontal edges
	vec3 hc =samplef(-1,-1, imageCoord) *  1.0 + samplef( 0,-1, imageCoord) *  2.0
		 	+samplef( 1,-1, imageCoord) *  1.0 + samplef(-1, 1, imageCoord) * -1.0
		 	+samplef( 0, 1, imageCoord) * -2.0 + samplef( 1, 1, imageCoord) * -1.0;
    // no need to check 0,-1 or 0,1 those will be 0 as well
    // checks for vertical edges
    vec3 vc =samplef(-1,-1, imageCoord) *  1.0 + samplef(-1, 0, imageCoord) *  2.0
		 	+samplef(-1, 1, imageCoord) *  1.0 + samplef( 1,-1, imageCoord) * -1.0
		 	+samplef( 1, 0, imageCoord) * -2.0 + samplef( 1, 1, imageCoord) * -1.0;

    // pythagoras to the rescue (pow of 0.6 insteadof sqrt since we get a little more thick lines with > 0.5)
	return samplef(0, 0, imageCoord) * pow(luminance(vc*vc + hc*hc), 0.6);
}

void main()
{
    // filter on our current pixel
    vec3 color = filterf(fragTexCoord);
    // pass out the final color
    finalColor = vec4(color, 1.0);
}