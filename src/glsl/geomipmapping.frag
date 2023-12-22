#version 330 core

in float Mode;
in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPosition;

out vec4 FragColor;

uniform vec2 rendersettings;
uniform vec4 inColor;
uniform sampler2D texture1;
uniform vec3 lightDirection;
uniform vec3 cameraPos;
uniform float doTexture;
uniform vec3 skyColor;
uniform vec3 terrainColor;
uniform float doFog;

vec3 calculateAmbient(vec3 lightColor, float strength);
vec3 calculateDiffuse(vec3 lightColor);
float calculateFog(float density);

void main()
{
    vec3 color;
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    float mode = rendersettings.x;
    float useWire = rendersettings.y;

   if (useWire < 0.5f) {
       if (doTexture > 0.5) color = texture(texture1, TexCoord).xyz;
       else color = terrainColor;

       vec3 ambient = calculateAmbient(lightColor, 0.5f);
       vec3 diffuse = calculateDiffuse(lightColor);
       color = (ambient + diffuse) * color;

       if (doFog > 0.5) {
           vec3 fogColour = skyColor;
           float fogFactor = calculateFog(0.0005);
           color = mix(fogColour, color, fogFactor);
       }

    } else {
        color = inColor.xyz;
    }
    FragColor = vec4(color, 1.0f);
}

vec3 calculateAmbient(vec3 lightColor, float strength) {
    return strength * lightColor;
}

vec3 calculateDiffuse(vec3 lightColor) {
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-lightDirection);

    float diff = max(dot(norm, lightDir), 0.0f);
    return diff * lightColor;
}

/* The distance fog concept is based on the following resource:
 * https://opengl-notes.readthedocs.io/en/latest/topics/texturing/aliasing.html */
float calculateFog(float density) {
    float dist = length(cameraPos - FragPosition);
    float fogFactor = exp(-density * dist);
    return clamp(fogFactor, 0.0f, 1.0f);
}

