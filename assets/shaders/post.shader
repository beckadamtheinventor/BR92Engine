VERTPROGRAM
    // https://stackoverflow.com/a/31501833
    out vec2 fragCoord;

    const vec2 pos[4]=vec2[4] (
        vec2(-1.0, 1.0),
        vec2(-1.0,-1.0),
        vec2( 1.0, 1.0),
        vec2( 1.0,-1.0)
    );
    void main() {
        fragCoord=0.5*pos[gl_VertexID] + vec2(0.5);
        gl_Position=vec4(pos[gl_VertexID], 0.0, 1.0);
    }
ENDPROGRAM
FRAGPROGRAM
    in vec2 fragCoord;
    uniform sampler2D screenTexture;
    uniform sampler2D fontTexture;
    out vec4 color;

    void main() {
        float d = gl_FragCoord.z / gl_FragCoord.w;
        vec4 col = texture(screenTexture, fragCoord);
        float lum = (0.2126*col.r + 0.7152*col.g + 0.0722*col.b);
        color = col * texture(fontTexture, vec2((lum+fragCoord.x)/16.0, fragCoord.y));
    }
ENDPROGRAM