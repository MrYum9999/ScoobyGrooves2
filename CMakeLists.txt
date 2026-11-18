cmake_minimum_required(VERSION 3.22)

project(ScoobyGrooves2 VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pull JUCE straight from GitHub so there is nothing to install by hand.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ScoobyGrooves2
    COMPANY_NAME              "Brad"
    PLUGIN_MANUFACTURER_CODE  Brad
    PLUGIN_CODE               Scg2
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "ScoobyGrooves2"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Fx Distortion
)

# The two open-licence fonts (SIL OFL) used by the lunchbox interface
juce_add_binary_data(ScoobyGrooves2Fonts SOURCES
    Fonts/Shrikhand-Regular.ttf
    Fonts/Righteous-Regular.ttf
)

juce_generate_juce_header(ScoobyGrooves2)

target_sources(ScoobyGrooves2 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(ScoobyGrooves2 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ScoobyGrooves2
    PRIVATE
        ScoobyGrooves2Fonts
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
