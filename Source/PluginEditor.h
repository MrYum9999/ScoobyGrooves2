#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace pal
{
// 1970s lunchbox palette
const juce::Colour cream     { 0xfff4e4bf };
const juce::Colour paper     { 0xfffbf0d4 };
const juce::Colour brown     { 0xff4a2c17 };
const juce::Colour darkBrown { 0xff2a170c };
const juce::Colour orange    { 0xffe0601f };
const juce::Colour gold      { 0xffeaa91a };
const juce::Colour mustard   { 0xffd18d14 };
const juce::Colour avocado   { 0xff6f7d2a };
const juce::Colour olive     { 0xff4f5a1d };
const juce::Colour teal      { 0xff23847e };
const juce::Colour red       { 0xffc23a24 };
const juce::Colour pink      { 0xffe58a7a };
}

class LunchboxLook : public juce::LookAndFeel_V4
{
public:
    LunchboxLook();
    static juce::Font title (float h);
    static juce::Font label (float h);

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return label (17.0f); }
    juce::Font getPopupMenuFont() override { return label (17.0f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class ScoobyGroovesEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ScoobyGroovesEditor (ScoobyGroovesProcessor&);
    ~ScoobyGroovesEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ScoobyGroovesProcessor& proc;
    LunchboxLook look;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider s; std::unique_ptr<SA> a; };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<BA>> buttonAtts;
    juce::ToggleButton lofiOn, tapeOn, tubeOn, routing, glitchSync;
    juce::ComboBox presetBox;

    juce::Rectangle<int> cardLofi, cardTape, cardTube, deck, glitchLampArea;
    float lamp = 0.0f;

    Knob& addKnob (const char* id, const juce::String& name, juce::Colour accent, bool big = false);
    void drawCard (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, juce::Colour, int icon);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScoobyGroovesEditor)
};
