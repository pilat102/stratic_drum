#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static const Colour bgCol(0xff0b1016);
static const Colour cardCol(0xff101820);
static const Colour lineCol(0xff1e3a45);
static const Colour cyanCol(0xff35c8e8);
static const Colour textDim(0xff8fa3ad);

LPDrumMachineAudioProcessorEditor::LPDrumMachineAudioProcessorEditor(LPDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1150, 820);

    auto styleButton = [](TextButton& b)
        {
            b.setColour(TextButton::buttonColourId, Colour(0xff0e141b));
            b.setColour(TextButton::buttonOnColourId, cyanCol);
        };

    // Master knobs
    addAndMakeVisible(masterVolumeSlider);
    masterVolumeSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    masterVolumeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 18);
    masterVolumeAttach.reset(new AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "master_volume", masterVolumeSlider));
    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.setText("MASTER", dontSendNotification);
    masterVolumeLabel.setJustificationType(Justification::centred);
    masterVolumeLabel.setColour(Label::textColourId, cyanCol);

    addAndMakeVisible(driveSlider);
    driveSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 18);
    driveAttach.reset(new AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "drive", driveSlider));
    addAndMakeVisible(driveLabel);
    driveLabel.setText("DRIVE", dontSendNotification);
    driveLabel.setJustificationType(Justification::centred);
    driveLabel.setColour(Label::textColourId, cyanCol);

    // Voice pads (16)
    for (int i = 0; i < 16; ++i)
    {
        addAndMakeVisible(voiceButtons[i]);
        voiceButtons[i].setButtonText(LPDrumMachineAudioProcessor::voiceNames[i]);
        voiceButtons[i].setClickingTogglesState(true);
        voiceButtons[i].setRadioGroupId(42);
        styleButton(voiceButtons[i]);
        voiceButtons[i].onClick = [this, i] { selectVoice(i); };
    }
    voiceButtons[0].setToggleState(true, dontSendNotification);

    // Transport buttons
    addAndMakeVisible(learnButton);
    learnButton.setButtonText("Learn");
    styleButton(learnButton);
    learnButton.onClick = [this] { audioProcessor.startLearning(selectedVoice); };

    addAndMakeVisible(testButton);
    testButton.setButtonText("Test");
    styleButton(testButton);
    testButton.onClick = [this] { audioProcessor.triggerDrum(selectedVoice); };

    addAndMakeVisible(resetButton);
    resetButton.setButtonText("Reset");
    styleButton(resetButton);
    resetButton.onClick = [this] { audioProcessor.resetVoice(selectedVoice); };

    addAndMakeVisible(noteLabel);
    noteLabel.setJustificationType(Justification::centred);
    noteLabel.setColour(Label::textColourId, cyanCol);

    // Copy / Paste / Rand
    addAndMakeVisible(copyButton);
    copyButton.setButtonText("Copy");
    styleButton(copyButton);
    copyButton.onClick = [this]
        {
            const int n = LPDrumMachineAudioProcessor::paramCount();
            for (int p = 0; p < n && p < 32; ++p)
            {
                if (auto* param = audioProcessor.apvts.getParameter(
                    LPDrumMachineAudioProcessor::voiceParamId(selectedVoice, LPDrumMachineAudioProcessor::paramBaseAt(p))))
                    clipboard[p] = param->getValue();
            }
            hasClipboard = true;
        };

    addAndMakeVisible(pasteButton);
    pasteButton.setButtonText("Paste");
    styleButton(pasteButton);
    pasteButton.onClick = [this]
        {
            if (!hasClipboard) return;
            const int n = LPDrumMachineAudioProcessor::paramCount();
            for (int p = 0; p < n && p < 32; ++p)
            {
                if (auto* param = audioProcessor.apvts.getParameter(
                    LPDrumMachineAudioProcessor::voiceParamId(selectedVoice, LPDrumMachineAudioProcessor::paramBaseAt(p))))
                    param->setValueNotifyingHost(clipboard[p]);
            }
        };

    addAndMakeVisible(randButton);
    randButton.setButtonText("Rand");
    styleButton(randButton);
    randButton.onClick = [this]
        {
            Random r;
            const int n = LPDrumMachineAudioProcessor::paramCount();
            for (int p = 0; p < n && p < 32; ++p)
            {
                if (auto* param = audioProcessor.apvts.getParameter(
                    LPDrumMachineAudioProcessor::voiceParamId(selectedVoice, LPDrumMachineAudioProcessor::paramBaseAt(p))))
                    param->setValueNotifyingHost(r.nextFloat());
            }
        };

    // Comboboxes
    auto fillWaves = [](ComboBox& c)
        {
            c.addItem("Sine", 1);
            c.addItem("Tri", 2);
            c.addItem("Saw", 3);
            c.addItem("Square", 4);
        };
    fillWaves(osc1WaveCombo);
    fillWaves(osc2WaveCombo);
    fillWaves(lfoShapeCombo);
    filtTypeCombo.addItem("LP", 1);
    filtTypeCombo.addItem("HP", 2);
    filtTypeCombo.addItem("BP", 3);
    lfoTargetCombo.addItem("Off", 1);
    lfoTargetCombo.addItem("Cutoff", 2);
    lfoTargetCombo.addItem("Pitch", 3);
    lfoTargetCombo.addItem("Volume", 4);
    smpRevCombo.addItem("Fwd", 1);
    smpRevCombo.addItem("Rev", 2);
    smpLoopCombo.addItem("Off", 1);
    smpLoopCombo.addItem("Loop", 2);
    chokeCombo.addItem("Off", 1);
    chokeCombo.addItem("1", 2);
    chokeCombo.addItem("2", 3);
    chokeCombo.addItem("3", 4);
    chokeCombo.addItem("4", 5);

    for (auto* c : { &osc1WaveCombo, &osc2WaveCombo, &filtTypeCombo, &lfoShapeCombo, &lfoTargetCombo,
                     &smpRevCombo, &smpLoopCombo, &chokeCombo, &factoryCombo })
    {
        addAndMakeVisible(c);
        c->setColour(ComboBox::backgroundColourId, Colour(0xff0e141b));
        c->setColour(ComboBox::textColourId, cyanCol);
        c->setColour(ComboBox::outlineColourId, lineCol);
    }

    for (auto* t : { &osc1Title, &osc2Title, &noiseTitle, &filtTitle, &envTitle, &pitchTitle,
                     &volTitle, &lfoTitle, &fxTitle, &smpTitle, &outTitle, &masterFxTitle })
        addAndMakeVisible(t);

    // Knobs
    auto initSlider = [this](Slider& s, Label& l, const char* text)
        {
            s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(Slider::TextBoxBelow, false, 64, 16);
            s.setColour(Slider::rotarySliderFillColourId, cyanCol);
            s.setColour(Slider::rotarySliderOutlineColourId, lineCol);
            s.setColour(Slider::thumbColourId, cyanCol);
            s.setColour(Slider::textBoxTextColourId, cyanCol);
            s.setColour(Slider::textBoxBackgroundColourId, Colour(0xff0e141b));
            s.setColour(Slider::textBoxOutlineColourId, Colour(0x00000000));
            l.setText(text, dontSendNotification);
            l.setJustificationType(Justification::centred);
            l.setColour(Label::textColourId, textDim);
            addAndMakeVisible(s);
            addAndMakeVisible(l);
        };

    initSlider(osc1Pitch, osc1PitchLabel, "Pitch");
    initSlider(osc1Level, osc1LevelLabel, "Level");
    initSlider(osc2Pitch, osc2PitchLabel, "Pitch");
    initSlider(osc2Level, osc2LevelLabel, "Level");
    initSlider(noiseLevel, noiseLevelLabel, "Level");
    initSlider(clickKnob, clickLabel, "Click");
    initSlider(filtCutoff, filtCutoffLabel, "Cutoff");
    initSlider(filtRes, filtResLabel, "Res");
    initSlider(filtEnv, filtEnvLabel, "Env");
    initSlider(filtDecay, filtDecayLabel, "Decay");
    initSlider(ampAttack, ampAttackLabel, "Attack");
    initSlider(ampDecay, ampDecayLabel, "Decay");
    initSlider(pitchEnvAmt, pitchEnvLabel, "Amount");
    initSlider(pitchDecay, pitchDecayLabel, "Decay");
    initSlider(volSlider, volLabel, "Vol");
    initSlider(lfoRate, lfoRateLabel, "Rate");
    initSlider(lfoDepth, lfoDepthLabel, "Depth");
    initSlider(fxDrive, fxDriveLabel, "Drive");
    initSlider(fxBits, fxBitsLabel, "Bits");
    initSlider(fxDecim, fxDecimLabel, "Decim");
    initSlider(smpLevel, smpLevelLabel, "Level");
    initSlider(smpRate, smpRateLabel, "Rate");
    initSlider(smpOffset, smpOffsetLabel, "Offset");
    initSlider(panSlider, panLabel, "Pan");
    initSlider(chorMix, chorMixLabel, "Ch Mix");
    initSlider(chorRate, chorRateLabel, "Ch Rate");
    initSlider(chorDepth, chorDepthLabel, "Ch Depth");
    initSlider(dlyMix, dlyMixLabel, "Dl Mix");
    initSlider(dlyTime, dlyTimeLabel, "Dl Time");
    initSlider(dlyFdb, dlyFdbLabel, "Dl Fdb");
    initSlider(revMix, revMixLabel, "Rv Mix");
    initSlider(revSize, revSizeLabel, "Rv Size");
    initSlider(compAmt, compAmtLabel, "Comp");

    // Global master FX attachments
    auto ga = [this](Slider& s, const char* id)
        {
            globalAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id, s));
        };
    ga(chorMix, "chor_mix");
    ga(chorRate, "chor_rate");
    ga(chorDepth, "chor_depth");
    ga(dlyMix, "dly_mix");
    ga(dlyTime, "dly_time");
    ga(dlyFdb, "dly_fdb");
    ga(revMix, "rev_mix");
    ga(revSize, "rev_size");
    ga(compAmt, "comp_amt");

    // Sample buttons
    addAndMakeVisible(loadSampleButton);
    loadSampleButton.setButtonText("Load SMP");
    styleButton(loadSampleButton);
    loadSampleButton.onClick = [this]
        {
            sampleChooser = std::make_unique<FileChooser>("Load sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
            sampleChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                [this](const FileChooser& fc)
                {
                    const File result = fc.getResult();
                    if (result != File())
                    {
                        audioProcessor.loadSampleForVoice(selectedVoice, result);
                        updateSampleInfo();
                    }
                });
        };

    addAndMakeVisible(clearSampleButton);
    clearSampleButton.setButtonText("Clr");
    styleButton(clearSampleButton);
    clearSampleButton.onClick = [this]
        {
            audioProcessor.clearSample(selectedVoice);
            updateSampleInfo();
        };

    addAndMakeVisible(sampleInfoLabel);
    sampleInfoLabel.setText("(no sample)", dontSendNotification);
    sampleInfoLabel.setColour(Label::textColourId, textDim);

    // Preset row
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save");
    styleButton(saveButton);
    saveButton.onClick = [this]
        {
            chooser = std::make_unique<FileChooser>("Save StraticDrum kit", File(), "*.lpdk");
            chooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                [this](const FileChooser& fc)
                {
                    File result = fc.getResult();
                    if (result != File())
                    {
                        if (result.getFileExtension().isEmpty())
                            result = result.withFileExtension(".lpdk");
                        audioProcessor.saveKitToFile(result);
                    }
                });
        };

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load");
    styleButton(loadButton);
    loadButton.onClick = [this]
        {
            chooser = std::make_unique<FileChooser>("Load StraticDrum kit", File(), "*.lpdk");
            chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                [this](const FileChooser& fc)
                {
                    const File result = fc.getResult();
                    if (result != File())
                        audioProcessor.loadKitFromFile(result);
                });
        };

    for (int i = 0; i < LPDrumMachineAudioProcessor::factoryPresetCount; ++i)
        factoryCombo.addItem(LPDrumMachineAudioProcessor::factoryPresetNames[i], i + 1);
    factoryCombo.setSelectedItemIndex(0, dontSendNotification);
    factoryCombo.onChange = [this]
        {
            const int idx = factoryCombo.getSelectedItemIndex();
            if (idx >= 0)
                audioProcessor.applyFactoryPreset(idx);
        };

    rebuildAttachments();
    updateNoteLabels();
    updateSampleInfo();
    startTimer(200);
}

LPDrumMachineAudioProcessorEditor::~LPDrumMachineAudioProcessorEditor()
{
    stopTimer();
}

void LPDrumMachineAudioProcessorEditor::selectVoice(int index)
{
    selectedVoice = index;
    rebuildAttachments();
    updateNoteLabels();
    updateSampleInfo();
}

void LPDrumMachineAudioProcessorEditor::rebuildAttachments()
{
    sliderAttachments.clear();
    comboAttachments.clear();

    const int v = selectedVoice;

    auto sa = [this, v](Slider& s, const char* base)
        {
            sliderAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, base), s));
        };

    sa(osc1Pitch, "o1p");
    sa(osc1Level, "o1l");
    sa(osc2Pitch, "o2p");
    sa(osc2Level, "o2l");
    sa(noiseLevel, "nz");
    sa(clickKnob, "clk");
    sa(filtCutoff, "fc");
    sa(filtRes, "res");
    sa(filtEnv, "fenv");
    sa(filtDecay, "fdec");
    sa(ampAttack, "atk");
    sa(ampDecay, "dec");
    sa(pitchEnvAmt, "penv");
    sa(pitchDecay, "pdec");
    sa(volSlider, "vol");
    sa(lfoRate, "lfor");
    sa(lfoDepth, "lfod");
    sa(fxDrive, "fxd");
    sa(fxBits, "fxb");
    sa(fxDecim, "fxm");
    sa(smpLevel, "smpl");
    sa(smpRate, "smpr");
    sa(smpOffset, "smpoff");
    sa(panSlider, "pan");

    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "o1w"), osc1WaveCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "o2w"), osc2WaveCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "ft"), filtTypeCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "lfos"), lfoShapeCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "lfot"), lfoTargetCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "smprv"), smpRevCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "smploop"), smpLoopCombo));
    comboAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, LPDrumMachineAudioProcessor::voiceParamId(v, "chk"), chokeCombo));
}

void LPDrumMachineAudioProcessorEditor::updateNoteLabels()
{
    const int note = audioProcessor.midiNoteMap[selectedVoice];
    noteLabel.setText(MidiMessage::getMidiNoteName(note, true, true, 4), dontSendNotification);
    learnButton.setButtonText(audioProcessor.getLearnTarget() == selectedVoice ? "..." : "Learn");
}

void LPDrumMachineAudioProcessorEditor::updateSampleInfo()
{
    const String name = audioProcessor.getSampleName(selectedVoice);
    sampleInfoLabel.setText(name.isEmpty() ? String("(no sample)") : name, dontSendNotification);
}

void LPDrumMachineAudioProcessorEditor::timerCallback()
{
    updateNoteLabels();
    updateSampleInfo();
}

void LPDrumMachineAudioProcessorEditor::paint(Graphics& g)
{
    g.fillAll(bgCol);

    for (int i = 0; i < 10; ++i)
    {
        if (cardRects[i].isEmpty()) continue;
        g.setColour(cardCol);
        g.fillRoundedRectangle(cardRects[i].toFloat(), 6.0f);
        g.setColour(lineCol);
        g.drawRoundedRectangle(cardRects[i].toFloat(), 6.0f, 1.0f);
    }

    auto hr = getLocalBounds().removeFromTop(44);
    g.setColour(cyanCol);
    g.setFont(20.0f);
    g.drawText("STRATIC DRUM", hr.removeFromLeft(220), Justification::centredLeft);
    g.setColour(textDim);
    g.setFont(12.0f);
    g.drawText("HYBRID DRUM SYNTH  |  JUCE 9  |  VST3", hr, Justification::centredRight);
}

void LPDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header: presets + file buttons
    auto header = area.removeFromTop(44);
    header.removeFromLeft(230);
    factoryCombo.setBounds(header.removeFromLeft(200).reduced(0, 8));
    saveButton.setBounds(header.removeFromLeft(64).reduced(2, 8));
    loadButton.setBounds(header.removeFromLeft(64).reduced(2, 8));
    copyButton.setBounds(header.removeFromLeft(64).reduced(2, 8));
    pasteButton.setBounds(header.removeFromLeft(64).reduced(2, 8));
    randButton.setBounds(header.removeFromLeft(64).reduced(2, 8));

    // Master row
    auto master = area.removeFromTop(92);
    auto m1 = master.removeFromLeft(100);
    masterVolumeLabel.setBounds(m1.removeFromTop(16));
    masterVolumeSlider.setBounds(m1);
    auto m2 = master.removeFromLeft(100);
    driveLabel.setBounds(m2.removeFromTop(16));
    driveSlider.setBounds(m2);

    auto lr = master.removeFromRight(340).removeFromTop(30);
    learnButton.setBounds(lr.removeFromLeft(76));
    noteLabel.setBounds(lr.removeFromLeft(84));
    testButton.setBounds(lr.removeFromLeft(76));
    resetButton.setBounds(lr.removeFromLeft(76));

    // Voice pads: 2 rows x 8
    auto pads = area.removeFromTop(72);
    auto row1 = pads.removeFromTop(34);
    const int bw = row1.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
        voiceButtons[i].setBounds(row1.removeFromLeft(bw).reduced(2, 2));
    for (int i = 8; i < 16; ++i)
        voiceButtons[i].setBounds(pads.removeFromLeft(bw).reduced(2, 2));

    area.removeFromTop(6);

    // Cards grid 5 x 2
    const int cols = 5;
    const int cw = area.getWidth() / cols;
    const int ch = area.getHeight() / 2;

    Rectangle<int> R[10];
    for (int i = 0; i < 10; ++i)
        R[i] = Rectangle<int>(area.getX() + (i % cols) * cw,
            area.getY() + (i / cols) * ch, cw, ch);

    int ci = 0;
    auto startCard = [this, &ci, &R](Label& title, const char* text) -> Rectangle<int>
        {
            auto r = R[ci];
            cardRects[ci] = r;
            ++ci;
            title.setText(text, dontSendNotification);
            title.setColour(Label::textColourId, cyanCol);
            title.setJustificationType(Justification::centredLeft);
            title.setBounds(r.removeFromTop(22).reduced(8, 2));
            return r.reduced(8, 2);
        };

    auto knobRow = [this](Rectangle<int>& r, int n,
        std::initializer_list<std::pair<Slider*, Label*>> items)
        {
            auto row = r.removeFromTop(80);
            const int w = row.getWidth() / n;
            for (auto& pr : items)
            {
                auto k = row.removeFromLeft(w).reduced(3, 0);
                pr.second->setBounds(k.removeFromTop(16));
                pr.first->setBounds(k);
            }
        };

    auto comboRow1 = [](Rectangle<int>& r, ComboBox& c)
        {
            c.setBounds(r.removeFromTop(24).reduced(2, 1));
        };

    auto comboRow2 = [](Rectangle<int>& r, ComboBox& c1, ComboBox& c2)
        {
            auto row = r.removeFromTop(24);
            const int w = row.getWidth() / 2;
            c1.setBounds(row.removeFromLeft(w).reduced(2, 1));
            c2.setBounds(row.reduced(2, 1));
        };

    auto r0 = startCard(osc1Title, "OSC 1");
    comboRow1(r0, osc1WaveCombo);
    knobRow(r0, 2, { { &osc1Pitch, &osc1PitchLabel }, { &osc1Level, &osc1LevelLabel } });

    auto r1 = startCard(osc2Title, "OSC 2");
    comboRow1(r1, osc2WaveCombo);
    knobRow(r1, 2, { { &osc2Pitch, &osc2PitchLabel }, { &osc2Level, &osc2LevelLabel } });

    auto r2 = startCard(filtTitle, "FILTER");
    comboRow1(r2, filtTypeCombo);
    knobRow(r2, 2, { { &filtCutoff, &filtCutoffLabel }, { &filtRes, &filtResLabel } });
    knobRow(r2, 2, { { &filtEnv, &filtEnvLabel }, { &filtDecay, &filtDecayLabel } });

    auto r3 = startCard(envTitle, "ENVELOPES");
    knobRow(r3, 2, { { &ampAttack, &ampAttackLabel }, { &ampDecay, &ampDecayLabel } });
    knobRow(r3, 2, { { &pitchEnvAmt, &pitchEnvLabel }, { &pitchDecay, &pitchDecayLabel } });

    auto r4 = startCard(lfoTitle, "LFO");
    comboRow2(r4, lfoShapeCombo, lfoTargetCombo);
    knobRow(r4, 2, { { &lfoRate, &lfoRateLabel }, { &lfoDepth, &lfoDepthLabel } });

    auto r5 = startCard(smpTitle, "SAMPLE");
    {
        auto row = r5.removeFromTop(24);
        loadSampleButton.setBounds(row.removeFromLeft(90).reduced(2, 1));
        clearSampleButton.setBounds(row.removeFromLeft(46).reduced(2, 1));
        sampleInfoLabel.setBounds(row.reduced(2, 1));
    }
    knobRow(r5, 3, { { &smpLevel, &smpLevelLabel }, { &smpRate, &smpRateLabel }, { &smpOffset, &smpOffsetLabel } });
    comboRow2(r5, smpRevCombo, smpLoopCombo);

    auto r6 = startCard(fxTitle, "FX");
    knobRow(r6, 3, { { &fxDrive, &fxDriveLabel }, { &fxBits, &fxBitsLabel }, { &fxDecim, &fxDecimLabel } });

    auto r7 = startCard(outTitle, "OUT / CHOKE");
    knobRow(r7, 2, { { &volSlider, &volLabel }, { &panSlider, &panLabel } });
    comboRow1(r7, chokeCombo);

    auto r8 = startCard(noiseTitle, "NOISE");
    knobRow(r8, 2, { { &noiseLevel, &noiseLevelLabel }, { &clickKnob, &clickLabel } });

    auto r9 = startCard(masterFxTitle, "MASTER FX");
    knobRow(r9, 3, { { &chorMix, &chorMixLabel }, { &chorRate, &chorRateLabel }, { &chorDepth, &chorDepthLabel } });
    knobRow(r9, 3, { { &dlyMix, &dlyMixLabel }, { &dlyTime, &dlyTimeLabel }, { &dlyFdb, &dlyFdbLabel } });
    knobRow(r9, 3, { { &revMix, &revMixLabel }, { &revSize, &revSizeLabel }, { &compAmt, &compAmtLabel } });
}