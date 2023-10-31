#version 330 core

in float Mode;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec2 rendersettings;
uniform sampler2D texture1;

void main()
{
    vec4 color;
    float mode = rendersettings.x;
    float useWire = rendersettings.y;

   if (useWire < 0.5f) {
       color = texture(texture1, TexCoord);
    } else {
        if (mode < 0.5f) {
            color = vec4(0.2, 0.2, 0.2, 1.0);
        }
        else {
            color = vec4(0.8, 0.8, 0.8, 1.0);
        }
    }
    FragColor = color;
}
