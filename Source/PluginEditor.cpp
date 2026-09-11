#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static const Colour bgCol(0xff17181d);
static const Colour cardCol(0xff24262c);
static const Colour cardLineCol(0xff3a3d45);
static const Colour knobGrey(0xff9aa0a6);

static const Colour cOsc(0xffff8a3d);
static const Colour cOsc2(0xffffc94d);
static const Colour cFilt(0xff4dd0e1);
static const Colour cEnv(0xff64b5f6);
static const Colour cLfo(0xffe573ff);
static const Colour cSmp(0xff81c784);
static const Colour cFx(0xffff5252);
static const Colour cOut(0xffaed581);
static const Colour cNz(0xffb0bec5);

LPDrumMachineAudioProcessorEditor::LPDrumMachineAudioProcessorEditor(LPDrumMachineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1150, 780);

    auto styleButton = [](TextButton& b, Colour bg)
        {
            b.setColour(TextButton::buttonColourId, bg);
            b.setColour(TextButton::buttonOnColourId, bg.brighter(0.2f));
        };

    // Master knobs
    addAndMakeVisible(masterVolumeSlider);
    masterVolumeSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    masterVolumeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 18);
    masterVolumeSlider.setColour(Slider::rotarySliderFillColourId, cOut);
    masterVolumeSlider.setColour(Slider::thumbColourId, cOut);
    masterVolumeAttach.reset(new AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "master_volume", masterVolumeSlider));
    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.setText("MASTER", dontSendNotification);
    masterVolumeLabel.setJustificationType(Justification::centred);
    masterVolumeLabel.setColour(Label::textColourId, knobGrey);

    addAndMakeVisible(driveSlider);
    driveSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 18);
    driveSlider.setColour(Slider::rotarySliderFillColourId, cFx);
    driveSlider.setColour(Slider::thumbColourId, cFx);
    driveAttach.reset(new AudioProcessorValueTreeState::SliderAttachment(audioProcessor.apvts, "drive", driveSlider));
    addAndMakeVisible(driveLabel);
    driveLabel.setText("DRIVE", dontSendNotification);
    driveLabel.setJustificationType(Justification::centred);
    driveLabel.setColour(Label::textColourId, knobGrey);

    // Voice pads
    for (int i = 0; i < 12; ++i)
    {
        addAndMakeVisible(voiceButtons[i]);
        voiceButtons[i].setButtonText(LPDrumMachineAudioProcessor::voiceNames[i]);
        voiceButtons[i].setClickingTogglesState(true);
        voiceButtons[i].setRadioGroupId(42);
        styleButton(voiceButtons[i], Colour(0xff3a3d45));
        voiceButtons[i].onClick = [this, i] { selectVoice(i); };
    }
    voiceButtons[0].setToggleState(true, dontSendNotification);

    // Transport buttons
    addAndMakeVisible(learnButton);
    learnButton.setButtonText("Learn");
    styleButton(learnButton, Colour(0xff2e7d32));
    learnButton.onClick = [this] { audioProcessor.startLearning(selectedVoice); };

    addAndMakeVisible(testButton);
    testButton.setButtonText("Test");
    styleButton(testButton, Colour(0xffef6c00));
    testButton.onClick = [this] { audioProcessor.triggerDrum(selectedVoice); };

    addAndMakeVisible(resetButton);
    resetButton.setButtonText("Reset");
    styleButton(resetButton, Colour(0xff546e7a));
    resetButton.onClick = [this] { audioProcessor.resetVoice(selectedVoice); };

    addAndMakeVisible(noteLabel);
    noteLabel.setJustificationType(Justification::centred);
    noteLabel.setColour(Label::textColourId, Colours::white);

    // Copy / Paste / Rand
    addAndMakeVisible(copyButton);
    copyButton.setButtonText("Copy");
    styleButton(copyButton, Colour(0xff5e35b1));
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
    styleButton(pasteButton, Colour(0xff5e35b1));
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
    styleButton(randButton, Colour(0xffad1457));
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

    // Comboboxes styling
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
        c->setColour(ComboBox::backgroundColourId, Colour(0xff33363f));
        c->setColour(ComboBox::textColourId, Colours::white);
        c->setColour(ComboBox::outlineColourId, cardLineCol);
    }

    // Section titles (visible flag; text/colour set in resized)
    for (auto* t : { &osc1Title, &osc2Title, &noiseTitle, &filtTitle, &envTitle, &pitchTitle,
                     &volTitle, &lfoTitle, &fxTitle, &smpTitle, &outTitle })
        addAndMakeVisible(t);

    // Knobs (rotary) + name labels
    auto initSlider = [this](Slider& s, Label& l, const char* text)
        {
            s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(Slider::TextBoxBelow, false, 64, 16);
            l.setText(text, dontSendNotification);
            l.setJustificationType(Justification::centred);
            l.setColour(Label::textColourId, knobGrey);
            addAndMakeVisible(s);
            addAndMakeVisible(l);
        };

    initSlider(osc1Pitch, osc1PitchLabel, "Pitch");
    initSlider(osc1Level, osc1LevelLabel, "Level");
    initSlider(osc2Pitch, osc2PitchLabel, "Pitch");
    initSlider(osc2Level, osc2LevelLabel, "Level");
    initSlider(noiseLevel, noiseLevelLabel, "Level");
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

    // Sample buttons
    addAndMakeVisible(loadSampleButton);
    loadSampleButton.setButtonText("Load SMP");
    styleButton(loadSampleButton, cSmp.darker(0.4f));
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
    styleButton(clearSampleButton, Colour(0xff546e7a));
    clearSampleButton.onClick = [this]
        {
            audioProcessor.clearSample(selectedVoice);
            updateSampleInfo();
        };

    addAndMakeVisible(sampleInfoLabel);
    sampleInfoLabel.setText("(no sample)", dontSendNotification);
    sampleInfoLabel.setColour(Label::textColourId, knobGrey);

    // Preset row buttons
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save");
    styleButton(saveButton, Colour(0xff0277bd));
    saveButton.onClick = [this]
        {
            chooser = std::make_unique<FileChooser>("Save LP kit", File(), "*.lpdk");
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
    styleButton(loadButton, Colour(0xff0277bd));
    loadButton.onClick = [this]
        {
            chooser = std::make_unique<FileChooser>("Load LP kit", File(), "*.lpdk");
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

    // Cards background
    for (int i = 0; i < 10; ++i)
    {
        if (cardRects[i].isEmpty()) continue;
        g.setColour(cardCol);
        g.fillRoundedRectangle(cardRects[i].toFloat(), 6.0f);
        g.setColour(cardLineCol);
        g.drawRoundedRectangle(cardRects[i].toFloat(), 6.0f, 1.0f);
    }

    // Header title
    g.setColour(Colours::white);
    g.setFont(22.0f);
    g.drawText("LP DRUM MACHINE - SYNTH ENGINE",
        getLocalBounds().removeFromTop(44).withTrimmedLeft(540),
        Justification::centredLeft);

    // Logo card text
    if (!cardRects[9].isEmpty())
    {
        g.setColour(knobGrey);
        g.setFont(14.0f);
        g.drawText("LP-DM v1.0\nJUCE 9 / VST3", cardRects[9], Justification::centred);
    }
}

void LPDrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Header: presets + file buttons
    auto header = area.removeFromTop(44);
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

    // Voice pads
    auto pads = area.removeFromTop(44);
    const int bw = pads.getWidth() / 12;
    for (int i = 0; i < 12; ++i)
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
    auto startCard = [this, &ci, &R](Label& title, const char* text, Colour col) -> Rectangle<int>
        {
            auto r = R[ci];
            cardRects[ci] = r;
            ++ci;
            title.setText(text, dontSendNotification);
            title.setColour(Label::textColourId, col);
            title.setJustificationType(Justification::centredLeft);
            title.setBounds(r.removeFromTop(22).reduced(8, 2));
            return r.reduced(8, 2);
        };

    auto knobRow = [this](Rectangle<int>& r, Colour col, int n,
        std::initializer_list<std::pair<Slider*, Label*>> items)
        {
            auto row = r.removeFromTop(80);
            const int w = row.getWidth() / n;
            for (auto& pr : items)
            {
                auto k = row.removeFromLeft(w).reduced(3, 0);
                pr.second->setBounds(k.removeFromTop(16));
                pr.first->setBounds(k);
                pr.first->setColour(Slider::rotarySliderFillColourId, col);
                pr.first->setColour(Slider::thumbColourId, col);
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

    // Card 0: OSC 1
    auto r0 = startCard(osc1Title, "OSC 1", cOsc);
    comboRow1(r0, osc1WaveCombo);
    knobRow(r0, cOsc, 2, { { &osc1Pitch, &osc1PitchLabel }, { &osc1Level, &osc1LevelLabel } });

    // Card 1: OSC 2
    auto r1 = startCard(osc2Title, "OSC 2", cOsc2);
    comboRow1(r1, osc2WaveCombo);
    knobRow(r1, cOsc2, 2, { { &osc2Pitch, &osc2PitchLabel }, { &osc2Level, &osc2LevelLabel } });

    // Card 2: FILTER
    auto r2 = startCard(filtTitle, "FILTER", cFilt);
    comboRow1(r2, filtTypeCombo);
    knobRow(r2, cFilt, 2, { { &filtCutoff, &filtCutoffLabel }, { &filtRes, &filtResLabel } });
    knobRow(r2, cFilt, 2, { { &filtEnv, &filtEnvLabel }, { &filtDecay, &filtDecayLabel } });

    // Card 3: ENVELOPES
    auto r3 = startCard(envTitle, "ENVELOPES", cEnv);
    knobRow(r3, cEnv, 2, { { &ampAttack, &ampAttackLabel }, { &ampDecay, &ampDecayLabel } });
    knobRow(r3, cEnv, 2, { { &pitchEnvAmt, &pitchEnvLabel }, { &pitchDecay, &pitchDecayLabel } });

    // Card 4: LFO
    auto r4 = startCard(lfoTitle, "LFO", cLfo);
    comboRow2(r4, lfoShapeCombo, lfoTargetCombo);
    knobRow(r4, cLfo, 2, { { &lfoRate, &lfoRateLabel }, { &lfoDepth, &lfoDepthLabel } });

    // Card 5: SAMPLE
    auto r5 = startCard(smpTitle, "SAMPLE", cSmp);
    {
        auto row = r5.removeFromTop(24);
        loadSampleButton.setBounds(row.removeFromLeft(90).reduced(2, 1));
        clearSampleButton.setBounds(row.removeFromLeft(46).reduced(2, 1));
        sampleInfoLabel.setBounds(row.reduced(2, 1));
    }
    knobRow(r5, cSmp, 3, { { &smpLevel, &smpLevelLabel }, { &smpRate, &smpRateLabel }, { &smpOffset, &smpOffsetLabel } });
    comboRow2(r5, smpRevCombo, smpLoopCombo);

    // Card 6: FX
    auto r6 = startCard(fxTitle, "FX", cFx);
    knobRow(r6, cFx, 3, { { &fxDrive, &fxDriveLabel }, { &fxBits, &fxBitsLabel }, { &fxDecim, &fxDecimLabel } });

    // Card 7: OUT / CHOKE
    auto r7 = startCard(outTitle, "OUT / CHOKE", cOut);
    knobRow(r7, cOut, 2, { { &volSlider, &volLabel }, { &panSlider, &panLabel } });
    comboRow1(r7, chokeCombo);

    // Card 8: NOISE
    auto r8 = startCard(noiseTitle, "NOISE", cNz);
    knobRow(r8, cNz, 1, { { &noiseLevel, &noiseLevelLabel } });

    // Card 9: logo (drawn in paint)
    cardRects[9] = R[9];
}