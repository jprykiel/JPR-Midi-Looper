cmake_minimum_required(VERSION 3.22)
project(JPRMidiLooper VERSION 0.1.0)
if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Version minimale de macOS")

# JUCE est téléchargé automatiquement par CMake (pas d'installation manuelle).
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(JPRMidiLooper
    COMPANY_NAME              "JPR"
    PLUGIN_MANUFACTURER_CODE  Jprx
    PLUGIN_CODE               Jmlp
    FORMATS                   AU
    PRODUCT_NAME              "JPR MIDI Looper"
    IS_MIDI_EFFECT            TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         TRUE
    IS_SYNTH                  FALSE
    AU_MAIN_TYPE              kAudioUnitType_MIDIProcessor
    COPY_PLUGIN_AFTER_BUILD   TRUE)

target_sources(JPRMidiLooper PRIVATE
    SharedState.cpp
    Announcer.mm
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(JPRMidiLooper PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(JPRMidiLooper
    PRIVATE
        juce::juce_audio_utils
        "-framework Cocoa"
        "-framework AVFoundation"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
