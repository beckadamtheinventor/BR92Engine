VERTPROGRAM
    // Input vertex attributes
    in vec3 v;

    // Uniform values
    uniform mat4 mvp;
    uniform mat4 model;
    uniform float tno;
    uniform float scale;

    // Output vertex attributes (to fragment shader)
    out vec2 fragTexCoord;
    out vec2 lightTexCoord;

    void main()
    {
        // Unpack
        float x = 0;
        float y = v.x*scale;
        float z = v.y*scale;
        // transform
        vec4 pos = model*vec4(x, y, z, 1.0);

        // light map coordinate
        lightTexCoord = vec2(pos.x / 64.0, pos.z / 64.0);

        uint vno = uint(v.z);

        uint tt = uint(tno);
        float tx = float((tt & 0x3Fu) + (vno & 1u)) / 64.0;
        float ty = float(((tt >> 6u) & 0x3Fu) + (vno >> 1u)) / 64.0;
        // Calculate final vertex position
        gl_Position = mvp*pos;

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
    uniform sampler2D texture1;
    uniform vec4 FogColor;
    uniform float FogMin;
    uniform float FogMax;
    uniform float LightLevel;
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
        float x = (gl_FragCoord.x / renderwidth) * 3.141 - 1.5705;
        float d = sin(x) + gl_FragCoord.z / gl_FragCoord.w;
        float alpha = getFogFactor(d);
        vec4 texelColor = texture(texture0, fragTexCoord);
        // vec3 light = texture(texture1, lightTexCoord).rgb * LightLevel;
        texelColor.rgb *= LightLevel;
        vec4 fogColor = vec4(FogColor.rgb, texelColor.a);
        finalColor = mix(texelColor, fogColor, alpha);
    }
ENDPROGRAM