#version 330 core

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
uniform float fogDensity;

uniform sampler2D heightmapTexture;

uniform float textureWidth;
uniform float textureHeight;

vec3 calculateAmbient(vec3 lightColor, float strength);
vec3 calculateDiffuse(vec3 lightColor);
float calculateFog(float density);

uniform float yScale;

void main()
{
    vec3 color;
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    float mode = rendersettings.x;
    float useWire = rendersettings.y;

   if (useWire < 0.5f) {
       vec2 texPos = vec2((FragPosition.x + 0.5 * textureWidth)/ (textureWidth),
                          (FragPosition.z + 0.5 * textureHeight)/ (textureHeight));

       if (doTexture > 0.5) color = texture(texture1, texPos).xyz;
       else color = terrainColor;

       vec3 ambient = calculateAmbient(lightColor, 0.5f);
       vec3 diffuse = calculateDiffuse(lightColor);
       color = (ambient + diffuse) * color;

       if (doFog > 0.5) {
           vec3 fogColour = skyColor;
           float fogFactor = calculateFog(fogDensity);
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
    vec2 texPos = vec2((FragPosition.x + 0.5 * textureWidth)/ (textureWidth),
                       (FragPosition.z + 0.5 * textureHeight)/ (textureHeight));

    /* Based on
     * https://www.slideshare.net/repii/terrain-rendering-in-frostbite-using-procedural-shader-splatting-presentation?type=powerpoint */
    float leftHeight = texture(heightmapTexture, texPos - vec2(1.0 / textureWidth, 0)).r;
    float rightHeight = texture(heightmapTexture, texPos + vec2(1.0 / textureWidth, 0)).r;
    float upHeight = texture(heightmapTexture, texPos + vec2(0, 1.0 / textureHeight)).r;
    float downHeight = texture(heightmapTexture, texPos - vec2(0, 1.0 / textureHeight)).r;

    float dx = (leftHeight - rightHeight) * yScale * 65535;
    float dz = (downHeight - upHeight) * yScale * 65535;

    vec3 normal = normalize(vec3(dx, 2.0f, dz));

    vec3 lightDir = normalize(-lightDirection);

    float diff = max(dot(normal, lightDir), 0.0f);
    return diff * lightColor;
}

/* The distance fog concept is based on the following resource:
 * https://opengl-notes.readthedocs.io/en/latest/topics/texturing/aliasing.html */
float calculateFog(float density) {
    float dist = length(cameraPos - FragPosition);
    float fogFactor = exp(-density * dist);
    return clamp(fogFactor, 0.0f, 1.0f);
}
