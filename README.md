# LP Drum Machine

A 12-voice hybrid drum synth plugin (VST3, Standalone) built with JUCE 9.
Each voice combines synthesized oscillators, noise, a multimode filter, envelopes, LFO, per-voice FX, and a sample layer.

![LP Drum Machine](docs/screenshot.png)

## Features

- **12 voices**: Kick, Snare, HH Closed/Open, Tom Low/Mid/High, Crash, Ride, Clap, Rim, Cowbell
- **Hybrid synthesis per voice**:
  - 2 oscillators (Sine/Tri/Saw/Square) + noise source
  - Multimode filter (LP/HP/BP) with resonance and filter envelope
  - AMP and Pitch envelopes
  - LFO with 3 targets (Cutoff / Pitch / Volume)
  - Per-voice FX: Drive, Bitcrusher, Decimator
  - **Sample layer**: load any wav/aiff/flac/mp3/ogg, with reverse / loop / offset / pitch
- **Choke groups**: hi-hats and other voices can mute each other
- **Pan** per voice with equal-power stereo placement
- **MIDI-learn**: assign any pad on your controller to any voice
- **Kit management**: save/load custom kits to `.lpdk` files, 4 factory presets included
- **Copy / Paste / Random** for fast sound design

## Factory presets

| Preset | Character |
|---|---|
| Factory Default | Balanced neutral kit |
| **Linkin Park Kit** | Tight, dry, punchy — HT/Meteora era |
| 909 Electro | Deep kick, electro claps, LFO on toms |
| LoFi Boom Bap | Saturated, filtered, 12-bit textures |

## Build from source (Windows)

1. Install **JUCE 9** from https://juce.com/get-juce (e.g. to `C:\JUCE`).
2. Install **Visual Studio 2022 or 2026** with the **Desktop development with C++** workload.
3. Open `LPDrumMachine2.jucer` in **Projucer**. If the modules path is missing, set it to `C:\JUCE\modules` and press **Ctrl+S**.
4. Click **Save and open in IDE**.
5. In Visual Studio, select **Release | x64** and **Build Solution**.
6. Outputs:
   - `Builds/VisualStudio2026/x64/Release/VST3/LPDrumMachine2.vst3`
   - `Builds/VisualStudio2026/x64/Release/Standalone Plugin/LPDrumMachine2.exe`

### First-run note

Smart App Control on Windows 11 may block unsigned binaries. Right-click the built `.exe` / `.vst3` → **Properties** → check **Unblock** (or disable Smart App Control).

## Default MIDI map

| Note | Voice | Note | Voice |
|---|---|---|---|
| C1 (36) | Kick | D#2 (51) | Ride |
| D1 (38) | Snare | D#1 (39) | Clap |
| A#0 (46) | HH Open | C#1 (37) | Rim |
| A#1 (58) | HH Closed | G#2 (56) | Cowbell |
| F1 (41) | Tom Low | | |
| G1 (43) | Tom Mid | | |
| A1 (45) | Tom High | | |
| C#2 (49) | Crash | | |

Click **Learn** on any voice and hit a pad on your controller to re-assign.

## Quick recipes

- **Tight rock kick**: OSC1 Sine 48Hz, Pitch Env 130, Decay 0.22, Drive 1.4
- **909-ish snare**: Noise 0.85, OSC1 Tri 210Hz, HP 500Hz, Decay 0.13
- **Hip-hop clap**: Noise 1.0, BP 1500Hz, Res 3, Bits 10, Decim 2
- **Lo-fi hat**: Noise 1.0, HP 7000Hz, Bits 12, Decay 0.04
- **Dub bongo (tom + LFO)**: Tom High, LFO on Pitch, Rate 6Hz, Depth 0.4

## Loading samples

1. Click a voice pad, then **Load SMP** in the SAMPLE card.
2. Choose a wav/aiff/flac/mp3/ogg file (stereo auto-summed to mono).
3. Dial **Smp Level** up to blend it with the synth layer.
4. Save the kit — the sample path is stored in the `.lpdk` file.

## License

MIT — see [LICENSE](LICENSE).