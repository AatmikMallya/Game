#ifndef UTILITY_GLSL
#define UTILITY_GLSL

vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

uint compress_color16(vec3 color) {
    // Convert RGB to HSV
    vec3 hsv = rgb2hsv(color);
    
    // H: 7 bits, S: 4 bits, V: 5 bits
    uint h = uint(hsv.x * 128.0);
    uint s = uint(hsv.y * 16.0);
    uint v = uint(hsv.z * 32.0);
    
    // Pack into a single uint
    return (h << 9) | (s << 5) | v;
}

vec3 decompress_color16(uint packedColor) {
    // Extract H, S, V components
    uint h = (packedColor >> 9) & 0x7F; // 7 bits for hue
    uint s = (packedColor >> 5) & 0x0F; // 4 bits for saturation
    uint v = packedColor & 0x1F;        // 5 bits for value
    
    // Convert back to RGB
    vec3 hsv = vec3(float(h) / 128.0, float(s) / 16.0, float(v) / 32.0);
    return hsv2rgb(hsv);
}

#endif //UTILITY_GLSL