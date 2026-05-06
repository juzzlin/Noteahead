// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "synth_presets.hpp"

namespace noteahead {

const std::vector<SynthPreset>& SynthPresets::presets()
{
    static const std::vector<SynthPreset> presetList = {
        // --- BASS (0-19) ---
        {"Init", {}},
        {"Fat Bass", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.15f}, {"mixLevel1", 1.0f}, {"mixLevel2", 0.7f}, {"lpfCutoff", 0.25f}, {"lpfResonance", 0.3f}, {"ampDecay", 0.45f}, {"ampSustain", 0.4f}, {"ampRelease", 0.3f}}},
        {"Sub Bass", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", -1.0f}, {"mixLevel1", 1.0f}, {"mixLevel2", 0.5f}, {"lpfCutoff", 0.15f}, {"ampAttack", 0.05f}, {"ampRelease", 0.4f}}},
        {"Acid Line", {{"vco1Waveform", 1.0f}, {"lpfCutoff", 0.15f}, {"lpfResonance", 0.85f}, {"modTarget", 2.0f}, {"modIntensity", 0.9f}, {"modAttack", 0.0f}, {"modDecay", 0.42f}, {"ampDecay", 0.45f}, {"ampSustain", 0.0f}}},
        {"Reese Bass", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.5f}, {"vco2Pitch", 12.0f}, {"lpfCutoff", 0.2f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.7f}, {"lfoRate", 0.2f}}},
        {"Slap Bass", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"modTarget", 2.0f}, {"modIntensity", 0.8f}, {"modAttack", 0.0f}, {"modDecay", 0.25f}, {"ampAttack", 0.0f}, {"ampDecay", 0.35f}, {"ampSustain", 0.0f}}},
        {"Analog Moog", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Octave", -1.0f}, {"vco2Pitch", 5.0f}, {"mixLevel1", 0.8f}, {"mixLevel2", 0.6f}, {"lpfCutoff", 0.2f}, {"lpfResonance", 0.2f}, {"ampAttack", 0.02f}}},
        {"Dark FM Bass", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 1200.0f}, {"modTarget", 1.0f}, {"modIntensity", 0.75f}, {"lpfCutoff", 0.3f}, {"ampDecay", 0.4f}}},
        {"Driving Bass", {{"vco1Waveform", 1.0f}, {"multiMode", 3.0f}, {"multiLevel", 0.4f}, {"multiShape", 0.2f}, {"lpfCutoff", 0.3f}, {"ampDecay", 0.48f}}},
        {"Clicky Sub", {{"vco1Waveform", 0.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.7f}, {"modAttack", 0.0f}, {"modDecay", 0.1f}, {"lpfCutoff", 0.1f}, {"ampAttack", 0.0f}}},
        {"Pulse Bass", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"lfoTarget", 2.0f}, {"lfoIntensity", 0.8f}, {"lfoRate", 0.3f}, {"lpfCutoff", 0.3f}, {"ampDecay", 0.45f}}},
        {"Dirt Bass", {{"vco1Waveform", 1.0f}, {"multiMode", 2.0f}, {"multiLevel", 0.6f}, {"multiShape", 0.4f}, {"lpfCutoff", 0.2f}, {"lpfResonance", 0.4f}}},
        {"Organ Bass", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", 1.0f}, {"mixLevel1", 1.0f}, {"mixLevel2", 0.4f}, {"lpfCutoff", 0.4f}}},
        {"Wide Sub", {{"vco1Waveform", 0.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.1f}, {"panSpread", 0.8f}, {"lpfCutoff", 0.15f}}},
        {"Grit Bass", {{"vco1Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 700.0f}, {"lpfCutoff", 0.25f}, {"lpfResonance", 0.5f}}},
        {"Techno Bass", {{"vco1Waveform", 1.0f}, {"ampDecay", 0.35f}, {"ampSustain", 0.0f}, {"delayType", 2.0f}, {"delayMix", 0.25f}, {"delayTime", 0.375f}}},
        {"Soft Moog", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Octave", -1.0f}, {"lpfCutoff", 0.15f}, {"ampAttack", 0.08f}, {"ampRelease", 0.45f}}},
        {"Deep Sine", {{"vco1Waveform", 0.0f}, {"lpfCutoff", 0.1f}, {"lpfResonance", 0.0f}, {"ampAttack", 0.02f}}},
        {"Mod Bass", {{"vco1Waveform", 1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.85f}, {"lfoRate", 0.6f}, {"lpfCutoff", 0.2f}}},
        {"Reso Bass", {{"vco1Waveform", 2.0f}, {"multiMode", 2.0f}, {"multiLevel", 0.5f}, {"lpfResonance", 0.9f}, {"lpfCutoff", 0.1f}}},

        // --- LEADS (20-39) ---
        {"Power Lead", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.3f}, {"portamento", 0.15f}, {"lpfCutoff", 0.8f}, {"delayMix", 0.3f}}},
        {"Sync Lead", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"modTarget", 1.0f}, {"modIntensity", 0.95f}, {"modAttack", 0.6f}, {"modDecay", 0.45f}, {"lpfCutoff", 0.7f}}},
        {"Square Lead", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"portamento", 0.1f}, {"delayType", 2.0f}, {"delayMix", 0.4f}, {"delayTime", 0.5f}}},
        {"Tri Lead", {{"vco1Waveform", 0.0f}, {"portamento", 0.25f}, {"gain", 0.7f}, {"lpfCutoff", 0.9f}}},
        {"Folding Lead", {{"vco1Waveform", 2.0f}, {"lfoTarget", 2.0f}, {"lfoIntensity", 0.9f}, {"lfoRate", 0.4f}, {"hpfCutoff", 0.3f}}},
        {"Soft Lead", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Pitch", 10.0f}, {"ampAttack", 0.15f}, {"delayMix", 0.5f}}},
        {"Screamer", {{"vco1Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 3600.0f}, {"lpfResonance", 0.75f}, {"modTarget", 1.0f}, {"modIntensity", 0.8f}}},
        {"Detuned Saw", {{"vco1Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.8f}, {"panSpread", 0.6f}, {"lpfCutoff", 0.6f}}},
        {"Wobble Lead", {{"vco1Waveform", 1.0f}, {"lfoMode", 1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.9f}, {"lfoRate", 0.25f}, {"lpfCutoff", 0.3f}}},
        {"Fifth Lead", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 700.0f}, {"mixLevel1", 0.8f}, {"mixLevel2", 0.8f}}},
        {"Plastic Lead", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.85f}, {"lpfResonance", 0.6f}, {"ampDecay", 0.35f}, {"ampSustain", 0.2f}}},
        {"Whistle", {{"vco1Waveform", 0.0f}, {"vco1Octave", 2.0f}, {"lpfResonance", 0.9f}, {"portamento", 0.3f}}},
        {"Hero Lead", {{"vco1Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.2f}, {"portamento", 0.2f}, {"delayMix", 0.4f}, {"delayTime", 0.333f}}},
        {"Buzzy Lead", {{"vco1Waveform", 1.0f}, {"multiMode", 3.0f}, {"multiLevel", 0.7f}, {"multiShape", 0.1f}, {"lpfCutoff", 0.5f}}},
        {"Ghostly", {{"vco1Waveform", 0.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.6f}, {"lfoRate", 0.5f}, {"delayMix", 0.6f}, {"delayFeedback", 0.7f}}},
        {"Gritty Mono", {{"vco1Waveform", 2.0f}, {"multiMode", 3.0f}, {"multiLevel", 0.5f}, {"portamento", 0.1f}, {"lpfCutoff", 0.4f}}},
        {"Classic 80s", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 2.0f}, {"vco2Shape", 0.5f}, {"vco2Pitch", 8.0f}, {"lpfCutoff", 0.45f}}},
        {"Saw Swell", {{"vco1Waveform", 1.0f}, {"ampAttack", 0.6f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.8f}, {"lfoRate", 0.1f}}},
        {"Space Lead", {{"vco1Waveform", 0.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 2400.0f}, {"delayMix", 0.7f}, {"delayType", 5.0f}}},
        {"Aggro Sync", {{"vco1Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"modTarget", 2.0f}, {"modIntensity", 0.9f}, {"modDecay", 0.35f}}},

        // --- PADS (40-59) ---
        {"Soft Pad", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Pitch", 7.0f}, {"ampAttack", 0.8f}, {"ampRelease", 0.7f}, {"lpfCutoff", 0.35f}, {"panSpread", 0.5f}}},
        {"Deep Sea", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", -1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.4f}, {"lfoRate", 0.05f}, {"lpfCutoff", 0.2f}, {"ampAttack", 0.7f}}},
        {"Saw Pad", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 5.0f}, {"ampAttack", 0.75f}, {"ampRelease", 0.6f}, {"lpfCutoff", 0.25f}}},
        {"Strings", {{"vco1Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.4f}, {"ampAttack", 0.7f}, {"ampRelease", 0.65f}, {"panSpread", 0.9f}, {"lpfCutoff", 0.4f}}},
        {"Organ Pad", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", 1.0f}, {"ampAttack", 0.65f}, {"ampSustain", 1.0f}, {"lpfCutoff", 0.5f}}},
        {"Glassy", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.1f}, {"vco1Octave", 1.0f}, {"ampAttack", 0.6f}, {"delayMix", 0.5f}, {"hpfCutoff", 0.4f}}},
        {"Ethereal", {{"vco1Waveform", 2.0f}, {"lfoTarget", 2.0f}, {"lfoIntensity", 0.75f}, {"lfoRate", 0.1f}, {"delayMix", 0.65f}, {"delayType", 2.0f}, {"ampAttack", 0.8f}}},
        {"Warmth", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"lpfCutoff", 0.18f}, {"ampAttack", 0.75f}, {"ampSustain", 0.9f}}},
        {"Pulsing", {{"vco1Waveform", 1.0f}, {"lfoMode", 1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.85f}, {"lfoRate", 0.25f}, {"ampAttack", 0.6f}}},
        {"Cloudy", {{"vco1Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.9f}, {"lpfCutoff", 0.2f}, {"ampAttack", 0.85f}}},
        {"Vocaloid", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.8f}, {"lpfResonance", 0.85f}, {"lpfCutoff", 0.35f}, {"ampAttack", 0.7f}}},
        {"Dreamy", {{"vco1Waveform", 0.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.55f}, {"lfoRate", 0.08f}, {"delayMix", 0.6f}}},
        {"Ice Pad", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.05f}, {"vco1Octave", 2.0f}, {"hpfCutoff", 0.6f}, {"ampAttack", 0.75f}}},
        {"Aurora", {{"vco1Waveform", 1.0f}, {"lfoTarget", 2.0f}, {"lfoIntensity", 0.65f}, {"lfoRate", 0.05f}, {"panSpread", 0.8f}, {"ampAttack", 0.8f}}},
        {"Majestic", {{"vco1Waveform", 1.0f}, {"vco1Octave", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.3f}, {"ampAttack", 0.75f}}},
        {"Airy", {{"vco1Waveform", 0.0f}, {"hpfCutoff", 0.7f}, {"ampAttack", 0.8f}, {"delayMix", 0.4f}}},
        {"Twilight", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 0.0f}, {"ampAttack", 0.75f}, {"lpfCutoff", 0.25f}}},
        {"Retro Pad", {{"vco1Waveform", 1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.75f}, {"lfoRate", 0.15f}, {"ampAttack", 0.65f}, {"lpfCutoff", 0.35f}}},
        {"Darkness", {{"vco1Waveform", 1.0f}, {"vco1Octave", -1.0f}, {"lpfCutoff", 0.1f}, {"ampAttack", 0.85f}}},
        {"Heavenly", {{"vco1Waveform", 0.0f}, {"vco1Octave", 1.0f}, {"ampAttack", 0.8f}, {"delayMix", 0.65f}, {"delayType", 5.0f}}},

        // --- PLUCKS (60-79) ---
        {"Bright Pluck", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"modTarget", 2.0f}, {"modIntensity", 0.85f}, {"modDecay", 0.25f}, {"ampSustain", 0.0f}, {"ampDecay", 0.4f}, {"delayMix", 0.3f}}},
        {"Bell", {{"vco1Waveform", 0.0f}, {"vco1Octave", 2.0f}, {"vco2Waveform", 2.0f}, {"vco2Octave", 3.0f}, {"mixLevel2", 0.4f}, {"ampSustain", 0.0f}, {"ampDecay", 0.55f}, {"delayMix", 0.5f}}},
        {"Short Saw", {{"vco1Waveform", 1.0f}, {"lpfCutoff", 0.3f}, {"ampSustain", 0.0f}, {"ampDecay", 0.35f}}},
        {"Digital Pluck", {{"vco1Waveform", 2.0f}, {"multiMode", 3.0f}, {"multiLevel", 0.5f}, {"ampSustain", 0.0f}, {"ampDecay", 0.3f}}},
        {"Woody", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"lpfCutoff", 0.15f}, {"modTarget", 2.0f}, {"modIntensity", 0.75f}, {"modDecay", 0.2f}, {"ampSustain", 0.0f}}},
        {"Crystal", {{"vco1Waveform", 0.0f}, {"vco1Octave", 2.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.6f}, {"delayMix", 0.45f}}},
        {"Perc Bass", {{"vco1Waveform", 1.0f}, {"modTarget", 2.0f}, {"modIntensity", 0.8f}, {"modDecay", 0.15f}, {"ampSustain", 0.0f}, {"ampDecay", 0.3f}}},
        {"Stab", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 4.0f}, {"lpfCutoff", 0.45f}, {"ampSustain", 0.0f}, {"ampDecay", 0.45f}}},
        {"Mallet", {{"vco1Waveform", 0.0f}, {"vco1Octave", 1.0f}, {"multiMode", 2.0f}, {"multiLevel", 0.3f}, {"ampSustain", 0.0f}, {"ampDecay", 0.45f}}},
        {"Tines", {{"vco1Waveform", 0.0f}, {"vco1Octave", 1.0f}, {"vco2Waveform", 2.0f}, {"vco2Octave", 3.0f}, {"ampSustain", 0.0f}, {"ampDecay", 0.5f}}},
        {"Koto", {{"vco1Waveform", 1.0f}, {"vco1Shape", 0.9f}, {"lpfCutoff", 0.35f}, {"ampSustain", 0.0f}, {"ampDecay", 0.4f}}},
        {"Harp", {{"vco1Waveform", 0.0f}, {"vco2Pitch", 12.0f}, {"ampSustain", 0.0f}, {"ampDecay", 0.5f}, {"delayMix", 0.35f}}},
        {"Chirp", {{"vco1Waveform", 2.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.9f}, {"modDecay", 0.1f}, {"ampSustain", 0.0f}}},
        {"Bamboo", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 19.0f}, {"ampSustain", 0.0f}, {"ampDecay", 0.35f}}},
        {"Marimba", {{"vco1Waveform", 0.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.65f}, {"modDecay", 0.15f}, {"ampSustain", 0.0f}, {"ampDecay", 0.45f}}},
        {"Clav", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.2f}, {"lpfCutoff", 0.4f}, {"ampSustain", 0.0f}, {"ampDecay", 0.4f}}},
        {"Plastic Pluck", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.75f}, {"lpfCutoff", 0.3f}, {"ampSustain", 0.0f}, {"ampDecay", 0.35f}}},
        {"Square Stab", {{"vco1Waveform", 2.0f}, {"lpfCutoff", 0.45f}, {"ampSustain", 0.0f}, {"ampDecay", 0.25f}}},
        {"Glocken", {{"vco1Waveform", 0.0f}, {"vco1Octave", 3.0f}, {"ampSustain", 0.0f}, {"ampDecay", 0.6f}, {"delayMix", 0.45f}}},
        {"Shorty", {{"vco1Waveform", 1.0f}, {"lpfCutoff", 0.15f}, {"ampSustain", 0.0f}, {"ampDecay", 0.25f}}},

        // --- KEYS (80-99) ---
        {"Organ", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.5f}, {"vco2Waveform", 2.0f}, {"vco2Shape", 0.5f}, {"vco2Octave", 1.0f}, {"ampSustain", 1.0f}}},
        {"Classic Poly", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 5.0f}, {"ampDecay", 0.55f}, {"ampSustain", 0.6f}, {"lpfCutoff", 0.55f}}},
        {"Rhodesy", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 2.0f}, {"vco2Shape", 0.1f}, {"vco2Octave", 1.0f}, {"ampDecay", 0.65f}, {"lpfCutoff", 0.35f}}},
        {"Wurly", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.15f}, {"vco2Waveform", 1.0f}, {"ampDecay", 0.6f}, {"lpfCutoff", 0.3f}}},
        {"EPiano", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", 1.0f}, {"vco2Pitch", 4.0f}, {"ampDecay", 0.68f}, {"lpfCutoff", 0.45f}}},
        {"Jazz Organ", {{"vco1Waveform", 0.0f}, {"vco2Waveform", 0.0f}, {"vco2Octave", 2.0f}, {"mixLevel1", 0.8f}, {"mixLevel2", 0.6f}, {"ampSustain", 1.0f}}},
        {"Church", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 2.0f}, {"vco2Octave", 1.0f}, {"voiceMode", 1.0f}, {"ampAttack", 0.1f}, {"ampSustain", 1.0f}}},
        {"Digital Key", {{"vco1Waveform", 2.0f}, {"multiMode", 3.0f}, {"multiLevel", 0.4f}, {"delayMix", 0.35f}}},
        {"Bright Keys", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 12.0f}, {"lpfCutoff", 0.7f}, {"ampDecay", 0.55f}}},
        {"Mellow Keys", {{"vco1Waveform", 0.0f}, {"lpfCutoff", 0.25f}, {"ampAttack", 0.05f}, {"ampSustain", 0.75f}}},
        {"Synth Brass", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 7.0f}, {"ampAttack", 0.12f}, {"ampSustain", 0.8f}, {"lpfCutoff", 0.35f}, {"modTarget", 2.0f}, {"modIntensity", 0.7f}, {"modAttack", 0.25f}}},
        {"Brass Swell", {{"vco1Waveform", 1.0f}, {"modTarget", 2.0f}, {"modIntensity", 0.8f}, {"modAttack", 0.45f}, {"modDecay", 0.6f}}},
        {"Detuned Keys", {{"vco1Waveform", 1.0f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.35f}, {"ampSustain", 0.7f}}},
        {"Pulse Keys", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.35f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.75f}, {"lfoRate", 0.5f}}},
        {"Analog Keys", {{"vco1Waveform", 1.0f}, {"vco2Pitch", 6.0f}, {"lpfCutoff", 0.45f}, {"ampDecay", 0.65f}}},
        {"Vibe", {{"vco1Waveform", 0.0f}, {"vco1Octave", 1.0f}, {"lpfCutoff", 0.25f}, {"ampDecay", 0.6f}}},
        {"Gospel", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Octave", -1.0f}, {"ampSustain", 1.0f}, {"mixLevel2", 0.45f}}},
        {"Funky", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.65f}, {"lpfResonance", 0.55f}, {"lpfCutoff", 0.55f}}},
        {"Harpsi", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.85f}, {"ampSustain", 0.0f}, {"ampDecay", 0.45f}}},
        {"Boutique", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 1200.0f}, {"lpfCutoff", 0.45f}}},

        // --- FX / TEXTURE (100-119) ---
        {"Wind", {{"multiMode", 3.0f}, {"multiLevel", 0.8f}, {"multiShape", 0.5f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.85f}, {"lfoRate", 0.1f}, {"lpfCutoff", 0.2f}, {"ampSustain", 1.0f}}},
        {"Rain", {{"multiMode", 3.0f}, {"multiLevel", 0.6f}, {"multiShape", 0.95f}, {"hpfCutoff", 0.65f}, {"delayMix", 0.65f}}},
        {"Static", {{"multiMode", 3.0f}, {"multiLevel", 0.9f}, {"multiShape", 0.1f}, {"hpfCutoff", 0.45f}}},
        {"Engine", {{"vco1Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", -2400.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.75f}, {"lfoRate", 0.35f}}},
        {"Aliens", {{"vco1Waveform", 0.0f}, {"vco2Sync", 1.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.95f}, {"lfoRate", 0.65f}, {"delayMix", 0.75f}}},
        {"Siren", {{"vco1Waveform", 1.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.9f}, {"lfoRate", 0.45f}, {"lfoWaveform", 1.0f}}},
        {"Laser", {{"vco1Waveform", 1.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.1f}, {"modDecay", 0.35f}, {"ampSustain", 0.0f}}},
        {"UFO", {{"vco1Waveform", 0.0f}, {"vco2Sync", 1.0f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.85f}, {"lfoRate", 0.4f}, {"delayMix", 0.8f}}},
        {"Rumble", {{"vco1Waveform", 1.0f}, {"vco1Octave", -1.0f}, {"lpfCutoff", 0.08f}, {"voiceMode", 1.0f}, {"voiceDepth", 0.5f}}},
        {"Birds", {{"vco1Waveform", 0.0f}, {"vco1Octave", 2.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.9f}, {"lfoRate", 0.8f}, {"lfoWaveform", 2.0f}}},
        {"Computer", {{"vco1Waveform", 2.0f}, {"vco2Sync", 1.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.95f}, {"lfoRate", 0.75f}, {"lfoWaveform", 2.0f}}},
        {"Drone", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 3.0f}, {"ampAttack", 0.85f}, {"ampSustain", 1.0f}, {"lpfCutoff", 0.15f}}},
        {"Sweep FX", {{"multiMode", 3.0f}, {"multiLevel", 0.5f}, {"lpfCutoff", 0.05f}, {"modTarget", 2.0f}, {"modIntensity", 1.0f}, {"modAttack", 0.85f}}},
        {"Glitch", {{"multiMode", 3.0f}, {"multiLevel", 0.8f}, {"lfoTarget", 2.0f}, {"lfoIntensity", 0.95f}, {"lfoRate", 0.85f}}},
        {"Ocean", {{"multiMode", 3.0f}, {"multiLevel", 0.6f}, {"lfoTarget", 1.0f}, {"lfoIntensity", 0.75f}, {"lfoRate", 0.05f}, {"lpfCutoff", 0.15f}}},
        {"Alarm", {{"vco1Waveform", 2.0f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.85f}, {"lfoRate", 0.65f}, {"lfoWaveform", 1.0f}}},
        {"Robotic", {{"vco1Waveform", 2.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 1200.0f}, {"lpfResonance", 0.65f}, {"lfoTarget", 2.0f}}},
        {"Falling", {{"vco1Waveform", 1.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.15f}, {"modDecay", 0.65f}, {"ampSustain", 0.0f}}},
        {"Rising", {{"vco1Waveform", 1.0f}, {"modTarget", 0.0f}, {"modIntensity", 0.85f}, {"modDecay", 0.65f}, {"ampSustain", 0.0f}}},
        {"Metallic", {{"vco1Waveform", 1.0f}, {"vco2Sync", 1.0f}, {"vco2Pitch", 1400.0f}, {"lpfCutoff", 0.55f}}},

        // --- OTHERS (120-126) ---
        {"Accordion", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 3.0f}, {"ampAttack", 0.1f}, {"lpfCutoff", 0.65f}}},
        {"Harmonica", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.45f}, {"lpfCutoff", 0.45f}, {"ampAttack", 0.08f}}},
        {"Whistle 2", {{"vco1Waveform", 0.0f}, {"vco1Octave", 1.0f}, {"portamento", 0.35f}, {"lpfCutoff", 0.85f}}},
        {"Bagpipes", {{"vco1Waveform", 1.0f}, {"vco2Waveform", 1.0f}, {"vco2Pitch", 1.5f}, {"voiceMode", 1.0f}, {"ampAttack", 0.15f}}},
        {"Flute", {{"vco1Waveform", 0.0f}, {"ampAttack", 0.12f}, {"delayMix", 0.25f}, {"lfoTarget", 0.0f}, {"lfoIntensity", 0.55f}}},
        {"Oboe", {{"vco1Waveform", 1.0f}, {"vco1Shape", 0.75f}, {"lpfCutoff", 0.35f}, {"ampAttack", 0.1f}}},
        {"End of Line", {{"vco1Waveform", 2.0f}, {"vco1Shape", 0.95f}, {"lpfCutoff", 0.1f}, {"delayMix", 0.85f}, {"delayFeedback", 0.85f}}}
    };

    return presetList;
}

SynthPreset SynthPresets::initPreset()
{
    return { "Init", {} };
}

} // namespace noteahead
