#pragma once
#include <cmath>

// Simple stereo biquad (RBJ cookbook). No allocations, safe on the audio thread.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

    float process (int ch, float x)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

    void set (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0;
        a1 = na1 / a0; a2 = na2 / a0;
    }

    static constexpr float pi = 3.14159265358979f;

    void lowpass (double fs, float f, float q)
    {
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void highpass (double fs, float f, float q)
    {
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void peak (double fs, float f, float q, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }

    void lowShelf (double fs, float f, float q, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w);
        const float be = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) - (A - 1) * c + be),
             2 * A * ((A - 1) - (A + 1) * c),
             A * ((A + 1) - (A - 1) * c - be),
             (A + 1) + (A - 1) * c + be,
             -2 * ((A - 1) + (A + 1) * c),
             (A + 1) + (A - 1) * c - be);
    }

    void highShelf (double fs, float f, float q, float dB)
    {
        const float A = std::pow (10.f, dB / 40.f);
        const float w = 2 * pi * f / (float) fs, c = std::cos (w);
        const float be = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) + (A - 1) * c + be),
             -2 * A * ((A - 1) + (A + 1) * c),
             A * ((A + 1) + (A - 1) * c - be),
             (A + 1) - (A - 1) * c + be,
             2 * ((A - 1) - (A + 1) * c),
             (A + 1) - (A - 1) * c - be);
    }
};
