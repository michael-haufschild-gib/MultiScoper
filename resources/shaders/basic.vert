#version 330 core
in vec2 position;
in float distFromCenter;

uniform mat4 projection;

out float vDistFromCenter;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vDistFromCenter = distFromCenter;
}
