#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

// ----------------------------------------------------------------------------
//  Small helpers
// ----------------------------------------------------------------------------
namespace sg
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;

struct Rng
{
    uint32_t s = 0x1234567u;
    explicit Rng (uint32_t seed = 0x1234567u) : s (seed ? seed : 1u) {}
    inline uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float uni()     { return (next() >> 8) * (1.0f / 16777216.0f); }   // 0..1
    inline float bi()      { return uni() * 2.0f - 1.0f; }                    // -1..1
};

// Simple one-pole lowpass / smoother
struct OnePole
{
    float a = 0.0f, z = 0.0f;
    void setTime (float seconds, double sr) { a = std::exp (-1.0f / (float) (seconds * sr)); }
    inline float process (float x) { z = x + a * (z - x); return z; }
};

// DC blocker
struct DCBlock
{
    float x1 = 0, y1 = 0, r = 0.9995f;
    void prepare (double sr) { r = 1.0f - (float) (twoPi * 10.0 / sr); x1 = y1 = 0; }
    inline float process (float x) { float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
};

// Topology-preserving state variable lowpass (one channel)
struct SVFLow
{
    float g = 0, k = 1.4f, ic1 = 0, ic2 = 0;
    double sr = 48000;
    void prepare (double s) { sr = s; ic1 = ic2 = 0; }
    void set (float hz, float q = 0.707f)
    {
        hz = juce::jlimit (20.0f, (float) (sr * 0.45), hz);
        g = std::tan (juce::MathConstants<float>::pi * hz / (float) sr);
        k = 1.0f / q;
    }
    inline float process (float x)
    {
        float a1 = 1.0f / (1.0f + g * (g + k));
        float v3 = x - ic2;
        float v1 = a1 * ic1 + g * a1 * v3;
        float v2 = ic2 + g * v1;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v2;
    }
};

// Biquad peak filter (RBJ)
struct Peak
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (double sr, float hz, float q, float gainDb)
    {
        float A = std::pow (10.0f, gainDb / 40.0f);
        float w = twoPi * hz / (float) sr, c = std::cos (w), s = std::sin (w), al = s / (2.0f * q);
        float a0 = 1 + al / A;
        b0 = (1 + al * A) / a0; b1 = -2 * c / a0; b2 = (1 - al * A) / a0;
        a1 = -2 * c / a0;       a2 = (1 - al / A) / a0;
    }
    inline float process (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

inline float hermite (const float* buf, int size, float pos)
{
    int i = (int) std::floor (pos);
    float f = pos - (float) i;
    auto at = [&] (int k) { k %= size; if (k < 0) k += size; return buf[k]; };
    float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
    float c1 = 0.5f * (x1 - xm1);
    float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * f + c2) * f + c1) * f + x0;
}

// ----------------------------------------------------------------------------
//  LO-FI: sample-rate reduction, bit crushing, dusty crackle, tone filter
//  v2: mu-law style crusher (quiet note tails stay grainy instead of
//  sputtering), anti-alias pre-filter, much quieter vinyl-style dust.
// ----------------------------------------------------------------------------
struct LoFi
{
    double sr = 48000;
    float phase = 0, held[2] {};
    SVFLow tone[2], pre[2], dustTone[2];
    float popEnv = 0, popPan = 0.5f, popPrev = 0;
    Rng rng { 0xBEEF };

    void prepare (double s)
    {
        sr = s; phase = 0; held[0] = held[1] = 0; popEnv = 0; popPrev = 0;
        for (int c = 0; c < 2; ++c) { tone[c].prepare (s); pre[c].prepare (s); dustTone[c].prepare (s); dustTone[c].set (3200.0f, 0.7f); }
    }

    void setTone (float hz) { for (auto& t : tone) t.set (hz, 0.75f); }
    void setRate (float rateHz) { for (auto& f : pre) f.set (std::min (rateHz * 0.48f, (float) sr * 0.45f), 0.6f); }

    static inline float crush (float x, float levels)
    {
        // companded quantiser: steps are fine near silence, coarse when loud
        constexpr float mu = 40.0f;
        const float k = 1.0f / std::log (1.0f + mu);
        x = juce::jlimit (-1.0f, 1.0f, x);
        float a = std::log (1.0f + mu * std::abs (x)) * k;
        a = std::round (a * levels) / levels;
        float y = (std::exp (a / k) - 1.0f) / mu;
        return x < 0 ? -y : y;
    }

    inline void process (float& l, float& r, float rateHz, float bits, float crackle)
    {
        float in[2] = { pre[0].process (l), pre[1].process (r) };
        phase += rateHz / (float) sr;
        if (phase >= 1.0f || rateHz >= (float) sr * 0.98f)
        {
            phase -= std::floor (phase);
            held[0] = in[0]; held[1] = in[1];
        }
        const float levels = std::pow (2.0f, bits - 1.0f);

        // dust: occasional soft ticks, never louder than about -36 dBFS
        float dust[2] = { 0, 0 };
        if (crackle > 0.0f)
        {
            if (rng.uni() < crackle * crackle * 5.0f / (float) sr)
            {
                popEnv = (0.004f + 0.012f * rng.uni() * rng.uni()) * crackle;
                popPan = rng.uni();
            }
            float n = rng.bi() * popEnv;
            popEnv *= 0.93f;
            float tick = n - popPrev; popPrev = n;
            for (int c = 0; c < 2; ++c)
            {
                float pan = c == 0 ? 1.0f - popPan : popPan;
                dust[c] = dustTone[c].process (tick * (0.4f + 0.6f * pan) + rng.bi() * 0.0006f * crackle);
            }
        }

        float out[2];
        for (int c = 0; c < 2; ++c)
            out[c] = tone[c].process (crush (held[c], levels)) + dust[c];
        l = out[0]; r = out[1];
    }
};

// ----------------------------------------------------------------------------
//  CASSETTE: wow, flutter, head bump, HF loss, saturation, hiss, dropouts.
//  Always outputs with a fixed latency of baseDelay samples (also for its
//  "dry" output) so the parallel mix never comb-filters.
// ----------------------------------------------------------------------------
struct Cassette
{
    double sr = 48000;
    std::vector<float> buf[2];
    int size = 0, w = 0;
    float baseDelay = 0;        // samples
    float wowPh = 0, wowPh2 = 0, flPh = 0, flPh2 = 0;
    float drift = 0, driftTarget = 0; int driftCount = 0;
    SVFLow hfLoss[2]; Peak bump[2]; SVFLow hissTone[2];
    float hissPrev[2] {};
    float dropGain = 1, dropTarget = 1; int dropCount = 0;
    OnePole dropSmooth;
    Rng rng { 0xCA55E77E };

    int latency() const { return (int) baseDelay; }

    void prepare (double s)
    {
        sr = s;
        size = (int) (s * 0.1) + 8;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        w = 0;
        baseDelay = (float) std::round (s * 0.006);       // 6 ms centre for the wow/flutter swing
        for (int c = 0; c < 2; ++c) { hfLoss[c].prepare (s); hissTone[c].prepare (s); hissTone[c].set (6000.0f, 0.6f); bump[c].set (s, 95.0f, 0.9f, 2.5f); }
        dropSmooth.setTime (0.012f, s); dropSmooth.z = 1.0f;
        drift = driftTarget = 0; driftCount = 0; dropGain = dropTarget = 1;
    }

    void setAge (float age)
    {
        float hz = 17000.0f * std::pow (4200.0f / 17000.0f, age);
        for (auto& f : hfLoss) f.set (hz, 0.62f);
        for (int c = 0; c < 2; ++c) bump[c].set (sr, 95.0f, 0.9f, 1.5f + 3.0f * age);
    }

    // in: stereo frame; outputs wet (tape) and dryDelayed (aligned, unprocessed)
    // bars >= 0: host is playing, lock the wow to the bar (one swing per bar)
    inline void process (float l, float r, float wow, float flutter, float hiss, float age,
                         float& wetL, float& wetR, float& dryL, float& dryR, double bars = -1.0)
    {
        buf[0][(size_t) w] = l; buf[1][(size_t) w] = r;

        // slow random drift for the wow so it never sounds like a pure LFO
        if (--driftCount <= 0) { driftTarget = rng.bi(); driftCount = (int) (sr * (0.3 + 0.7 * rng.uni())); }
        drift += (driftTarget - drift) * (float) (2.0 / sr);

        const bool synced = bars >= 0.0;
        if (synced)
        {
            wowPh  = (float) (bars - std::floor (bars));
            wowPh2 = (float) (bars * 0.5 - std::floor (bars * 0.5));
        }
        else
        {
            wowPh  += 0.55f / (float) sr;  if (wowPh  >= 1) wowPh  -= 1;
            wowPh2 += 0.23f / (float) sr;  if (wowPh2 >= 1) wowPh2 -= 1;
        }
        flPh   += 7.3f / (float) sr;   if (flPh   >= 1) flPh   -= 1;
        flPh2  += 11.7f / (float) sr;  if (flPh2  >= 1) flPh2  -= 1;

        const float ms = (float) sr * 0.001f;
        float wowMod = wow * wow * 3.2f * ms * (0.55f * std::sin (twoPi * wowPh) + 0.25f * std::sin (twoPi * wowPh2) + (synced ? 0.12f : 0.35f) * drift);
        float flMod  = flutter * 0.22f * ms * (0.7f * std::sin (twoPi * flPh) + 0.3f * std::sin (twoPi * flPh2));
        float d = juce::jlimit (2.0f, baseDelay * 1.95f, baseDelay + wowMod + flMod);

        float rp = (float) w - d;
        float tl = hermite (buf[0].data(), size, rp);
        float tr = hermite (buf[1].data(), size, rp + 0.0f);

        // dropouts: oxide wear makes the level dip now and then
        if (--dropCount <= 0)
        {
            if (dropTarget < 1.0f) { dropTarget = 1.0f; dropCount = (int) (sr * (0.4 + 2.0 * rng.uni())); }
            else if (rng.uni() < age * age * 0.6f) { dropTarget = 0.35f + 0.5f * rng.uni(); dropCount = (int) (sr * (0.02 + 0.07 * rng.uni())); }
            else dropCount = (int) (sr * 0.25);
        }
        dropGain = dropSmooth.process (dropTarget);

        float drive = 1.0f + age * 1.6f;
        float makeUp = 1.0f / std::tanh (drive * 0.5f) * 0.5f;
        float io[2] = { tl, tr };
        for (int c = 0; c < 2; ++c)
        {
            float x = std::tanh (io[c] * drive) * makeUp;
            x = bump[c].process (x);
            x = hfLoss[c].process (x);
            float n = rng.bi();
            float hissS = hissTone[c].process (n - hissPrev[c]); hissPrev[c] = n;
            x = x * dropGain + hissS * hiss * hiss * 0.045f;
            io[c] = x;
        }
        wetL = io[0]; wetR = io[1];

        int dp = w - (int) baseDelay; if (dp < 0) dp += size;
        dryL = buf[0][(size_t) dp]; dryR = buf[1][(size_t) dp];

        if (++w >= size) w = 0;
    }
};

// ----------------------------------------------------------------------------
//  TUBE: asymmetric triode-style saturation with sag (bloom) and warmth
// ----------------------------------------------------------------------------
struct Tube
{
    double sr = 48000;
    float env = 0, atk = 0, rel = 0;
    SVFLow warm[2]; DCBlock dc[2];

    void prepare (double s)
    {
        sr = s; env = 0;
        atk = std::exp (-1.0f / (float) (0.004 * s));
        rel = std::exp (-1.0f / (float) (0.15 * s));
        for (int c = 0; c < 2; ++c) { warm[c].prepare (s); dc[c].prepare (s); }
    }

    void setWarmth (float w01)
    {
        float hz = 14000.0f * std::pow (2200.0f / 14000.0f, w01);
        for (auto& f : warm) f.set (hz, 0.6f);
    }

    inline void process (float& l, float& r, float drive, float bias, float sag)
    {
        float k = juce::Decibels::decibelsToGain (drive * 32.0f);
        float lvl = std::max (std::abs (l), std::abs (r));
        env = lvl > env ? lvl + atk * (env - lvl) : lvl + rel * (env - lvl);
        float sagGain = 1.0f / (1.0f + sag * 6.0f * env * std::sqrt (k));
        float b = bias * 0.7f;
        float tb = std::tanh (b);
        // makeup keeps the level roughly steady as drive goes up
        float makeUp = 1.0f / (0.25f + 0.75f * std::tanh (k * 0.35f)) * (1.0f + sag * 0.8f)
                     * juce::Decibels::decibelsToGain (2.5f - 10.0f * drive);
        float io[2] = { l, r };
        for (int c = 0; c < 2; ++c)
        {
            float u = io[c] * k * sagGain;
            // triode-ish: positive side saturates harder than negative (even harmonics)
            float y = std::tanh (u + b) - tb;
            y = y > 0 ? y : y * (1.0f - 0.25f * bias) + 0.25f * bias * std::tanh (y * 1.5f);
            y = dc[c].process (y * makeUp * 0.42f);
            io[c] = warm[c].process (y);
        }
        l = io[0]; r = io[1];
    }
};

// ----------------------------------------------------------------------------
//  GLITCH v2: one knob, musically timed.
//  Tempo mode: glitches start on the host's 16th-note grid (favouring
//  downbeats) and last whole note values. Pick mode: glitches start on the
//  notes you pick. Every glitch is level-matched so it is never louder than
//  what you were playing. Low settings = rare and gentle stutters/chops;
//  wilder types (reverse, crush, tape stop, half-speed) unlock as you turn up.
// ----------------------------------------------------------------------------
struct Glitch
{
    enum Type { Stutter, Chop, Reverse, Crush, TapeStop, HalfSpeed };

    struct Clock
    {
        bool slot = false;      // a 16th-note boundary was crossed on this sample
        int  slotInBar = 0;     // 0..15 (or more in odd metres)
        bool onset = false;     // a picked note started on this sample (Pick mode)
        bool pickMode = false;
        double samplesPerBeat = 24000.0;
    };

    double sr = 48000;
    std::vector<float> buf[2];
    int size = 0, w = 0;
    bool active = false;
    Type type = Stutter;
    int len = 0, pos = 0, slice = 1, start = 0, chopLen = 1, cooldown = 0;
    float readPos = 0, speed = 1, crushBits = 4, crushHold = 8, holdCount = 0, held[2] {};
    float repeatGain = 1.0f;
    int fade = 96;
    // level matching
    float liveEnv = 0, liveA = 0, liveAtk = 0, ref = 0, gEnv = 0, gAtk = 0, gRel = 0, gain = 1, gainA = 0;
    Rng rng { 0x6117C4u };

    void prepare (double s)
    {
        sr = s;
        size = (int) (s * 4.0);
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        w = 0; active = false; cooldown = 0;
        fade = (int) (s * 0.004);
        liveA = std::exp (-1.0f / (float) (0.25 * s));
        liveAtk = std::exp (-1.0f / (float) (0.01 * s));
        gAtk = std::exp (-1.0f / (float) (0.003 * s));
        gRel = std::exp (-1.0f / (float) (0.12 * s));
        gainA = std::exp (-1.0f / (float) (0.008 * s));
        liveEnv = ref = gEnv = 0; gain = 1;
    }

    inline float rd (int c, float p) { return hermite (buf[c].data(), size, p); }
    inline float rdi (int c, int p)  { p %= size; if (p < 0) p += size; return buf[c][(size_t) p]; }

    template <size_t N> float pick (const float (&opts)[N], int count) { return opts[(size_t) std::min ((int) N - 1, (int) (rng.uni() * (float) count))]; }

    void trigger (float g, double spb)
    {
        std::pair<Type, float> pool[6]; int np = 0;
        pool[np++] = { Stutter, 1.0f };
        if (g > 0.10f) pool[np++] = { Chop,      0.6f };
        if (g > 0.30f) pool[np++] = { Reverse,   0.6f };
        if (g > 0.40f) pool[np++] = { Crush,     0.5f };
        if (g > 0.55f) pool[np++] = { TapeStop,  0.35f };
        if (g > 0.70f) pool[np++] = { HalfSpeed, 0.35f };
        float tot = 0; for (int i = 0; i < np; ++i) tot += pool[i].second;
        float r = rng.uni() * tot, acc = 0; type = Stutter;
        for (int i = 0; i < np; ++i) { acc += pool[i].second; if (r <= acc) { type = pool[i].first; break; } }

        auto beats = [&] (double b) { return std::max (64, (int) std::round (b * spb)); };
        pos = 0; repeatGain = 1.0f;
        switch (type)
        {
            case Stutter:
            {
                // slice in note values; first slice plays live, then repeats
                const float slices[] = { 0.5f, 0.25f, 0.125f, 1.0f / 6.0f };
                int n = g < 0.5f ? 2 : (g < 0.8f ? 3 : 4);
                float sl = pick (slices, n);
                const float totals[] = { 0.5f, 1.0f, 2.0f };
                float tl = pick (totals, g < 0.6f ? 2 : 3);
                if (tl < sl * 2.0f) tl = sl * 2.0f;
                slice = beats (sl); len = beats (tl); start = w;
                break;
            }
            case Chop:
            {
                const float chops[] = { 0.25f, 0.125f };
                chopLen = beats (pick (chops, 2));
                const float totals[] = { 0.5f, 1.0f };
                len = beats (pick (totals, 2));
                break;
            }
            case Reverse:
            {
                const float sl[] = { 0.5f, 1.0f };
                slice = beats (pick (sl, 2)); len = slice; start = w - slice;
                break;
            }
            case Crush:
            {
                const float t[] = { 0.5f, 1.0f };
                len = beats (pick (t, 2));
                crushBits = 3.0f + rng.uni() * 2.5f;
                crushHold = 4.0f + rng.uni() * 12.0f;
                holdCount = 0;
                break;
            }
            case TapeStop:
                len = beats (g > 0.85f && rng.uni() < 0.5f ? 2.0 : 1.0);
                readPos = (float) w - 1.0f; speed = 1.0f;
                break;
            case HalfSpeed:
            {
                const float t[] = { 0.5f, 1.0f };
                len = beats (pick (t, 2));
                readPos = (float) w - 1.0f;
                break;
            }
        }
        ref = std::max (liveEnv, 1.0e-4f);
        active = true;
    }

    inline void process (float& l, float& r, float g, const Clock& clk)
    {
        buf[0][(size_t) w] = l; buf[1][(size_t) w] = r;
        const float lv = std::max (std::abs (l), std::abs (r));
        if (! active) liveEnv = lv > liveEnv ? lv + liveAtk * (liveEnv - lv) : lv + liveA * (liveEnv - lv);

        if (! active && g > 0.005f)
        {
            if (cooldown > 0) --cooldown;
            else if (! clk.pickMode && clk.slot)
            {
                // events per bar: ~1 every 6 bars at 10%, ~1 per bar at 50%, ~2.5 per bar at 100%
                const float perBar = 0.12f + 2.4f * std::pow (g, 1.7f);
                const int s = clk.slotInBar;
                const float wgt = s == 0 ? 2.0f : (s % 4 == 0 ? 1.4f : (s % 2 == 0 ? 0.9f : 0.5f));
                if (rng.uni() < perBar / 16.0f / 0.8625f * wgt) trigger (g, clk.samplesPerBeat);
            }
            else if (clk.pickMode && clk.onset)
            {
                if (rng.uni() < 0.06f + 0.55f * std::pow (g, 1.4f)) trigger (g, clk.samplesPerBeat);
            }
        }

        if (active)
        {
            float gl = l, gr = r;
            bool matchLevel = true;
            switch (type)
            {
                case Stutter:
                {
                    if (pos >= slice)
                    {
                        int i = pos % slice;
                        if (i == 0) repeatGain *= 0.9f;
                        gl = rdi (0, start + i) * repeatGain; gr = rdi (1, start + i) * repeatGain;
                        float wv = 1.0f;
                        if (i < fade) wv = (float) i / (float) fade;
                        else if (i > slice - fade) wv = (float) (slice - i) / (float) fade;
                        gl *= wv; gr *= wv;
                    }
                    break;
                }
                case Chop:
                {
                    int i = pos % (chopLen * 2);
                    float gate;
                    if (i < chopLen) gate = 1.0f;
                    else
                    {
                        int k = i - chopLen;
                        gate = k < fade ? 1.0f - (float) k / (float) fade
                             : (k > chopLen - fade ? (float) (k - (chopLen - fade)) / (float) fade : 0.0f);
                        gate = 0.08f + 0.92f * gate;
                    }
                    gl = l * gate; gr = r * gate; matchLevel = false;
                    break;
                }
                case Reverse:
                {
                    int i = slice - 1 - pos;
                    gl = rdi (0, start + i); gr = rdi (1, start + i);
                    break;
                }
                case TapeStop:
                {
                    float t = (float) pos / (float) len;
                    speed = std::max (0.0f, 1.0f - t * 1.3f);
                    speed = speed * speed;
                    readPos += speed;
                    float amp = std::min (1.0f, speed * 5.0f);
                    gl = rd (0, readPos) * amp; gr = rd (1, readPos) * amp;
                    break;
                }
                case HalfSpeed:
                    readPos += 0.5f;
                    gl = rd (0, readPos); gr = rd (1, readPos);
                    break;
                case Crush:
                {
                    if ((holdCount -= 1.0f) <= 0) { holdCount += crushHold; held[0] = l; held[1] = r; }
                    float q = std::pow (2.0f, crushBits - 1.0f);
                    gl = std::round (held[0] * q) / q;
                    gr = std::round (held[1] * q) / q;
                    break;
                }
            }

            // never louder than what was playing when the glitch began
            const float ge = std::max (std::abs (gl), std::abs (gr));
            gEnv = ge > gEnv ? ge + gAtk * (gEnv - ge) : ge + gRel * (gEnv - ge);
            float target = matchLevel ? std::min (1.0f, 1.15f * ref / std::max (gEnv, 1.0e-5f)) : 1.0f;
            gain = target + gainA * (gain - target);
            gl *= gain; gr *= gain;

            float e = 1.0f;
            if (pos < fade) e = (float) pos / (float) fade;
            else if (pos > len - fade) e = (float) (len - pos) / (float) fade;
            l = l + (gl - l) * e;
            r = r + (gr - r) * e;

            if (++pos >= len)
            {
                active = false; gEnv = 0; gain = 1;
                cooldown = (int) (clk.samplesPerBeat * std::max (0.25, 1.0 - 0.75 * g));
            }
        }

        if (++w >= size) w = 0;
    }
};

// Detects picked notes (fast envelope jumping above the slow one)
struct OnsetDetector
{
    float fast = 0, slow = 0, fa = 0, fr = 0, sa = 0;
    int hold = 0, refractory = 0;
    void prepare (double sr)
    {
        fa = std::exp (-1.0f / (float) (0.001 * sr));
        fr = std::exp (-1.0f / (float) (0.030 * sr));
        sa = std::exp (-1.0f / (float) (0.120 * sr));
        refractory = (int) (0.09 * sr);
        fast = slow = 0; hold = 0;
    }
    inline bool process (float x)
    {
        x = std::abs (x);
        fast = x > fast ? x + fa * (fast - x) : x + fr * (fast - x);
        slow = fast + sa * (slow - fast);
        if (hold > 0) { --hold; return false; }
        if (fast > 0.003f && fast > slow * 1.9f + 0.001f) { hold = refractory; return true; }
        return false;
    }
};

} // namespace sg
