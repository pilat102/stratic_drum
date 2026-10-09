#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

using namespace juce;

static constexpr float twoPi = 2.0f * MathConstants<float>::pi;
static constexpr float quarterPi = 0.25f * MathConstants<float>::pi;
static const int NV = LPDrumMachineAudioProcessor::NUM_VOICES;

const char* LPDrumMachineAudioProcessor::voiceNames[] = {
    "KICK", "SNARE", "HH CL", "HH OP",
    "TOM L", "TOM M", "TOM H",
    "CRASH", "RIDE", "CLAP", "RIM", "COWBELL",
    "SHKR", "CONGA", "WBLK", "SUB"
};

static const char* bases[32] = {
    "vol", "o1w", "o1p", "o1l", "o2w", "o2p", "o2l", "nz", "ft",
    "fc", "res", "fenv", "fdec", "atk", "dec", "penv", "pdec",
    "lfor", "lfos", "lfod", "lfot",
    "fxd", "fxb", "fxm",
    "smpl", "smpr", "smprv", "smpoff", "smploop",
    "pan", "chk", "clk"
};

const char* LPDrumMachineAudioProcessor::paramBaseAt(int p)
{
    return bases[p];
}

enum PIdx
{
    P_VOL = 0, P_O1W, P_O1P, P_O1L, P_O2W, P_O2P, P_O2L, P_NZ, P_FT,
    P_FC, P_RES, P_FENV, P_FDEC, P_ATK, P_DEC, P_PENV, P_PDEC,
    P_LFOR, P_LFOS, P_LFOD, P_LFOT,
    P_FXD, P_FXB, P_FXM,
    P_SMPL, P_SMPR, P_SMPRV, P_SMPOFF, P_SMPLOOP,
    P_PAN, P_CHK, P_CLK
};

struct VDef {
    int o1w; float o1p, o1l; int o2w; float o2p, o2l; float nz;
    int ft; float fc, res, fenv, fdec, atk, dec, penv, pdec, vol, clk;
};

static const VDef vdefs[16] = {
    { 0,  50.0f, 1.0f,  0, 200.0f, 0.0f,  0.00f, 0,  8000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.30f, 120.0f, 0.06f, 0.95f, 0.30f },
    { 1, 180.0f, 0.5f,  0, 200.0f, 0.0f,  0.80f, 0,  9000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.15f,   0.0f, 0.05f, 0.90f, 0.25f },
    { 0, 200.0f, 0.0f,  0, 200.0f, 0.0f,  1.00f, 1,  7000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.05f,   0.0f, 0.05f, 0.60f, 0.00f },
    { 0, 200.0f, 0.0f,  0, 200.0f, 0.0f,  1.00f, 1,  6000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.40f,   0.0f, 0.05f, 0.55f, 0.00f },
    { 0,  90.0f, 1.0f,  0, 200.0f, 0.0f,  0.05f, 0,  5000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.40f,  60.0f, 0.05f, 0.85f, 0.15f },
    { 0, 130.0f, 1.0f,  0, 200.0f, 0.0f,  0.05f, 0,  5000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.35f,  60.0f, 0.05f, 0.85f, 0.15f },
    { 0, 170.0f, 1.0f,  0, 200.0f, 0.0f,  0.05f, 0,  6000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.30f,  60.0f, 0.05f, 0.85f, 0.15f },
    { 0, 200.0f, 0.0f,  0, 200.0f, 0.0f,  1.00f, 1,  5000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 1.20f,   0.0f, 0.05f, 0.70f, 0.00f },
    { 3, 300.0f, 0.15f, 0, 200.0f, 0.0f,  1.00f, 1,  6000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.80f,   0.0f, 0.05f, 0.50f, 0.00f },
    { 0, 200.0f, 0.0f,  0, 200.0f, 0.0f,  1.00f, 2,  1500.0f, 2.0f, 0.0f, 0.10f, 0.001f, 0.25f,   0.0f, 0.05f, 0.80f, 0.10f },
    { 3, 400.0f, 0.6f,  0, 200.0f, 0.0f,  0.30f, 0,  6000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.06f,   0.0f, 0.05f, 0.60f, 0.20f },
    { 3, 550.0f, 0.7f,  3, 820.0f, 0.5f,  0.00f, 0,  8000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.25f,   0.0f, 0.05f, 0.55f, 0.00f },
    { 0, 200.0f, 0.0f,  0, 200.0f, 0.0f,  1.00f, 1,  8000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.04f,   0.0f, 0.05f, 0.50f, 0.00f },
    { 0, 220.0f, 1.0f,  0, 200.0f, 0.0f,  0.05f, 0,  4000.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.25f,  60.0f, 0.05f, 0.80f, 0.20f },
    { 0, 200.0f, 0.0f,  3, 1200.0f, 0.8f, 0.00f, 2, 2500.0f, 3.0f, 0.0f, 0.10f, 0.001f, 0.05f,   0.0f, 0.05f, 0.60f, 0.00f },
    { 0,  35.0f, 1.0f,  0, 200.0f, 0.0f,  0.00f, 0,   500.0f, 0.7f, 0.0f, 0.10f, 0.001f, 0.50f,  60.0f, 0.06f, 0.90f, 0.00f }
};

String LPDrumMachineAudioProcessor::voiceParamId(int voice, const char* base)
{
    return "v" + String(voice) + "_" + base;
}

static inline float waveShape(float phase, int type)
{
    switch (type)
    {
    case 1:  return 4.0f * std::fabs(phase - 0.5f) - 1.0f;   // Tri
    case 2:  return 2.0f * phase - 1.0f;                      // Saw
    case 3:  return phase < 0.5f ? 1.0f : -1.0f;              // Square
    default: return std::sin(phase * twoPi);                 // Sine
    }
}

LPDrumMachineAudioProcessor::LPDrumMachineAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    for (int v = 0; v < NV; ++v)
    {
        sampleRates[v] = 44100.0;
        for (int p = 0; p < P_COUNT; ++p)
            params[v][p] = apvts.getRawParameterValue(voiceParamId(v, bases[p]));
    }
}

LPDrumMachineAudioProcessor::~LPDrumMachineAudioProcessor()
{
}

AudioProcessorValueTreeState::ParameterLayout LPDrumMachineAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto add = [&params](const String& id, const String& name, float min, float max, float def)
        {
            params.push_back(std::make_unique<AudioParameterFloat>(ParameterID{ id, 1 }, name, min, max, def));
        };

    auto addSkew = [&params](const String& id, const String& name, float min, float max, float def, float skew)
        {
            params.push_back(std::make_unique<AudioParameterFloat>(ParameterID{ id, 1 }, name,
                NormalisableRange<float>(min, max, 1.0f, skew), def));
        };

    auto addChoice = [&params](const String& id, const String& name, StringArray choices, int def)
        {
            params.push_back(std::make_unique<AudioParameterChoice>(ParameterID{ id, 1 }, name, choices, def));
        };

    add("master_volume", "Master Volume", 0.0f, 1.0f, 0.85f);
    add("drive", "Drive", 0.0f, 2.0f, 1.3f);

    add("chor_mix", "Chorus Mix", 0.0f, 1.0f, 0.0f);
    add("chor_rate", "Chorus Rate", 0.1f, 5.0f, 0.8f);
    add("chor_depth", "Chorus Depth", 0.0f, 1.0f, 0.4f);
    add("dly_mix", "Delay Mix", 0.0f, 1.0f, 0.0f);
    add("dly_time", "Delay Time", 0.02f, 0.6f, 0.25f);
    add("dly_fdb", "Delay Fdb", 0.0f, 0.85f, 0.35f);
    add("rev_mix", "Reverb Mix", 0.0f, 1.0f, 0.0f);
    add("rev_size", "Reverb Size", 0.1f, 1.0f, 0.5f);
    add("comp_amt", "Comp", 0.0f, 1.0f, 0.3f);

    const StringArray waves{ "Sine", "Tri", "Saw", "Square" };
    const StringArray filts{ "LP", "HP", "BP" };
    const StringArray targets{ "Off", "Cutoff", "Pitch", "Volume" };
    const StringArray chokes{ "Off", "1", "2", "3", "4" };
    const StringArray revs{ "Fwd", "Rev" };
    const StringArray loops{ "Off", "Loop" };

    for (int v = 0; v < NV; ++v)
    {
        const auto& d = vdefs[v];
        String n = String(voiceNames[v]) + " ";

        add(voiceParamId(v, "vol"), n + "Vol", 0.0f, 1.0f, d.vol);
        addChoice(voiceParamId(v, "o1w"), n + "Osc1 Wave", waves, d.o1w);
        add(voiceParamId(v, "o1p"), n + "Osc1 Pitch", 20.0f, 600.0f, d.o1p);
        add(voiceParamId(v, "o1l"), n + "Osc1 Level", 0.0f, 1.0f, d.o1l);
        addChoice(voiceParamId(v, "o2w"), n + "Osc2 Wave", waves, d.o2w);
        add(voiceParamId(v, "o2p"), n + "Osc2 Pitch", 20.0f, 1200.0f, d.o2p);
        add(voiceParamId(v, "o2l"), n + "Osc2 Level", 0.0f, 1.0f, d.o2l);
        add(voiceParamId(v, "nz"), n + "Noise", 0.0f, 1.0f, d.nz);
        addChoice(voiceParamId(v, "ft"), n + "Filter Type", filts, d.ft);
        addSkew(voiceParamId(v, "fc"), n + "Cutoff", 20.0f, 18000.0f, d.fc, 0.3f);
        add(voiceParamId(v, "res"), n + "Resonance", 0.5f, 10.0f, d.res);
        add(voiceParamId(v, "fenv"), n + "Filt Env", -8000.0f, 8000.0f, d.fenv);
        add(voiceParamId(v, "fdec"), n + "Filt Decay", 0.01f, 2.0f, d.fdec);
        add(voiceParamId(v, "atk"), n + "Attack", 0.0005f, 0.1f, d.atk);
        add(voiceParamId(v, "dec"), n + "Decay", 0.02f, 3.0f, d.dec);
        add(voiceParamId(v, "penv"), n + "Pitch Env", 0.0f, 500.0f, d.penv);
        add(voiceParamId(v, "pdec"), n + "Pitch Decay", 0.01f, 1.0f, d.pdec);
        add(voiceParamId(v, "lfor"), n + "LFO Rate", 0.1f, 20.0f, 5.0f);
        addChoice(voiceParamId(v, "lfos"), n + "LFO Shape", waves, 0);
        add(voiceParamId(v, "lfod"), n + "LFO Depth", 0.0f, 1.0f, 0.0f);
        addChoice(voiceParamId(v, "lfot"), n + "LFO Target", targets, 0);
        add(voiceParamId(v, "fxd"), n + "FX Drive", 0.5f, 4.0f, 1.0f);
        add(voiceParamId(v, "fxb"), n + "FX Bits", 2.0f, 16.0f, 16.0f);
        add(voiceParamId(v, "fxm"), n + "FX Decim", 1.0f, 32.0f, 1.0f);
        add(voiceParamId(v, "smpl"), n + "Smp Level", 0.0f, 1.0f, 0.0f);
        add(voiceParamId(v, "smpr"), n + "Smp Rate", 0.25f, 4.0f, 1.0f);
        addChoice(voiceParamId(v, "smprv"), n + "Smp Rev", revs, 0);
        add(voiceParamId(v, "smpoff"), n + "Smp Offset", 0.0f, 1.0f, 0.0f);
        addChoice(voiceParamId(v, "smploop"), n + "Smp Loop", loops, 0);
        add(voiceParamId(v, "pan"), n + "Pan", -1.0f, 1.0f, 0.0f);
        addChoice(voiceParamId(v, "chk"), n + "Choke", chokes, (v == 2 || v == 3) ? 1 : 0);
        add(voiceParamId(v, "clk"), n + "Click", 0.0f, 1.0f, d.clk);
    }

    return { params.begin(), params.end() };
}

const String LPDrumMachineAudioProcessor::getName() const { return JucePlugin_Name; }
bool LPDrumMachineAudioProcessor::acceptsMidi() const { return true; }
bool LPDrumMachineAudioProcessor::producesMidi() const { return false; }
bool LPDrumMachineAudioProcessor::isMidiEffect() const { return false; }
double LPDrumMachineAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int LPDrumMachineAudioProcessor::getNumPrograms() { return 1; }
int LPDrumMachineAudioProcessor::getCurrentProgram() { return 0; }
void LPDrumMachineAudioProcessor::setCurrentProgram(int) {}
const String LPDrumMachineAudioProcessor::getProgramName(int) { return {}; }
void LPDrumMachineAudioProcessor::changeProgramName(int, const String&) {}

void LPDrumMachineAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;

    const int cN = (int)(sr * 0.05) + 4;
    const int dN = (int)(sr * 0.6) + 4;
    chorBuf.assign((size_t)cN, 0.0f);
    dlyBuf.assign((size_t)dN, 0.0f);
    chorPos = dlyPos = 0;
    chorPhase = 0.0f;

    const double k = sr / 44100.0;
    const int combLen[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int apLen[4] = { 556, 441, 341, 225 };
    for (int i = 0; i < 8; ++i)
    {
        combs[i].buf.assign((size_t)jmax(16, (int)(combLen[i] * k)), 0.0f);
        combs[i].idx = 0;
        combs[i].lp = 0.0f;
    }
    for (int i = 0; i < 4; ++i)
    {
        aps[i].buf.assign((size_t)jmax(8, (int)(apLen[i] * k)), 0.0f);
        aps[i].idx = 0;
    }
    compEnv = 0.0f;
}

void LPDrumMachineAudioProcessor::releaseResources() {}

float LPDrumMachineAudioProcessor::processReverbSample(float in)
{
    float acc = 0.0f;

    for (auto& c : combs)
    {
        const int n = (int)c.buf.size();
        const float o = c.buf[(size_t)c.idx];
        c.lp = o * 0.5f + c.lp * 0.5f;
        c.buf[(size_t)c.idx] = in + c.lp * revFeedback;
        c.idx = (c.idx + 1) % n;
        acc += o;
    }

    acc *= 0.12f;

    for (auto& a : aps)
    {
        const int n = (int)a.buf.size();
        const float o = a.buf[(size_t)a.idx];
        a.buf[(size_t)a.idx] = acc + o * 0.5f;
        acc = o - acc;
        a.idx = (a.idx + 1) % n;
    }

    return acc;
}

void LPDrumMachineAudioProcessor::startLearning(int drumIndex) { learnTarget = drumIndex; }
void LPDrumMachineAudioProcessor::stopLearning() { learnTarget = -1; }

void LPDrumMachineAudioProcessor::triggerDrum(int index, float velocity)
{
    if (index < 0 || index >= NV)
        return;

    const int g = (int)params[index][P_CHK]->load();
    if (g > 0)
    {
        for (int j = 0; j < NV; ++j)
        {
            if (j != index && (int)params[j][P_CHK]->load() == g)
            {
                voices[j].active = false;
                voices[j].smpActive = false;
                voices[j].ampEnv = 0.0f;
            }
        }
    }

    auto& v = voices[index];
    v.active = true;
    v.envStage = 1;
    v.ampEnv = 0.0f;
    v.filtEnv = 1.0f;
    v.pitchEnv = 1.0f;
    v.o1phase = 0.0f;
    v.o2phase = 0.0f;
    v.fx1 = v.fx2 = v.fy1 = v.fy2 = 0.0f;
    v.heldSample = 0.0f;
    v.decimCount = 0;
    v.vel = velocity;
    v.age = 0.0f;

    const int len = sampleBuffers[index].getNumSamples();
    const int rv = (int)params[index][P_SMPRV]->load();
    const float off = params[index][P_SMPOFF]->load();
    v.smpDir = (rv == 1) ? -1.0f : 1.0f;
    v.smpPos = (rv == 1) ? (float)jmax(0, len - 1) : off * (float)jmax(0, len - 1);
    v.smpActive = true;
}

void LPDrumMachineAudioProcessor::resetVoice(int index)
{
    if (index < 0 || index >= NV)
        return;

    const auto& d = vdefs[index];

    const std::pair<const char*, float> vals[] = {
        { "vol", d.vol }, { "o1p", d.o1p }, { "o1l", d.o1l },
        { "o2p", d.o2p }, { "o2l", d.o2l }, { "nz", d.nz },
        { "fc", d.fc },   { "res", d.res }, { "fenv", d.fenv },
        { "fdec", d.fdec }, { "atk", d.atk }, { "dec", d.dec },
        { "penv", d.penv }, { "pdec", d.pdec },
        { "lfor", 5.0f }, { "lfod", 0.0f },
        { "fxd", 1.0f },  { "fxb", 16.0f }, { "fxm", 1.0f },
        { "smpl", 0.0f }, { "smpr", 1.0f }, { "smpoff", 0.0f },
        { "pan", 0.0f },  { "clk", d.clk }
    };

    for (const auto& p : vals)
    {
        if (auto* param = apvts.getParameter(voiceParamId(index, p.first)))
            param->setValueNotifyingHost(param->convertTo0to1(p.second));
    }

    const std::pair<const char*, float> choices[] = {
        { "o1w", (float)d.o1w }, { "o2w", (float)d.o2w }, { "ft", (float)d.ft },
        { "lfos", 0.0f }, { "lfot", 0.0f },
        { "smprv", 0.0f }, { "smploop", 0.0f },
        { "chk", (float)((index == 2 || index == 3) ? 1 : 0) }
    };

    for (const auto& p : choices)
    {
        if (auto* param = apvts.getParameter(voiceParamId(index, p.first)))
            param->setValueNotifyingHost(param->convertTo0to1(p.second));
    }
}

//==============================================================================
// Presets (voice >= 0 = voice param, voice = -1 = global master FX)
const char* LPDrumMachineAudioProcessor::factoryPresetNames[] = {
    "Factory Default", "Linkin Park Kit", "909 Electro", "LoFi Boom Bap",
    "Analog 808", "Synthwave Pulse", "Horror Ritual", "Metal Forge"
};
const int LPDrumMachineAudioProcessor::factoryPresetCount = 8;

struct Ov { int voice; const char* base; float value; };

static const Ov lpKitOv[] = {
    { 0, "o1p", 48.0f },  { 0, "dec", 0.22f }, { 0, "penv", 130.0f }, { 0, "clk", 0.35f },
    { 1, "o1p", 210.0f }, { 1, "dec", 0.12f }, { 1, "nz", 0.85f }, { 1, "clk", 0.3f },
    { 2, "vol", 0.55f },  { 2, "dec", 0.04f },
    { 3, "vol", 0.50f },  { 3, "dec", 0.25f },
    { 4, "dec", 0.35f },  { 5, "dec", 0.30f }, { 6, "dec", 0.26f },
    { 7, "dec", 0.90f },  { 8, "dec", 0.50f }, { 9, "dec", 0.18f }
};

static const Ov electroOv[] = {
    { 0, "dec", 0.35f },  { 0, "penv", 160.0f },
    { 1, "nz", 1.0f },    { 1, "o1l", 0.25f }, { 1, "dec", 0.18f },
    { 2, "dec", 0.03f },  { 3, "dec", 0.60f },
    { 9, "res", 3.0f },   { 9, "dec", 0.30f },
    { 9, "fxb", 10.0f },  { 9, "fxm", 2.0f },
    { 11, "dec", 0.30f }, { 11, "vol", 0.60f }, { 11, "fxd", 2.0f },
    { 6, "lfot", 2.0f },  { 6, "lfod", 0.35f }, { 6, "lfor", 6.0f },
    { -1, "rev_mix", 0.15f }
};

static const Ov lofiOv[] = {
    { 0, "fc", 2500.0f }, { 0, "dec", 0.28f }, { 0, "fxd", 1.6f },
    { 1, "fc", 5000.0f }, { 1, "dec", 0.20f }, { 1, "fxb", 10.0f }, { 1, "fxm", 2.0f },
    { 2, "fc", 5000.0f }, { 2, "vol", 0.50f }, { 2, "fxb", 12.0f },
    { 7, "dec", 1.50f },
    { 8, "lfot", 3.0f },  { 8, "lfod", 0.30f }, { 8, "lfor", 7.0f },
    { -1, "rev_mix", 0.18f }
};

static const Ov analog808Ov[] = {
    { 0, "o1p", 40.0f }, { 0, "dec", 0.60f }, { 0, "penv", 180.0f },
    { 1, "nz", 1.0f },   { 1, "o1l", 0.3f },  { 1, "dec", 0.20f },
    { 2, "dec", 0.04f }, { 3, "dec", 0.50f },
    { 4, "dec", 0.50f }, { 5, "dec", 0.45f }, { 6, "dec", 0.40f },
    { 7, "dec", 1.30f },
    { 9, "res", 2.5f },  { 9, "dec", 0.25f },
    { 11, "dec", 0.35f },
    { -1, "comp_amt", 0.5f }
};

static const Ov synthwaveOv[] = {
    { 0, "dec", 0.30f }, { 0, "penv", 100.0f },
    { 1, "dec", 0.25f }, { 1, "fxb", 12.0f },
    { 2, "lfot", 3.0f }, { 2, "lfod", 0.6f }, { 2, "lfor", 8.0f }, { 2, "lfos", 3.0f },
    { 4, "o1w", 2.0f },  { 4, "dec", 0.45f },
    { 5, "o1w", 2.0f },  { 5, "dec", 0.40f },
    { 6, "o1w", 2.0f },  { 6, "dec", 0.35f },
    { 7, "dec", 1.40f },
    { 8, "lfot", 3.0f }, { 8, "lfod", 0.4f }, { 8, "lfor", 4.0f },
    { -1, "chor_mix", 0.35f }, { -1, "dly_mix", 0.18f }
};

static const Ov horrorOv[] = {
    { 0, "o1p", 35.0f }, { 0, "dec", 0.80f }, { 0, "penv", 60.0f }, { 0, "fc", 1500.0f },
    { 1, "dec", 0.40f }, { 1, "fc", 3000.0f }, { 1, "fxb", 10.0f },
    { 4, "o1p", 60.0f }, { 4, "dec", 0.80f }, { 4, "fc", 2000.0f },
    { 4, "lfot", 2.0f }, { 4, "lfor", 0.5f }, { 4, "lfod", 0.3f },
    { 5, "o1p", 80.0f }, { 5, "dec", 0.70f }, { 5, "fc", 2000.0f },
    { 6, "o1p", 100.0f },{ 6, "dec", 0.60f },
    { 7, "dec", 2.00f },
    { 11, "o1p", 300.0f }, { 11, "o2w", 3.0f }, { 11, "o2p", 410.0f },
    { 11, "o2l", 0.5f }, { 11, "dec", 0.6f },
    { -1, "rev_mix", 0.35f }, { -1, "rev_size", 0.9f },
    { -1, "dly_mix", 0.12f }, { -1, "dly_time", 0.45f }
};

static const Ov metalOv[] = {
    { 0, "o1p", 60.0f }, { 0, "dec", 0.18f }, { 0, "penv", 80.0f },
    { 0, "clk", 0.5f },  { 0, "fc", 9000.0f }, { 0, "fxd", 1.3f },
    { 1, "nz", 1.0f },   { 1, "o1l", 0.25f }, { 1, "dec", 0.12f },
    { 1, "clk", 0.35f }, { 1, "fc", 8000.0f },
    { 2, "dec", 0.03f }, { 2, "vol", 0.55f },
    { 3, "dec", 0.20f },
    { 4, "o1p", 90.0f }, { 4, "dec", 0.30f }, { 4, "clk", 0.2f }, { 4, "fc", 6000.0f },
    { 5, "o1p", 130.0f },{ 5, "dec", 0.26f }, { 5, "clk", 0.2f },
    { 6, "o1p", 170.0f },{ 6, "dec", 0.22f }, { 6, "clk", 0.2f },
    { 7, "dec", 1.10f },
    { -1, "rev_mix", 0.12f }, { -1, "comp_amt", 0.6f }, { -1, "dly_mix", 0.0f }
};

static const Ov* presetOv[] = { nullptr, lpKitOv, electroOv, lofiOv,
                                     analog808Ov, synthwaveOv, horrorOv, metalOv };
static const int presetOvCount[] = { 0,
                                     numElementsInArray(lpKitOv),
                                     numElementsInArray(electroOv),
                                     numElementsInArray(lofiOv),
                                     numElementsInArray(analog808Ov),
                                     numElementsInArray(synthwaveOv),
                                     numElementsInArray(horrorOv),
                                     numElementsInArray(metalOv) };

static const float presetDrive[] = { 1.3f, 1.4f, 1.2f, 1.8f, 1.1f, 1.3f, 1.6f, 1.4f };
static const float presetMaster[] = { 0.85f, 0.85f, 0.80f, 0.80f, 0.85f, 0.82f, 0.80f, 0.85f };

void LPDrumMachineAudioProcessor::applyFactoryPreset(int index)
{
    if (index < 0 || index >= factoryPresetCount)
        return;

    for (int v = 0; v < NV; ++v)
        resetVoice(v);

    auto setP = [this](const String& id, float value)
        {
            if (auto* p = apvts.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(value));
        };

    setP("master_volume", presetMaster[index]);
    setP("drive", presetDrive[index]);

    setP("chor_mix", 0.0f); setP("chor_rate", 0.8f); setP("chor_depth", 0.4f);
    setP("dly_mix", 0.0f);  setP("dly_time", 0.25f); setP("dly_fdb", 0.35f);
    setP("rev_mix", 0.0f);  setP("rev_size", 0.5f);  setP("comp_amt", 0.3f);

    const Ov* ov = presetOv[index];
    const int count = presetOvCount[index];
    for (int i = 0; i < count; ++i)
    {
        const String id = (ov[i].voice >= 0) ? voiceParamId(ov[i].voice, ov[i].base)
            : String(ov[i].base);
        setP(id, ov[i].value);
    }
}

//==============================================================================
// Samples
void LPDrumMachineAudioProcessor::loadSampleForVoice(int voice, const File& file)
{
    if (voice < 0 || voice >= NV)
        return;

    AudioFormatManager manager;
    manager.registerBasicFormats();

    std::unique_ptr<AudioFormatReader> reader(manager.createReaderFor(file));
    if (reader == nullptr)
        return;

    const int len = (int)reader->lengthInSamples;
    AudioBuffer<float> buf(1, jmax(1, len));
    buf.clear();

    if (reader->numChannels >= 2)
    {
        AudioBuffer<float> tmp((int)reader->numChannels, jmax(1, len));
        reader->read(&tmp, 0, len, 0, true, true);
        const float* l = tmp.getReadPointer(0);
        const float* r = tmp.getReadPointer(1);
        float* dst = buf.getWritePointer(0);
        for (int i = 0; i < len; ++i)
            dst[i] = (l[i] + r[i]) * 0.5f;
    }
    else
    {
        reader->read(&buf, 0, len, 0, true, true);
    }

    voices[voice].smpActive = false;
    sampleBuffers[voice] = std::move(buf);
    sampleRates[voice] = reader->sampleRate;
    sampleNames[voice] = file.getFileName();
    samplePaths[voice] = file.getFullPathName();
}

void LPDrumMachineAudioProcessor::clearSample(int voice)
{
    if (voice < 0 || voice >= NV)
        return;

    voices[voice].smpActive = false;
    sampleBuffers[voice].setSize(1, 1);
    sampleBuffers[voice].clear();
    sampleRates[voice] = 44100.0;
    sampleNames[voice] = String();
    samplePaths[voice] = String();
}

//==============================================================================
// Kit save/load
void LPDrumMachineAudioProcessor::saveKitToFile(const File& file)
{
    auto state = apvts.copyState();

    for (int i = 0; i < NV; ++i)
    {
        state.setProperty("midiNote_" + String(i), midiNoteMap[i], nullptr);
        state.setProperty("samplePath_" + String(i), samplePaths[i], nullptr);
    }

    if (auto xml = state.createXml())
        xml->writeTo(file);
}

bool LPDrumMachineAudioProcessor::loadKitFromFile(const File& file)
{
    auto xml = parseXML(file);
    if (xml == nullptr)
        return false;

    auto state = ValueTree::fromXml(*xml);
    if (!state.isValid())
        return false;

    apvts.replaceState(state);

    for (int i = 0; i < NV; ++i)
    {
        if (state.hasProperty("midiNote_" + String(i)))
            midiNoteMap[i] = (int)state.getProperty("midiNote_" + String(i));

        const String path = state.getProperty("samplePath_" + String(i)).toString();
        if (path.isNotEmpty() && File(path).existsAsFile())
            loadSampleForVoice(i, File(path));
        else
            clearSample(i);
    }

    return true;
}

void LPDrumMachineAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    buffer.clear();

    const float masterVol = apvts.getRawParameterValue("master_volume")->load();
    const float drive = apvts.getRawParameterValue("drive")->load();

    const float chorMix = apvts.getRawParameterValue("chor_mix")->load();
    const float chorRate = apvts.getRawParameterValue("chor_rate")->load();
    const float chorDepth = apvts.getRawParameterValue("chor_depth")->load();
    const float dlyMix = apvts.getRawParameterValue("dly_mix")->load();
    const float dlyTime = apvts.getRawParameterValue("dly_time")->load();
    const float dlyFdb = apvts.getRawParameterValue("dly_fdb")->load();
    const float revMix = apvts.getRawParameterValue("rev_mix")->load();
    const float revSize = apvts.getRawParameterValue("rev_size")->load();
    const float compAmt = apvts.getRawParameterValue("comp_amt")->load();

    revFeedback = 0.70f + 0.25f * revSize;

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            const float vel = msg.getFloatVelocity();

            if (learnTarget >= 0)
            {
                midiNoteMap[learnTarget] = note;
                learnTarget = -1;
                continue;
            }

            for (int i = 0; i < NV; ++i)
            {
                if (midiNoteMap[i] == note)
                {
                    triggerDrum(i, vel);
                    break;
                }
            }
        }
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    const float sr = (float)sampleRate;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float outL = 0.0f, outR = 0.0f;

        for (int d = 0; d < NV; ++d)
        {
            auto& v = voices[d];

            if (!v.active)
                continue;

            const float vol = params[d][P_VOL]->load();
            const int   o1w = (int)params[d][P_O1W]->load();
            const float o1p = params[d][P_O1P]->load();
            const float o1l = params[d][P_O1L]->load();
            const int   o2w = (int)params[d][P_O2W]->load();
            const float o2p = params[d][P_O2P]->load();
            const float o2l = params[d][P_O2L]->load();
            const float nz = params[d][P_NZ]->load();
            const int   ft = (int)params[d][P_FT]->load();
            const float fc = params[d][P_FC]->load();
            const float res = params[d][P_RES]->load();
            const float fenv = params[d][P_FENV]->load();
            const float fdec = params[d][P_FDEC]->load();
            const float atk = params[d][P_ATK]->load();
            const float dec = params[d][P_DEC]->load();
            const float penv = params[d][P_PENV]->load();
            const float pdec = params[d][P_PDEC]->load();
            const float lfor = params[d][P_LFOR]->load();
            const int   lfos = (int)params[d][P_LFOS]->load();
            const float lfod = params[d][P_LFOD]->load();
            const int   lfot = (int)params[d][P_LFOT]->load();
            const float fxd = params[d][P_FXD]->load();
            const float fxb = params[d][P_FXB]->load();
            const float fxm = params[d][P_FXM]->load();
            const float smpl = params[d][P_SMPL]->load();
            const float smpr = params[d][P_SMPR]->load();
            const float smpoff = params[d][P_SMPOFF]->load();
            const int   smploop = (int)params[d][P_SMPLOOP]->load();
            const float pan = params[d][P_PAN]->load();
            const float clk = params[d][P_CLK]->load();

            v.age += 1.0f / sr;

            if (v.envStage == 1)
            {
                v.ampEnv += 1.0f / jmax(0.0001f, atk * sr);
                if (v.ampEnv >= 1.0f) { v.ampEnv = 1.0f; v.envStage = 2; }
            }
            else
            {
                v.ampEnv *= 1.0f - 1.0f / (dec * sr);
            }

            v.filtEnv *= 1.0f - 1.0f / (fdec * sr);
            v.pitchEnv *= 1.0f - 1.0f / (pdec * sr);

            float vo = 0.0f;

            if (v.smpActive && smpl > 0.001f)
            {
                const int slen = sampleBuffers[d].getNumSamples();

                if (slen < 2)
                {
                    v.smpActive = false;
                }
                else
                {
                    v.smpPos += smpr * v.smpDir * (float)(sampleRates[d] / (double)sr);

                    bool stop = false;
                    if (v.smpDir > 0.0f)
                    {
                        if (v.smpPos >= (float)(slen - 1))
                        {
                            if (smploop == 1)
                                v.smpPos = smpoff * (float)(slen - 1);
                            else
                                stop = true;
                        }
                    }
                    else
                    {
                        if (v.smpPos <= 0.0f)
                            stop = true;
                    }

                    if (stop)
                    {
                        v.smpActive = false;
                    }
                    else
                    {
                        const float* rd = sampleBuffers[d].getReadPointer(0);
                        int i0 = (int)v.smpPos;
                        i0 = jlimit(0, slen - 2, i0);
                        const float frac = v.smpPos - (float)i0;
                        const float smp = rd[i0] + (rd[i0 + 1] - rd[i0]) * frac;
                        vo += smp * smpl * v.vel;
                    }
                }
            }
            else if (v.smpActive && smpl <= 0.001f)
            {
                v.smpActive = false;
            }

            if (v.ampEnv <= 0.001f && v.envStage == 2 && (!v.smpActive || smploop == 1))
            {
                v.active = false;
                continue;
            }

            v.lfoPhase += lfor / sr;
            if (v.lfoPhase >= 1.0f) v.lfoPhase -= 1.0f;
            const float lfoRaw = waveShape(v.lfoPhase, lfos);
            const float lfo = lfoRaw * lfod;

            const float drop = penv * v.pitchEnv;
            const float pitchMul = (lfot == 2) ? (1.0f + lfo * 0.5f) : 1.0f;
            v.o1phase += ((o1p + drop) * pitchMul) / sr;
            if (v.o1phase >= 1.0f) v.o1phase -= 1.0f;
            v.o2phase += ((o2p + drop) * pitchMul) / sr;
            if (v.o2phase >= 1.0f) v.o2phase -= 1.0f;

            const float noise = random.nextFloat() * 2.0f - 1.0f;
            const float click = (v.age < 0.002f) ? noise * clk * 2.0f : 0.0f;

            const float input = waveShape(v.o1phase, o1w) * o1l
                + waveShape(v.o2phase, o2w) * o2l
                + noise * nz
                + click;

            float cutoff = jlimit(20.0f, 18000.0f, fc + fenv * v.filtEnv);
            if (lfot == 1)
                cutoff = jlimit(20.0f, 18000.0f, cutoff * std::exp2(lfo * 3.0f));

            const float Q = jlimit(0.3f, 12.0f, res);
            const float w0 = twoPi * cutoff / sr;
            const float cw = std::cos(w0);
            const float sw = std::sin(w0);
            const float alpha = sw / (2.0f * Q);

            float b0, b1, b2;
            if (ft == 1) { b0 = (1.0f + cw) * 0.5f; b1 = -(1.0f + cw); b2 = b0; }
            else if (ft == 2) { b0 = alpha;              b1 = 0.0f;         b2 = -alpha; }
            else { b0 = (1.0f - cw) * 0.5f; b1 = 1.0f - cw;    b2 = b0; }

            const float a0 = 1.0f + alpha;
            b0 /= a0; b1 /= a0; b2 /= a0;
            const float na1 = (-2.0f * cw) / a0;
            const float na2 = (1.0f - alpha) / a0;

            const float x0 = input;
            float y0 = b0 * x0 + b1 * v.fx1 + b2 * v.fx2 - na1 * v.fy1 - na2 * v.fy2;

            if (!std::isfinite(y0))
            {
                v.fx1 = v.fx2 = v.fy1 = v.fy2 = 0.0f;
                y0 = 0.0f;
            }
            else
            {
                v.fx2 = v.fx1; v.fx1 = x0;
                v.fy2 = v.fy1; v.fy1 = y0;
            }

            float s = std::tanh(y0 * fxd);

            const int decim = jmax(1, (int)fxm);
            if (++v.decimCount >= decim)
            {
                v.decimCount = 0;
                v.heldSample = s;
            }
            else
            {
                s = v.heldSample;
            }

            const int bits = jlimit(2, 16, (int)fxb);
            const float steps = (float)((1 << bits) - 1);
            s = std::round(s * steps) / steps;

            float volEff = vol;
            if (lfot == 3)
                volEff = vol * (1.0f - lfod * (0.5f - 0.5f * lfoRaw));

            vo += s * v.ampEnv * volEff * v.vel;

            const float gL = std::cos((pan + 1.0f) * quarterPi);
            const float gR = std::sin((pan + 1.0f) * quarterPi);
            outL += vo * gL;
            outR += vo * gR;
        }

        // ---- Master chain ----
        float L = std::tanh(outL * drive) * masterVol;
        float R = std::tanh(outR * drive) * masterVol;
        const float send = (L + R) * 0.5f;

        const int cN = (int)chorBuf.size();
        if (cN > 4 && chorMix > 0.001f)
        {
            chorPhase += chorRate / sr;
            if (chorPhase >= 1.0f) chorPhase -= 1.0f;
            const float lfo = std::sin(chorPhase * twoPi);

            chorBuf[(size_t)chorPos] = send;
            const float dt = (0.012f + 0.006f * chorDepth * (lfo * 0.5f + 0.5f)) * sr;
            float rp = (float)chorPos - dt;
            while (rp < 0.0f) rp += (float)cN;
            const int i0 = (int)rp % cN;
            const int i1 = (i0 + 1) % cN;
            const float fr = rp - std::floor(rp);
            const float wet = chorBuf[(size_t)i0] + (chorBuf[(size_t)i1] - chorBuf[(size_t)i0]) * fr;
            chorPos = (chorPos + 1) % cN;

            L += wet * chorMix;
            R += wet * chorMix;
        }

        const int dN = (int)dlyBuf.size();
        if (dN > 4 && dlyMix > 0.001f)
        {
            const int dtS = jlimit(1, dN - 1, (int)(dlyTime * sr));
            const int readPos = (dlyPos - dtS + dN) % dN;
            const float dOut = dlyBuf[(size_t)readPos];
            dlyBuf[(size_t)dlyPos] = send + dOut * dlyFdb;
            dlyPos = (dlyPos + 1) % dN;

            L += dOut * dlyMix;
            R += dOut * dlyMix;
        }

        if (revMix > 0.001f)
        {
            const float rv = processReverbSample(send);
            L += rv * revMix;
            R += rv * revMix;
        }

        // Master limiter + safety soft clip
        const float peak = jmax(std::fabs(L), std::fabs(R));
        compEnv = jmax(compEnv * 0.9997f, peak);
        float g = 1.0f;
        if (compEnv > 0.7f)
        {
            const float target = 0.7f / compEnv;
            g = 1.0f + compAmt * (target - 1.0f);
        }
        L *= g;
        R *= g;

        L = std::tanh(L * 1.1f) / 1.1f;
        R = std::tanh(R * 1.1f) / 1.1f;

        left[i] = L;
        right[i] = R;
    }

    midiMessages.clear();
}

bool LPDrumMachineAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* LPDrumMachineAudioProcessor::createEditor()
{
    return new LPDrumMachineAudioProcessorEditor(*this);
}

void LPDrumMachineAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = apvts.copyState();

    for (int i = 0; i < NV; ++i)
    {
        state.setProperty("midiNote_" + String(i), midiNoteMap[i], nullptr);
        state.setProperty("samplePath_" + String(i), samplePaths[i], nullptr);
    }

    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LPDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto state = ValueTree::fromXml(*xmlState);
            apvts.replaceState(state);

            for (int i = 0; i < NV; ++i)
            {
                if (state.hasProperty("midiNote_" + String(i)))
                    midiNoteMap[i] = (int)state.getProperty("midiNote_" + String(i));

                const String path = state.getProperty("samplePath_" + String(i)).toString();
                if (path.isNotEmpty() && File(path).existsAsFile())
                    loadSampleForVoice(i, File(path));
                else
                    clearSample(i);
            }
        }
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LPDrumMachineAudioProcessor();
}