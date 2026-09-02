#version 330

// used the default lighting example from raylib and converted it to toon
// used https://roystan.net/articles/toon-shader/ as an guide to convert the lighting shader

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

// Input lighting values
uniform Light lights[1];
uniform vec4 ambient;
uniform vec3 viewEye;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewEye - fragPosition);
    vec3 specular = vec3(0.0);

    vec4 tint = colDiffuse * fragColor;

    vec3 light = normalize(lights[0].position - fragPosition);

    float NdotL = max(dot(normal, light), 0.0);
    // add toon effect by cutting off the dot negative from positive
    // even better effect from smootstep
    float lightIntensity = smoothstep(-0.1, 0.1, NdotL);

    // original lighting before toonifying it:
    // lightDot += lights[0].color.rgb * NdotL;
    lightDot += lights[0].color.rgb * lightIntensity;

    float specCo = 0.0;
    if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 32.0); // shinyness is 32 instead of a uniform
    specular += smoothstep(0.1, 0.2, specCo);

    // rim lighting - 1 minus the dot of view and normal gives the edges facing away from the camera
    float rimLight = 1.0 - dot(viewD, normal);
    // multiply the rimlight by the Blinn-Phong rasing it to the power of 0.1 gives a nice curve facing the light source
    float rimIntenstiy = rimLight * pow(NdotL, 0.1);
    // smoothe the effect for a more cartoony look
    rimIntenstiy = smoothstep(0.6, 0.7, rimIntenstiy);
    // add tint to the specular then multiply by dot product of the light and then mutiply by texelColor
    finalColor = (texelColor * ((tint + vec4(specular, 1.0)) * vec4(lightDot, 1.0)));
    // add our rimLighting to the product of our color, ambient and tint
    finalColor += (texelColor * ambient * tint) + vec4(vec3(rimIntenstiy), 1.0);

    // gamma correction
    finalColor = pow(finalColor, vec4(1.0 / 2.2));
}