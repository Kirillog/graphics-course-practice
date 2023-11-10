#version 330 core

const vec2 VERTICES[6] = vec2[6](
    vec2(-1.0, -1.0),
    vec2(-0.5, -1.0),
    vec2(-1.0, -0.5),
    vec2(-1.0, -0.5),
    vec2(-0.5, -1.0),
    vec2(-0.5, -0.5)
);

const vec2 TEXCOORD[6] = vec2[6](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0)
);

out vec2 texcoord;

void main()
{
    texcoord = TEXCOORD[gl_VertexID];
    gl_Position = vec4(VERTICES[gl_VertexID], 0.0, 1.0);
}