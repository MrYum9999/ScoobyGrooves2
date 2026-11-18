# Scooby Grooves 2

Version 2 has its own name and plugin ID, so it installs alongside the original Scooby Grooves.

## What's new in 2
- Lo-Fi: much quieter, softer dust and a smoother crusher, so quiet note tails no longer sputter or pop.
- Glitch: rarer and gentler at low settings, and never louder than your playing. Every glitch lasts a whole note value.
- Glitch Sync switch: TEMPO starts glitches on your DAW's beat grid; PICK starts them on the notes you play. Standalone or with the transport stopped, Tempo runs at the DAW tempo or 120 BPM.
- While the DAW is playing, the cassette Wow swings once per bar, in time with the song.
- The GitHub build also makes a standalone app (ScoobyGrooves2.exe) you can run without a DAW.

A lo-fi guitar box: Lo-Fi, Cassette and Tube sections, each with its own on switch and Blend knob, plus a one-knob Glitch.

## Controls

**Lo-Fi** (orange)
- Bits: bit depth, from 16 bits (clean) down to 3 bits (crunchy)
- Rate: sample rate, from 44.1k (clean) down to 2k (gritty, aliased)
- Tone: low-pass filter that darkens the crushed sound
- Dust: crackle and surface noise
- Blend: how much of this section you hear

**Cassette** (teal)
- Wow: slow, drifting pitch wobble
- Flutter: fast tape shimmer
- Hiss: tape noise
- Age: older tape means duller highs, more low-end bump, more saturation and random level dropouts
- Blend

**Tube** (green)
- Drive: amount of saturation (the level stays roughly steady as you turn it up)
- Bias: adds asymmetry and warm even harmonics
- Warmth: rolls off the top end
- Sag: the tube "squish" and bloom when you dig in
- Blend

**Bottom deck**
- Routing: in Series the signal goes Tube > Cassette > Lo-Fi; in Parallel all three get the same guitar signal and are mixed side by side. In both modes, each Blend knob sets how much of its section you hear.
- Preset: ten starting points (Lunchbox Classic, Dusty Groove, Sunday Cassette, Warm Tube Strum, Crushed Funk, Wobbly Memory, Broken Walkman, Bedroom Soul, Glitch Picnic, Clean Lunch)
- Glitch: at 0 it's off. Around 10% you get about one gentle stutter every five bars; at 50% about one per bar; at 100% two or three per bar. Wilder types unlock as you turn up: stutters and chops first, then reverse, crush bursts, tape stops and half-speed warbles. The Active lamp lights up while a glitch is happening.
- Glitch Sync: TEMPO (on the beat grid) or PICK (on your picked notes).
- Mix: dry/wet for the whole plugin
- Output: final level

Double-click any knob to reset it.

## Build on GitHub (no tools needed)

1. Create a new repository named `ScoobyGrooves2`.
2. Upload `CMakeLists.txt`, `README.md`, the `Source` folder and the `Fonts` folder.
3. Add the file `.github/workflows/build.yml`, using the contents of `build.yml` in this folder.
4. Wait for the green check under **Actions**, then download **ScoobyGrooves2-VST3** (and **ScoobyGrooves2-Standalone** if you want the app) from Artifacts.
5. Copy `ScoobyGrooves2.vst3` into `C:\Program Files\Common Files\VST3` and rescan plugins in your DAW.

## Fonts

Shrikhand (Jonny Pinhorn) and Righteous (Astigmatic) are used under the SIL Open Font License; the licence files are in the `Fonts` folder.
