#version 330 core
in vec2 position;
in float vParam; // 0 at baseline, 1 at waveform
in float tParam; // 0 to 1 along wave

uniform mat4 projection;

out float vV;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vV = vParam;
}
