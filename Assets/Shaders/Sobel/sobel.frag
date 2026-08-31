#version 330

// example from shader toy: https://www.shadertoy.com/view/4ss3Dr

// Sobel Kernel - Horizontal
//  1  2  1
//  0  0  0
// -1 -2 -1

// was looking into creating a larger kernel for thicker lines but we can fake it by multiplying 
// 2  4  6  4  2
// 1  2  4  2  1
// 0  0  0  0  0
//-1 -2 -4 -2 -1
//-2 -4 -6 -4 -2
// multiply in the samplef function UV to get the above kernel for fewer pixel lookups

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

const float near = 0.09;
const float far = 200.0;

const float lineThickness = 3.0;

// This function turns compressed GPU depth into linear distance
float LinearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0; // Back to Normalized Device Coordinates
	return (2.0 * near * far) / (far + near - z * (far - near));
}

// helper function to do get pixel at texture space
vec3 samplef(const int x, const int y, vec2 imageCoord)
{
    // translates image space to texile space (pixels of relative spacing)
    vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));
    // use the passed x and y to offset pixels
	vec2 offsetUV = imageCoord + (vec2(x,y) * texelSize * lineThickness);
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

    // pythagoras to the rescue
	return samplef(0, 0, imageCoord) * sqrt(luminance(vc*vc + hc*hc));
}

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(texture0, 0));
	
	float rawDepth = texture(texture1, fragTexCoord).r;
	float myDepth = LinearizeDepth(rawDepth);

	float depthN = LinearizeDepth(texture(texture1, fragTexCoord + vec2(0.0, texelSize.y)).r);
	float depthS = LinearizeDepth(texture(texture1, fragTexCoord - vec2(0.0, texelSize.y)).r);
	float depthE = LinearizeDepth(texture(texture1, fragTexCoord + vec2(texelSize.x, 0.0)).r);
	float depthW = LinearizeDepth(texture(texture1, fragTexCoord - vec2(texelSize.x, 0.0)).r);

	// depth edge detection
	float edge = abs(myDepth - depthN) + abs(myDepth - depthS) +
	abs(myDepth - depthE) + abs(myDepth - depthW);

	// read the color of the game
	vec4 baseColor = texture(texture0, fragTexCoord);
	// sobel filter
	vec3 color = filterf(fragTexCoord);
	// check both depth threshold and sobel
	if (edge > 0.2 && (color.r > 0.25 || color.g > 0.25 || color.b > 0.25))
	{
		finalColor = vec4(0.0, 0.0, 0.0, 1.0); // Draw Black Line
	}
	else
	{
		finalColor = baseColor; // original color
	}
}
// next thing to implement:

//#define EdgeColor vec4(0.2, 0.2, 0.15, 1.0)
//#define BackgroundColor vec4(1,0.95,0.85,1)
//#define NoiseAmount 0.01
//#define ErrorPeriod 30.0
//#define ErrorRange 0.003


//void mainImage( out vec4 fragColor, in vec2 fragCoord )
//{
//	float time = floor(iTime * 16.0) / 16.0;
//	vec2 uv = fragCoord.xy / iResolution.xy;
//
//	float noise = (texture(iChannel1, uv * 0.5).r - 0.5) * NoiseAmount;
//	vec2 uvs[3];
//	uvs[0] = uv + vec2(ErrorRange * sin(ErrorPeriod * uv.y + 0.0) + noise, ErrorRange * sin(ErrorPeriod * uv.x + 0.0) + noise);
//	uvs[1] = uv + vec2(ErrorRange * sin(ErrorPeriod * uv.y + 1.047) + noise, ErrorRange * sin(ErrorPeriod * uv.x + 3.142) + noise);
//	uvs[2] = uv + vec2(ErrorRange * sin(ErrorPeriod * uv.y + 2.094) + noise, ErrorRange * sin(ErrorPeriod * uv.x + 1.571) + noise);
//
//	float edge = texture(iChannel0, uvs[0]).r * texture(iChannel0, uvs[1]).r * texture(iChannel0, uvs[2]).r;
//	float diffuse = texture(iChannel0, uv).g;
//
//	float w = fwidth(diffuse) * 2.0;
//	vec4 mCol = mix(BackgroundColor * 0.5, BackgroundColor, mix(0.0, 1.0, smoothstep(-w, w, diffuse - 0.3)));
//	fragColor = mix(EdgeColor, mCol, edge);
//	//fragColor = vec4(diffuse);
//}