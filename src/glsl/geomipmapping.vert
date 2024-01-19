#version 330 core
layout (location = 0) in vec2 aPos;

out vec3 FragPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec2 offset;
uniform sampler2D heightmapTexture;
uniform float textureWidth;
uniform float textureHeight;

void main()
{
    vec2 texPos =  vec2((aPos.x + offset.x + 0.5 * textureWidth) / (textureWidth),
                        (aPos.y + offset.y + 0.5 * textureHeight) / (textureHeight));

    float height = texture(heightmapTexture, texPos).r;
    float y = height * 65535;

    vec3 actualPos = vec3(aPos.x + offset.x, y, aPos.y + offset.y);

    FragPosition = vec3(model * vec4(actualPos, 1.0));
    gl_Position = projection * view * model * vec4(actualPos, 1.0);
}
