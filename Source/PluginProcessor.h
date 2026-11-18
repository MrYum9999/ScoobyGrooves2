#pragma once
#include <JuceHeader.h>
#include "DSP.h"

class ScoobyGroovesProcessor : public juce::AudioProcessor
{
public:
    ScoobyGroovesProcessor();
    ~ScoobyGroovesProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ScoobyGrooves2"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // For the editor's glitch lamp
    std::atomic<float> glitchLamp { 0.0f };

private:
    sg::LoFi lofi;
    sg::Cassette tape;
    sg::Tube tube;
    sg::LoFi lofiP;   // separate state for the parallel path
    sg::Tube tubeP;
    sg::Glitch glitch;
    sg::OnsetDetector onsets;
    double freeBeats = 0.0;
    long long lastSlot = -1;
    int pendingOnset = -1;

    // the dry signal is delayed to line up with the cassette's fixed latency
    std::vector<float> dryBuf[2];
    int dryW = 0, drySize = 0;

    juce::SmoothedValue<float> sLofiOn, sTapeOn, sTubeOn, sLofiLvl, sTapeLvl, sTubeLvl,
                               sMix, sOut, sRoute, sBits, sRate, sWow, sFlut, sHiss, sDrive, sBias, sSag, sCrackle;
    int currentProgram = 0;
    double sr = 48000.0;

    std::atomic<float>* p (const char* id) { return apvts.getRawParameterValue (id); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScoobyGroovesProcessor)
};
