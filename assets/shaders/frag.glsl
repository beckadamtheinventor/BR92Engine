#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec2 lightTexCoord;
in vec4 vertColor;
in float lightLevel;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 FogColor;
uniform float FogMin;
uniform float FogMax;
uniform float LightLevel;

// Output fragment color
out vec4 finalColor;

float getFogFactor(float d)
{
    if (d>=FogMax) return 1;
    if (d<=FogMin) return 0;

    return 1 - (FogMax - d) / (FogMax - FogMin);
}

void main()
{
    float d = gl_FragCoord.z / gl_FragCoord.w;
    float alpha = getFogFactor(d);
    vec4 texelColor = texture(texture0, fragTexCoord);
    texelColor *= vertColor;
    float light = lightLevel * LightLevel;
    vec4 fogColor = vec4(FogColor.rgb, 1.0f);
    finalColor = mix(texelColor * vec4(light, light, light, 1.0f), fogColor, alpha);
}

