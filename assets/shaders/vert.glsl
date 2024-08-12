#version 330

// Input vertex attributes
in uint vertexInfo1;
in uint vertexInfo2;

// Uniform values
uniform mat4 mvp;
uniform vec3 drawPosition;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 vertColor;
out float lightLevel;

#define SCALE 2.0f;

void main()
{
    // Unpack
    float x = float((vertexInfo1 >> 8u) & 0xFFu);
    float y = float((vertexInfo1 >> 29u) & 1u);
    float z = float((vertexInfo1 >> 0u) & 0xFFu);
    x += drawPosition.x;
    y += drawPosition.y;
    z += drawPosition.z;
    x *= SCALE;
    y *= SCALE;
    z *= SCALE;
    float light = float(1u + ((vertexInfo1 >> 16u) & 0xFFu));
    uint vno = (vertexInfo1 >> 30u) & 3u;
    uint ri = (vertexInfo2 >> 12u) & 0xFu;
    uint gi = (vertexInfo2 >> 16u) & 0xFu;
    uint bi = (vertexInfo2 >> 20u) & 0xFu;

    uint tno = vertexInfo2 & 0xFFFu;
    float tx = float((tno & 0x3Fu) + (vno & 1u)) / 64.0;
    float ty = float(((tno >> 6u) & 0x3Fu) + (vno >> 1u)) / 64.0;
    // Calculate final vertex position
    gl_Position = mvp*vec4(x, y, z, 1.0);

    // Send vertex attributes to fragment shader
    fragTexCoord = vec2(tx, ty);
    lightLevel = light/128.0f;
    vertColor = vec4(float(ri)/15.0, float(gi)/15.0, float(bi)/15.0, 1.0);
}