#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
//out vec3 Position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float xzScale;
uniform float yScale;

void main()
{
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    //Position = (view * model * vec4(aPos, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(aPos.x * xzScale, aPos.y * yScale, aPos.z * xzScale, 1.0);
}
