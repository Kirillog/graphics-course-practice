#include "perlin.h"
#include "grid.h"

float interpolate(float a0, float a1, float w) {
    return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
}

vec2 randomGradient(int ix, int iy, float t) {
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2;   // rotation width
    unsigned a = ix, b = iy;
    a *= 3284157443;
    b ^= a << s | a >> w - s;
    b *= 1911520717;
    a ^= b << s | b >> w - s;
    a *= 2048419325;
    float random = a * (3.14159265 / ~(~0u >> 1));   // in [0, 2*Pi]
    return {std::cos(random + t), std::sin(random + t)};
}

float dotGridGradient(int ix, int iy, float x, float y, float t) {
    vec2 gradient = randomGradient(ix, iy, t);

    float dx = x - (float) ix;
    float dy = y - (float) iy;

    return (dx * gradient.x + dy * gradient.y);
}

float perlin(float x, float y, float t) {
    int x0 = (int) floor(x);
    int x1 = x0 + 1;
    int y0 = (int) floor(y);
    int y1 = y0 + 1;

    float sx = x - (float) x0;
    float sy = y - (float) y0;

    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y, t);
    n1 = dotGridGradient(x1, y0, x, y, t);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y, t);
    n1 = dotGridGradient(x1, y1, x, y, t);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return (value + 1) / 2;
}
