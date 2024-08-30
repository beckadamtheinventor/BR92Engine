VERTPROGRAM
    // Input vertex attributes
    in uint vertexInfo1;

    // Uniform values
    uniform mat4 mvp;
    uniform vec3 drawPosition;

    // Output vertex attributes (to fragment shader)
    out vec2 fragTexCoord;
    out vec2 lightTexCoord;

    void main()
    {
        // Unpack
        float x = float((vertexInfo1 >> 20u) & 0xFFu);
        float y = float((vertexInfo1 >> 29u) & 1u);
        float z = float((vertexInfo1 >> 12u) & 0xFFu);

        // light map coordinate
        lightTexCoord = vec2(x / 64.0, z / 64.0);

        // offset the mesh chunk
        x += drawPosition.x;
        y += drawPosition.y;
        z += drawPosition.z;
        uint vno = (vertexInfo1 >> 30u) & 3u;

        uint tno = vertexInfo1 & 0xFFFu;
        float tx = float((tno & 0x3Fu) + (vno & 1u)) / 64.0;
        float ty = float(((tno >> 6u) & 0x3Fu) + (vno >> 1u)) / 64.0;
        // Calculate final vertex position
        gl_Position = mvp*vec4(x, y, z, 1.0);

        // Send vertex attributes to fragment shader
        fragTexCoord = vec2(tx, ty);
    }
ENDPROGRAM
FRAGPROGRAM
    // Input vertex attributes (from vertex shader)
    in vec2 fragTexCoord;
    in vec2 lightTexCoord;

    // Input uniform values
    uniform sampler2D texture0;
    uniform sampler2D texture1; // light map
    uniform vec4 FogColor;
    uniform float FogMin;
    uniform float FogMax;
    uniform float LightLevel;
    // uniform vec2 mapsize;
    uniform float renderwidth;

    // Output fragment color
    out vec4 finalColor;

    float getFogFactor(float d)
    {
        return smoothstep(FogMin, FogMax, d); // smooth interpolated
        // return clamp((d - FogMin) / (FogMax - FogMin), 0, 1); // linear interpolated (?)
    }

    void main()
    {
        float x = (gl_FragCoord.x / renderwidth) * 3.141;
        float d = gl_FragCoord.z / gl_FragCoord.w - sin(x);
        float alpha = getFogFactor(d);
        vec4 texelColor = texture(texture0, fragTexCoord);
        // vec3 light = texture(texture1, lightTexCoord).rgb * LightLevel;
        vec4 fogColor = vec4(FogColor.rgb, 1.0f);
        finalColor = mix(texelColor*vec4(LightLevel, LightLevel, LightLevel, 1.0), fogColor, alpha);
    }
ENDPROGRAM