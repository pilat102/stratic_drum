#pragma once
#include <JuceHeader.h>

using namespace juce;

class LPDrumMachineAudioProcessor : public AudioProcessor
{
public:
    LPDrumMachineAudioProcessor();
    ~LPDrumMachineAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String& newName) override;

    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void startLearning(int drumIndex);
    void stopLearning();
    int  getLearnTarget() const { return learnTarget; }
    void triggerDrum(int index, float velocity = 0.9f);
    void resetVoice(int index);
    void applyFactoryPreset(int index);
    void saveKitToFile(const File& file);
    bool loadKitFromFile(const File& file);

    void loadSampleForVoice(int voice, const File& file);
    void clearSample(int voice);
    String getSampleName(int voice) const { return sampleNames[voice]; }

    static const int NUM_VOICES = 16;

    static const char* factoryPresetNames[];
    static const int factoryPresetCount;
    static String voiceParamId(int voice, const char* base);
    static const char* voiceNames[];
    static const char* paramBaseAt(int p);
    static int paramCount() { return 32; }

    AudioProcessorValueTreeState apvts;
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    int midiNoteMap[16] = { 36, 38, 42, 46, 41, 43, 45, 49,
                            51, 39, 37, 56, 70, 64, 75, 35 };

private:
    struct Voice
    {
        float o1phase = 0.0f, o2phase = 0.0f;
        float ampEnv = 0.0f, filtEnv = 0.0f, pitchEnv = 0.0f;
        float lfoPhase = 0.0f;
        int   envStage = 0;
        float fx1 = 0.0f, fx2 = 0.0f, fy1 = 0.0f, fy2 = 0.0f;
        float heldSample = 0.0f;
        int   decimCount = 0;
        float smpPos = 0.0f;
        float smpDir = 1.0f;
        bool  smpActive = false;
        float vel = 1.0f;
        float age = 0.0f;
        bool  active = false;
    };

    Voice voices[16];
    AudioBuffer<float> sampleBuffers[16];
    double sampleRates[16];
    String sampleNames[16];
    String samplePaths[16];

    // ---- Master FX state ----
    struct Comb { std::vector<float> buf; int idx = 0; float lp = 0.0f; };
    struct AllPass { std::vector<float> buf; int idx = 0; };

    std::vector<float> chorBuf, dlyBuf;
    int chorPos = 0, dlyPos = 0;
    float chorPhase = 0.0f;
    Comb combs[8];
    AllPass aps[4];
    float revFeedback = 0.84f;
    float compEnv = 0.0f;

    float processReverbSample(float in);

    double sampleRate = 44100.0;
    Random random;
    int learnTarget = -1;

    static const int P_COUNT = 32;
    std::atomic<float>* params[16][P_COUNT];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LPDrumMachineAudioProcessor)
};