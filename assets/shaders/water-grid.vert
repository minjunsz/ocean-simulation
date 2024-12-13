#version 410 core


out vec2 uv;

uniform uint gridLength;

void main() {
    // Compute the (u, v) grid coordinates based on vertexID
    float du = 1.0f / (gridLength - 1);
    float dv = 1.0f / (gridLength - 1);
    uv = vec2(mod(gl_VertexID, gridLength) * du, (gl_VertexID / gridLength) * dv);
}
