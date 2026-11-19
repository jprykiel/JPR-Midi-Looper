#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class JPRLooperEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit JPRLooperEditor (JPRLooperProcessor&);
    ~JPRLooperEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupLabel (juce::Label&, const juce::String& text, juce::Component& target);
    void setupHeading (juce::Label&, const juce::String& text);
    void runPendingAction();

    JPRLooperProcessor& proc;

    // Sections (titres lisibles par VoiceOver)
    juce::Label headTrack, headGlobal, headActions, headStatus;

    // Piste
    juce::Label        trackLabel, inputLabel, recModeLabel, drumLabel;
    juce::ComboBox     trackBox, inputBox, recModeBox, drumBox;
    juce::ToggleButton muteButton  { "Muet" };
    juce::ToggleButton soloButton  { "Solo" };
    juce::ToggleButton armedButton;

    // Global
    juce::Label        lengthLabel, spbLabel, swingLabel, gateLabel, midiOutLabel, cmdBaseLabel, cmdTrigTypeLabel, cmdTrigNumberLabel, logicModeLabel;
    juce::Label        grooveAmountLabel, grooveLengthLabel, grooveStepLabel, grooveValueLabel;
    juce::Slider       lengthSlider, swingSlider, gateSlider, cmdBaseSlider, cmdTrigNumberSlider;
    juce::Slider       grooveAmountSlider, grooveLengthSlider, grooveStepSlider, grooveValueSlider;
    juce::ComboBox     spbBox, midiOutBox, cmdTrigTypeBox, logicModeBox;

    // Transposition (globale)
    juce::Label        transposeLabel, transBaseLabel, transTrigTypeLabel, transTrigNumberLabel;
    juce::Slider       transposeSlider, transBaseSlider, transTrigNumberSlider;
    juce::ComboBox     transTrigTypeBox;
    juce::ToggleButton runningButton { "Lecture" };
    juce::ToggleButton inputMuteButton { "Clavier muet" };

    // Actions (exécutées après un délai, pour parcourir la liste sans déclencher)
    juce::Label    actionLabel;
    juce::ComboBox actionBox;
    int            pendingAction  = 0;
    unsigned       pendingSinceMs = 0;

    // État
    juce::Label statusLabel;

    using Apvts = juce::AudioProcessorValueTreeState;
    std::unique_ptr<Apvts::ComboBoxAttachment> trackAtt, inputAtt, recModeAtt, spbAtt, midiOutAtt, cmdTrigTypeAtt, logicModeAtt;
    std::unique_ptr<Apvts::SliderAttachment>   lengthAtt, swingAtt, gateAtt, cmdBaseAtt, cmdTrigNumberAtt;
    std::unique_ptr<Apvts::SliderAttachment>   grooveAmountAtt, grooveLengthAtt, grooveStepAtt, grooveValueAtt;
    std::unique_ptr<Apvts::ButtonAttachment>   muteAtt, soloAtt, armedAtt, runningAtt, inputMuteAtt;
    std::unique_ptr<Apvts::ComboBoxAttachment> drumAtt, transTrigTypeAtt;
    std::unique_ptr<Apvts::SliderAttachment>   transposeAtt, transBaseAtt, transTrigNumberAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JPRLooperEditor)
};
