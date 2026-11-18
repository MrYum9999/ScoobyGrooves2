#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct Preset
{
    const char* name;
    // lofiOn bits rate tone crackle lofiLvl | tapeOn wow flutter hiss age tapeLvl | tubeOn drive bias warmth sag tubeLvl | route glitch mix out
    float v[22];
};

const char* ids[22] = {
    "lofiOn", "lofiBits", "lofiRate", "lofiTone", "lofiCrackle", "lofiLevel",
    "tapeOn", "tapeWow", "tapeFlutter", "tapeHiss", "tapeAge", "tapeLevel",
    "tubeOn", "tubeDrive", "tubeBias", "tubeWarmth", "tubeSag", "tubeLevel",
    "routing", "glitch", "mix", "output"
};

const Preset presets[] = {
    { "Lunchbox Classic",  { 1, 10, 16000, 7000, 0.15f, 0.5f,   1, 0.30f, 0.20f, 0.15f, 0.35f, 1.0f,   1, 0.35f, 0.30f, 0.40f, 0.30f, 1.0f,   0, 0.00f, 1.00f,  0 } },
    { "Dusty Groove",      { 1,  9, 12000, 5200, 0.45f, 0.7f,   1, 0.20f, 0.15f, 0.25f, 0.50f, 1.0f,   1, 0.25f, 0.20f, 0.55f, 0.40f, 0.8f,   0, 0.00f, 1.00f,  0 } },
    { "Sunday Cassette",   { 0, 12, 22000, 9000, 0.00f, 0.5f,   1, 0.45f, 0.30f, 0.30f, 0.45f, 1.0f,   1, 0.15f, 0.20f, 0.35f, 0.20f, 0.6f,   0, 0.00f, 1.00f,  0 } },
    { "Warm Tube Strum",   { 0, 12, 22000, 9000, 0.00f, 0.5f,   1, 0.10f, 0.10f, 0.05f, 0.20f, 0.5f,   1, 0.55f, 0.45f, 0.45f, 0.45f, 1.0f,   0, 0.00f, 1.00f,  0 } },
    { "Crushed Funk",      { 1,  6,  8000, 4500, 0.10f, 0.9f,   0, 0.10f, 0.10f, 0.00f, 0.20f, 0.5f,   1, 0.50f, 0.30f, 0.25f, 0.60f, 1.0f,   0, 0.10f, 1.00f,  2 } },
    { "Wobbly Memory",     { 1, 11, 14000, 6000, 0.30f, 0.4f,   1, 0.85f, 0.45f, 0.30f, 0.60f, 1.0f,   1, 0.20f, 0.20f, 0.60f, 0.20f, 0.7f,   0, 0.05f, 1.00f,  0 } },
    { "Broken Walkman",    { 1,  8,  9000, 4000, 0.35f, 0.6f,   1, 0.70f, 0.70f, 0.50f, 0.85f, 1.0f,   1, 0.40f, 0.50f, 0.50f, 0.40f, 0.8f,   0, 0.35f, 1.00f,  0 } },
    { "Bedroom Soul",      { 1, 12, 18000, 6500, 0.20f, 0.35f,  1, 0.25f, 0.15f, 0.10f, 0.40f, 0.8f,   1, 0.30f, 0.35f, 0.55f, 0.35f, 0.8f,   1, 0.00f, 0.75f,  0 } },
    { "Glitch Picnic",     { 1,  7, 11000, 6000, 0.25f, 0.6f,   1, 0.35f, 0.30f, 0.20f, 0.40f, 1.0f,   1, 0.40f, 0.35f, 0.40f, 0.35f, 0.8f,   1, 0.70f, 1.00f,  0 } },
    { "Clean Lunch",       { 0, 16, 44100,16000, 0.00f, 0.0f,   0, 0.00f, 0.00f, 0.00f, 0.00f, 0.0f,   0, 0.00f, 0.00f, 0.00f, 0.00f, 0.0f,   0, 0.00f, 1.00f,  0 } },
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

juce::AudioProcessorValueTreeState::ParameterLayout ScoobyGroovesProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + "%"; });
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def, AudioParameterFloatAttributes a, float skew = 1.0f)
    {
        NormalisableRange<float> r (lo, hi); r.setSkewForCentre (skew == 1.0f ? (lo + hi) * 0.5f : skew);
        ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, a));
    };
    auto b = [&] (const char* id, const char* name, bool def) { ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    auto hz = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v >= 1000 ? String (v / 1000.0f, 1) + "k" : String (roundToInt (v)); });
    auto bitsA = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " bit"; });
    auto dbA = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; });

    b ("lofiOn", "Lo-Fi On", true);
    f ("lofiBits", "Lo-Fi Bits", 3.0f, 16.0f, 10.0f, bitsA);
    f ("lofiRate", "Lo-Fi Rate", 2000.0f, 44100.0f, 16000.0f, hz, 12000.0f);
    f ("lofiTone", "Lo-Fi Tone", 800.0f, 16000.0f, 7000.0f, hz, 4500.0f);
    f ("lofiCrackle", "Lo-Fi Dust", 0.0f, 1.0f, 0.1f, pct);
    f ("lofiLevel", "Lo-Fi Blend", 0.0f, 1.0f, 0.5f, pct);

    b ("tapeOn", "Cassette On", true);
    f ("tapeWow", "Wow", 0.0f, 1.0f, 0.3f, pct);
    f ("tapeFlutter", "Flutter", 0.0f, 1.0f, 0.2f, pct);
    f ("tapeHiss", "Hiss", 0.0f, 1.0f, 0.15f, pct);
    f ("tapeAge", "Tape Age", 0.0f, 1.0f, 0.35f, pct);
    f ("tapeLevel", "Cassette Blend", 0.0f, 1.0f, 1.0f, pct);

    b ("tubeOn", "Tube On", true);
    f ("tubeDrive", "Tube Drive", 0.0f, 1.0f, 0.35f, pct);
    f ("tubeBias", "Tube Bias", 0.0f, 1.0f, 0.3f, pct);
    f ("tubeWarmth", "Tube Warmth", 0.0f, 1.0f, 0.4f, pct);
    f ("tubeSag", "Tube Sag", 0.0f, 1.0f, 0.3f, pct);
    f ("tubeLevel", "Tube Blend", 0.0f, 1.0f, 1.0f, pct);

    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "routing", 1 }, "Routing", StringArray { "Series", "Parallel" }, 0));
    f ("glitch", "Glitch", 0.0f, 1.0f, 0.0f, pct);
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "glitchSync", 1 }, "Glitch Sync", StringArray { "Tempo", "Pick" }, 0));
    f ("mix", "Mix", 0.0f, 1.0f, 1.0f, pct);
    f ("output", "Output", -24.0f, 12.0f, 0.0f, dbA);

    return { ps.begin(), ps.end() };
}

ScoobyGroovesProcessor::ScoobyGroovesProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool ScoobyGroovesProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    auto in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void ScoobyGroovesProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    lofi.prepare (sr); tape.prepare (sr); tube.prepare (sr); glitch.prepare (sr);
    lofiP.prepare (sr); tubeP.prepare (sr); onsets.prepare (sr);
    freeBeats = 0.0; lastSlot = -1; pendingOnset = -1;

    drySize = tape.latency() + 4;
    for (auto& d : dryBuf) d.assign ((size_t) drySize, 0.0f);
    dryW = 0;
    setLatencySamples (tape.latency());

    auto init = [&] (juce::SmoothedValue<float>& s, float v, double t = 0.03) { s.reset (sr, t); s.setCurrentAndTargetValue (v); };
    init (sLofiOn, *p ("lofiOn")); init (sTapeOn, *p ("tapeOn")); init (sTubeOn, *p ("tubeOn"));
    init (sLofiLvl, *p ("lofiLevel")); init (sTapeLvl, *p ("tapeLevel")); init (sTubeLvl, *p ("tubeLevel"));
    init (sMix, *p ("mix")); init (sOut, juce::Decibels::decibelsToGain (p ("output")->load()));
    init (sRoute, *p ("routing"), 0.05);
    init (sBits, *p ("lofiBits")); init (sRate, *p ("lofiRate"));
    init (sWow, *p ("tapeWow"), 0.1); init (sFlut, *p ("tapeFlutter"), 0.1); init (sHiss, *p ("tapeHiss"));
    init (sDrive, *p ("tubeDrive")); init (sBias, *p ("tubeBias")); init (sSag, *p ("tubeSag"));
    init (sCrackle, *p ("lofiCrackle"));
}

void ScoobyGroovesProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int c = numIn; c < numOut; ++c) buffer.clear (c, 0, n);

    sLofiOn.setTargetValue (*p ("lofiOn")); sTapeOn.setTargetValue (*p ("tapeOn")); sTubeOn.setTargetValue (*p ("tubeOn"));
    sLofiLvl.setTargetValue (*p ("lofiLevel")); sTapeLvl.setTargetValue (*p ("tapeLevel")); sTubeLvl.setTargetValue (*p ("tubeLevel"));
    sMix.setTargetValue (*p ("mix")); sOut.setTargetValue (juce::Decibels::decibelsToGain (p ("output")->load()));
    sRoute.setTargetValue (*p ("routing"));
    sBits.setTargetValue (*p ("lofiBits")); sRate.setTargetValue (*p ("lofiRate"));
    sWow.setTargetValue (*p ("tapeWow")); sFlut.setTargetValue (*p ("tapeFlutter")); sHiss.setTargetValue (*p ("tapeHiss"));
    sDrive.setTargetValue (*p ("tubeDrive")); sBias.setTargetValue (*p ("tubeBias")); sSag.setTargetValue (*p ("tubeSag"));
    sCrackle.setTargetValue (*p ("lofiCrackle"));

    lofi.setTone (*p ("lofiTone")); lofiP.setTone (*p ("lofiTone"));
    lofi.setRate (*p ("lofiRate")); lofiP.setRate (*p ("lofiRate"));
    tape.setAge (*p ("tapeAge"));
    tube.setWarmth (*p ("tubeWarmth")); tubeP.setWarmth (*p ("tubeWarmth"));
    const float age = *p ("tapeAge");
    const float g = *p ("glitch");
    const bool pickMode = p ("glitchSync")->load() > 0.5f;

    // ---- musical clock from the DAW (falls back to 120 BPM free-running)
    double bpm = 120.0, ppq = 0.0, barBeats = 4.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto ts = pos->getTimeSignature()) if (ts->numerator > 0 && ts->denominator > 0) barBeats = ts->numerator * 4.0 / ts->denominator;
            if (auto pq = pos->getPpqPosition()) { ppq = *pq; playing = pos->getIsPlaying(); }
        }
    bpm = juce::jlimit (30.0, 300.0, bpm);
    const double beatsPerSample = bpm / 60.0 / sr;
    // the plugin reports latency, so the host shifts our output earlier; line the grid up with that
    double beatPos = playing ? ppq - tape.latency() * beatsPerSample : freeBeats;
    sg::Glitch::Clock clk;
    clk.pickMode = pickMode;
    clk.samplesPerBeat = sr * 60.0 / bpm;

    float* L = buffer.getWritePointer (0);
    float* R = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0);

    bool anyGlitch = false;
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i];
        const float xr = inR[i];

        const float lofiW = sLofiOn.getNextValue() * sLofiLvl.getNextValue();
        const float tapeW = sTapeOn.getNextValue() * sTapeLvl.getNextValue();
        const float tubeW = sTubeOn.getNextValue() * sTubeLvl.getNextValue();
        const float route = sRoute.getNextValue();          // 0 = series, 1 = parallel
        const float bits = sBits.getNextValue(), rate = sRate.getNextValue(), crackle = sCrackle.getNextValue();
        const float wow = sWow.getNextValue(), flut = sFlut.getNextValue(), hiss = sHiss.getNextValue();
        const float drive = sDrive.getNextValue(), bias = sBias.getNextValue(), sag = sSag.getNextValue();
        const float mix = sMix.getNextValue(), outG = sOut.getNextValue();

        // grid + picking clocks for this sample
        const long long slotIdx = (long long) std::floor (beatPos * 4.0);
        clk.slot = slotIdx != lastSlot;
        lastSlot = slotIdx;
        double inBar = std::fmod (beatPos, barBeats); if (inBar < 0) inBar += barBeats;
        clk.slotInBar = (int) std::floor (inBar * 4.0);
        const double bars = playing ? beatPos / barBeats : -1.0;
        beatPos += beatsPerSample;

        clk.onset = false;
        if (onsets.process (0.5f * (xl + xr))) pendingOnset = tape.latency();
        if (pendingOnset >= 0 && pendingOnset-- == 0) clk.onset = true;

        // dry signal delayed to match the cassette latency
        dryBuf[0][(size_t) dryW] = xl; dryBuf[1][(size_t) dryW] = xr;
        int dp = dryW - tape.latency(); if (dp < 0) dp += drySize;
        const float dl = dryBuf[0][(size_t) dp], dr = dryBuf[1][(size_t) dp];
        if (++dryW >= drySize) dryW = 0;

        // ---- SERIES: Tube -> Cassette -> Lo-Fi, each blended by its own level
        float sl = xl, srr = xr;
        if (route < 0.999f)
        {
            float tl = sl, tr = srr;
            tube.process (tl, tr, drive, bias, sag);
            sl += (tl - sl) * tubeW; srr += (tr - srr) * tubeW;

            float wl, wr, ddl, ddr;
            tape.process (sl, srr, wow, flut, hiss, age, wl, wr, ddl, ddr, bars);
            sl = ddl + (wl - ddl) * tapeW; srr = ddr + (wr - ddr) * tapeW;

            float ll = sl, lr = srr;
            lofi.process (ll, lr, rate, bits, crackle);
            sl += (ll - sl) * lofiW; srr += (lr - srr) * lofiW;
        }

        // ---- PARALLEL: each section fed the same input, blended side by side
        float pl = dl, pr = dr;
        if (route > 0.001f)
        {
            float wl, wr, ddl, ddr;
            if (route >= 0.999f) tape.process (xl, xr, wow, flut, hiss, age, wl, wr, ddl, ddr, bars);
            else { wl = sl; wr = srr; }   // during a switch the tape already ran in series
            float tl = dl, tr = dr; tubeP.process (tl, tr, drive, bias, sag);
            float ll = dl, lr = dr; lofiP.process (ll, lr, rate, bits, crackle);

            const float sum = tubeW + tapeW + lofiW;
            const float norm = sum > 1.0f ? 1.0f / sum : 1.0f;
            const float dryPart = std::max (0.0f, 1.0f - sum);
            pl = (tl * tubeW + wl * tapeW + ll * lofiW) * norm + dl * dryPart;
            pr = (tr * tubeW + wr * tapeW + lr * lofiW) * norm + dr * dryPart;
        }

        float wetL = sl + (pl - sl) * route;
        float wetR = srr + (pr - srr) * route;

        glitch.process (wetL, wetR, g, clk);
        anyGlitch |= glitch.active;

        float outL = (dl + (wetL - dl) * mix) * outG;
        float outR = (dr + (wetR - dr) * mix) * outG;
        // gentle safety clip so nothing ever explodes
        outL = std::tanh (outL * 0.8f) * 1.25f;
        outR = std::tanh (outR * 0.8f) * 1.25f;

        L[i] = outL;
        if (R != nullptr) R[i] = outR;
        else L[i] = 0.5f * (outL + outR);
    }

    freeBeats = beatPos;
    glitchLamp.store (anyGlitch ? 1.0f : 0.0f);
}

int ScoobyGroovesProcessor::getNumPrograms() { return numPresets; }
const juce::String ScoobyGroovesProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : ""; }

void ScoobyGroovesProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numPresets)) return;
    currentProgram = index;
    for (int k = 0; k < 22; ++k)
        if (auto* prm = apvts.getParameter (ids[k]))
            prm->setValueNotifyingHost (prm->convertTo0to1 (presets[index].v[k]));
}

void ScoobyGroovesProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void ScoobyGroovesProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = vt.getProperty ("program", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* ScoobyGroovesProcessor::createEditor() { return new ScoobyGroovesEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ScoobyGroovesProcessor(); }
