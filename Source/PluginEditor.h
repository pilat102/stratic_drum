#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;

class LPDrumMachineAudioProcessorEditor : public AudioProcessorEditor,
    private Timer
{
public:
    explicit LPDrumMachineAudioProcessorEditor(LPDrumMachineAudioProcessor&);
    ~LPDrumMachineAudioProcessorEditor() override;

    void paint(Graphics&) override;
    void resized() override;

private:
    void selectVoice(int index);
    void rebuildAttachments();
    void updateNoteLabels();
    void updateSampleInfo();
    void timerCallback() override;

    LPDrumMachineAudioProcessor& audioProcessor;
    int selectedVoice = 0;

    Rectangle<int> cardRects[10];

    // Master
    Slider masterVolumeSlider, driveSlider;
    Label masterVolumeLabel, driveLabel;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttach, driveAttach;

    // Voice selector (16 pads) + learn/test/reset
    TextButton voiceButtons[16];
    TextButton learnButton, testButton, resetButton;
    Label noteLabel;

    // Copy / paste / random
    TextButton copyButton, pasteButton, randButton;
    float clipboard[32];
    bool hasClipboard = false;

    // Synth panel of selected voice
    ComboBox osc1WaveCombo, osc2WaveCombo, filtTypeCombo;
    ComboBox lfoShapeCombo, lfoTargetCombo;
    ComboBox smpRevCombo, smpLoopCombo, chokeCombo;
    Label osc1Title, osc2Title, noiseTitle, filtTitle, envTitle, pitchTitle, volTitle, lfoTitle, fxTitle, smpTitle, outTitle, masterFxTitle;

    Slider osc1Pitch, osc1Level, osc2Pitch, osc2Level, noiseLevel, clickKnob;
    Slider filtCutoff, filtRes, filtEnv, filtDecay;
    Slider ampAttack, ampDecay, pitchEnvAmt, pitchDecay, volSlider;
    Slider lfoRate, lfoDepth, fxDrive, fxBits, fxDecim;
    Slider smpLevel, smpRate, smpOffset;
    Slider panSlider;

    Label osc1PitchLabel, osc1LevelLabel, osc2PitchLabel, osc2LevelLabel, noiseLevelLabel, clickLabel;
    Label filtCutoffLabel, filtResLabel, filtEnvLabel, filtDecayLabel;
    Label ampAttackLabel, ampDecayLabel, pitchEnvLabel, pitchDecayLabel, volLabel;
    Label lfoRateLabel, lfoDepthLabel, fxDriveLabel, fxBitsLabel, fxDecimLabel;
    Label smpLevelLabel, smpRateLabel, smpOffsetLabel, sampleInfoLabel;
    Label smpRevLabel, smpLoopLabel, panLabel, chokeLabel;

    // Master FX (global)
    Slider chorMix, chorRate, chorDepth, dlyMix, dlyTime, dlyFdb, revMix, revSize, compAmt;
    Label chorMixLabel, chorRateLabel, chorDepthLabel, dlyMixLabel, dlyTimeLabel, dlyFdbLabel, revMixLabel, revSizeLabel, compAmtLabel;

    TextButton loadSampleButton, clearSampleButton;
    std::unique_ptr<FileChooser> sampleChooser;

    // Presets
    ComboBox factoryCombo;
    TextButton saveButton, loadButton;
    std::unique_ptr<FileChooser> chooser;

    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttachments;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> globalAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LPDrumMachineAudioProcessorEditor)
};