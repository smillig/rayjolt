#version 330

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
    lightDot += lights[0].color.rgb * NdotL;

    float specCo = 0.0;
    if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 64.0); // shinyness is 16 instead of a uniform
    specular += specCo;

    finalColor = (texelColor * ((tint + vec4(specular, 1.0)) * vec4(lightDot, 1.0)));
    finalColor += texelColor * ambient * tint;

    // gamma correction
    finalColor = pow(finalColor, vec4(1.0 / 2.2));
}