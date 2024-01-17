#version 330 core
layout (location = 0) in vec2 aPos;
//layout (location = 1) in vec3 aOffset;


out vec3 FragPosition;


uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 normalMatrix;
uniform vec2 offset;

uniform sampler2D heightmapTexture;
//uniform usampler2D heightmapTexture;

uniform float textureWidth;
uniform float textureHeight;
uniform float cutOffX;
uniform float cutOffY;

uniform float maxT;
uniform float minT;

//uniform float xzScale;
/*uniform float yScale;*/
uniform float yShift;

void main()
{
    //TexCoord = vec2(aTexCoord.x, aTexCoord.y);

    vec2 texPos =  vec2((aPos.x + offset.x + 0.5 * textureWidth) / (textureWidth),
                        (aPos.y + offset.y + 0.5 * textureHeight) / (textureHeight));

    //vec2 texPos = int(aPos.x) % 2 == 0 && int(aPos.y) % 2 == 0 ? vec2(0.1,0.1) : vec2(0.8,0.8);
    //vec4 height = texelFetch(heightmapTexture, ivec2(aPos + offset), 0);

    float height = texture(heightmapTexture, texPos).r;
    //vec4 height = vec4(0,0,0,0);
    float y =  /*minT +*/ yShift + height * 65535 /** yScale*/; //*((maxT-minT)/30);
    vec3 actualPos = vec3(aPos.x + offset.x, y, aPos.y + offset.y);
    FragPosition = vec3(model * vec4(actualPos, 1.0));
    gl_Position = projection * view * model * vec4(actualPos, 1.0);
}
