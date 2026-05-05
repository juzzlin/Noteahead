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
        {"Fat Bass", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.8}, {"lpfCutoff", 0.3}, {"lpfResonance", 0.4}, {"ampAttack", 0.00}, {"ampDecay", 0.42}, {"ampSustain", 0.5}, {"ampRelease", 0.48}, {"voiceMode", 1}, {"voiceDepth", 0.2}, {"lfoMode", 2}, {"lfoTarget", 2}, {"lfoIntensity", 0.4}, {"lfoRate", 0.6}}},
        {"Sub Bass", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", -1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.5}, {"lpfCutoff", 0.2}, {"ampDecay", 0.45}, {"ampSustain", 0.4}}},
        {"Acid Line", {{"vco1Waveform", 1}, {"lpfCutoff", 0.2}, {"lpfResonance", 0.8}, {"modTarget", 2}, {"modIntensity", 0.7}, {"modAttack", 0.00}, {"modDecay", 0.39}, {"lfoTarget", 2}, {"lfoIntensity", 0.2}, {"lfoRate", 0.8}}},
        {"Reese Bass", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 2}, {"mixLevel1", 0.8}, {"mixLevel2", 0.8}, {"lpfCutoff", 0.2}, {"voiceMode", 1}, {"voiceDepth", 0.5}}},
        {"Slap Bass", {{"vco1Waveform", 2}, {"mixLevel1", 1.0}, {"lpfCutoff", 0.4}, {"modTarget", 2}, {"modIntensity", 0.5}, {"modAttack", 0.00}, {"modDecay", 0.26}, {"ampDecay", 0.34}}},
        {"Analog Moog", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Octave", -1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.6}, {"lpfCutoff", 0.3}, {"ampAttack", 0.55}}},
        {"Dark FM Bass", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 1}, {"mixLevel1", 0.8}, {"vco2Sync", 1}, {"lpfCutoff", 0.2}}},
        {"Driving Bass", {{"vco1Waveform", 1}, {"vco2Waveform", 2}, {"vco2Pitch", -2}, {"mixLevel1", 0.9}, {"mixLevel2", 0.7}, {"lpfCutoff", 0.3}, {"ampDecay", 0.45}}},
        {"Clicky Sub", {{"vco1Waveform", 0}, {"mixLevel1", 1.0}, {"lpfCutoff", 0.15}, {"ampAttack", 0.00}, {"ampDecay", 0.39}, {"modTarget", 2}, {"modIntensity", 0.8}, {"modDecay", 0.19}}},
        {"Pulse Bass", {{"vco1Waveform", 2}, {"vco1Shape", 0.4}, {"lpfCutoff", 0.3}, {"ampDecay", 0.42}}},
        {"Dirt Bass", {{"vco1Waveform", 1}, {"vco1Shape", 0.8}, {"mixLevel1", 1.0}, {"lpfCutoff", 0.25}, {"lpfResonance", 0.3}}},
        {"Organ Bass", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", 1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.4}}},
        {"Wide Sub", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 1}, {"mixLevel1", 0.8}, {"mixLevel2", 0.8}, {"panSpread", 0.8}}},
        {"Grit Bass", {{"vco1Waveform", 1}, {"vco1Shape", 0.5}, {"vco2Sync", 1}, {"mixLevel1", 0.7}, {"mixLevel2", 0.5}}},
        {"Techno Bass", {{"vco1Waveform", 1}, {"lpfCutoff", 0.2}, {"ampDecay", 0.39}, {"delayMix", 0.2}}},
        {"Soft Moog", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"mixLevel1", 0.6}, {"mixLevel2", 0.4}, {"lpfCutoff", 0.2}}},
        {"Deep Sine", {{"vco1Waveform", 0}, {"lpfCutoff", 0.1}, {"masterVolume", 1.0}}},
        {"Mod Bass", {{"vco1Waveform", 1}, {"modTarget", 1}, {"modIntensity", 0.4}, {"modDecay", 0.47}}},
        {"Reso Bass", {{"vco1Waveform", 1}, {"lpfResonance", 0.9}, {"lpfCutoff", 0.1}, {"modTarget", 2}, {"modIntensity", 0.8}}},

        // --- LEADS (20-39) ---
        {"Power Lead", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 2}, {"mixLevel1", 1.0}, {"mixLevel2", 1.0}, {"lpfCutoff", 0.8}, {"voiceMode", 1}, {"voiceDepth", 0.3}}},
        {"Sync Lead", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Sync", 1}, {"mixLevel1", 0.5}, {"mixLevel2", 1.0}, {"modTarget", 1}, {"modIntensity", 0.8}, {"modAttack", 0.73}, {"modDecay", 0.42}}},
        {"Square Lead", {{"vco1Waveform", 2}, {"vco1Shape", 0.1}, {"ampAttack", 0.64}, {"ampSustain", 0.8}}},
        {"Tri Lead", {{"vco1Waveform", 0}, {"portamento", 0.2}, {"delayMix", 0.3}}},
        {"Folding Lead", {{"vco1Waveform", 1}, {"vco1Shape", 0.7}, {"lpfCutoff", 0.6}, {"ampAttack", 0.59}}},
        {"Soft Lead", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 5}, {"mixLevel1", 0.8}, {"mixLevel2", 0.4}, {"lpfCutoff", 0.4}, {"ampAttack", 0.68}}},
        {"Screamer", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Sync", 1}, {"vco2Pitch", 24}, {"lpfResonance", 0.7}}},
        {"Detuned Saw", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 15}, {"mixLevel1", 1.0}, {"mixLevel2", 1.0}, {"voiceMode", 1}, {"voiceDepth", 0.6}}},
        {"Wobble Lead", {{"vco1Waveform", 1}, {"modTarget", 2}, {"modIntensity", 0.5}, {"modAttack", 0.73}, {"modDecay", 0.45}, {"lfoMode", 1}, {"lfoTarget", 2}, {"lfoIntensity", 0.6}, {"lfoRate", 0.2}}},
        {"Fifth Lead", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 700}, {"mixLevel1", 0.8}, {"mixLevel2", 0.6}}},
        {"Plastic Lead", {{"vco1Waveform", 2}, {"vco1Shape", 0.8}, {"lpfCutoff", 0.5}, {"ampDecay", 0.42}}},
        {"Whistle", {{"vco1Waveform", 0}, {"lpfCutoff", 0.9}, {"lpfResonance", 0.9}, {"portamento", 0.4}}},
        {"Hero Lead", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 2}, {"mixLevel1", 1.0}, {"mixLevel2", 0.8}, {"delayMix", 0.4}, {"delayFeedback", 0.5}}},
        {"Buzzy Lead", {{"vco1Waveform", 2}, {"vco1Shape", 0.9}, {"vco2Waveform", 1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.5}}},
        {"Ghostly", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 1200}, {"mixLevel1", 0.4}, {"mixLevel2", 0.3}, {"delayMix", 0.6}}},
        {"Gritty Mono", {{"vco1Waveform", 1}, {"vco1Shape", 0.6}, {"voiceMode", 1}, {"portamento", 0.1}}},
        {"Classic 80s", {{"vco1Waveform", 1}, {"vco2Waveform", 2}, {"vco2Shape", 0.2}, {"lpfCutoff", 0.5}, {"ampAttack", 0.64}}},
        {"Saw Swell", {{"vco1Waveform", 1}, {"modTarget", 2}, {"modIntensity", 0.8}, {"modAttack", 0.79}, {"modDecay", 0.50}}},
        {"Space Lead", {{"vco1Waveform", 0}, {"vco2Waveform", 1}, {"vco2Sync", 1}, {"delayType", 2}, {"delayMix", 0.5}}},
        {"Aggro Sync", {{"vco1Waveform", 1}, {"vco2Waveform", 2}, {"vco2Sync", 1}, {"modIntensity", 1.0}, {"modDecay", 0.34}}},

        // --- PADS (40-59) ---
        {"Soft Pad", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"mixLevel1", 0.8}, {"mixLevel2", 0.8}, {"vco2Pitch", 10}, {"lpfCutoff", 0.4}, {"ampAttack", 0.79}, {"ampDecay", 0.45}, {"ampSustain", 0.8}, {"ampRelease", 0.58}, {"panSpread", 0.5}, {"lfoTarget", 1}, {"lfoIntensity", 0.3}, {"lfoRate", 0.1}}},
        {"Deep Sea", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", -1}, {"lpfCutoff", 0.2}, {"ampAttack", 0.81}, {"ampRelease", 0.63}, {"delayMix", 0.4}}},
        {"Saw Pad", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 5}, {"mixLevel1", 1.0}, {"mixLevel2", 0.9}, {"lpfCutoff", 0.3}, {"ampAttack", 0.77}, {"ampSustain", 0.7}}},
        {"Strings", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 12}, {"mixLevel1", 0.8}, {"mixLevel2", 0.8}, {"lpfCutoff", 0.4}, {"ampAttack", 0.77}, {"ampRelease", 0.56}, {"panSpread", 0.8}}},
        {"Organ Pad", {{"vco1Waveform", 2}, {"vco1Shape", 0.5}, {"vco2Waveform", 0}, {"vco2Octave", 1}, {"ampAttack", 0.75}, {"ampSustain", 1.0}}},
        {"Glassy", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 2}, {"lpfCutoff", 0.5}, {"ampAttack", 0.73}, {"delayMix", 0.5}}},
        {"Ethereal", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 12}, {"lpfCutoff", 0.3}, {"ampAttack", 0.82}, {"delayType", 5}, {"delayMix", 0.6}, {"lfoTarget", 2}, {"lfoIntensity", 0.4}, {"lfoRate", 0.05}}},
        {"Warmth", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 2}, {"lpfCutoff", 0.2}, {"ampAttack", 0.78}, {"ampSustain", 0.9}}},
        {"Pulsing", {{"vco1Waveform", 2}, {"vco1Shape", 0.5}, {"modTarget", 2}, {"modIntensity", 0.4}, {"modAttack", 0.78}, {"modDecay", 0.45}}},
        {"Cloudy", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"voiceMode", 1}, {"voiceDepth", 0.8}, {"lpfCutoff", 0.2}, {"ampAttack", 0.80}}},
        {"Vocaloid", {{"vco1Waveform", 1}, {"vco1Shape", 0.8}, {"lpfCutoff", 0.4}, {"lpfResonance", 0.7}, {"ampAttack", 0.75}}},
        {"Dreamy", {{"vco1Waveform", 0}, {"vco2Pitch", 7}, {"lpfCutoff", 0.3}, {"delayType", 2}, {"delayMix", 0.4}}},
        {"Ice Pad", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 2}, {"hpfCutoff", 0.4}, {"ampAttack", 0.78}}},
        {"Aurora", {{"vco1Waveform", 1}, {"vco2Pitch", 3}, {"lpfCutoff", 0.2}, {"delayType", 4}, {"delayMix", 0.5}}},
        {"Majestic", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Octave", -1}, {"voiceMode", 1}, {"ampAttack", 0.77}}},
        {"Airy", {{"vco1Waveform", 0}, {"hpfCutoff", 0.6}, {"ampAttack", 0.79}, {"delayMix", 0.3}}},
        {"Twilight", {{"vco1Waveform", 1}, {"vco2Waveform", 0}, {"lpfCutoff", 0.25}, {"ampAttack", 0.77}}},
        {"Retro Pad", {{"vco1Waveform", 1}, {"vco2Waveform", 2}, {"vco2Shape", 0.3}, {"lpfCutoff", 0.3}, {"ampAttack", 0.73}}},
        {"Darkness", {{"vco1Waveform", 1}, {"lpfCutoff", 0.15}, {"ampAttack", 0.81}, {"delayMix", 0.4}}},
        {"Heavenly", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 19}, {"lpfCutoff", 0.4}, {"ampAttack", 0.78}}},

        // --- PLUCKS (60-79) ---
        {"Bright Pluck", {{"vco1Waveform", 2}, {"mixLevel1", 1.0}, {"lpfCutoff", 0.2}, {"modTarget", 2}, {"modIntensity", 0.6}, {"modAttack", 0.00}, {"modDecay", 0.34}, {"ampAttack", 0.00}, {"ampDecay", 0.39}, {"ampSustain", 0.0}}},
        {"Bell", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 2}, {"mixLevel1", 0.5}, {"mixLevel2", 0.5}, {"ampAttack", 0.00}, {"ampDecay", 0.50}, {"ampSustain", 0.0}}},
        {"Short Saw", {{"vco1Waveform", 1}, {"lpfCutoff", 0.3}, {"ampDecay", 0.34}, {"ampSustain", 0.0}}},
        {"Digital Pluck", {{"vco1Waveform", 2}, {"vco1Shape", 0.8}, {"lpfCutoff", 0.4}, {"ampDecay", 0.31}}},
        {"Woody", {{"vco1Waveform", 0}, {"lpfCutoff", 0.2}, {"modIntensity", 0.5}, {"modDecay", 0.26}, {"ampDecay", 0.34}}},
        {"Crystal", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 2}, {"delayMix", 0.4}, {"ampDecay", 0.45}}},
        {"Perc Bass", {{"vco1Waveform", 1}, {"lpfCutoff", 0.2}, {"ampDecay", 0.26}, {"modIntensity", 0.6}, {"modDecay", 0.19}}},
        {"Stab", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 2}, {"lpfCutoff", 0.5}, {"ampDecay", 0.39}}},
        {"Mallet", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 1}, {"ampDecay", 0.42}, {"lpfCutoff", 0.3}}},
        {"Tines", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 3}, {"mixLevel2", 0.3}, {"ampDecay", 0.50}}},
        {"Koto", {{"vco1Waveform", 1}, {"vco1Shape", 0.9}, {"lpfCutoff", 0.4}, {"ampDecay", 0.34}}},
        {"Harp", {{"vco1Waveform", 0}, {"vco2Pitch", 12}, {"ampDecay", 0.47}, {"delayMix", 0.3}}},
        {"Chirp", {{"vco1Waveform", 2}, {"modTarget", 0}, {"modIntensity", 0.8}, {"modDecay", 0.19}, {"ampDecay", 0.26}}},
        {"Bamboo", {{"vco1Waveform", 0}, {"vco2Waveform", 1}, {"vco2Pitch", 19}, {"ampDecay", 0.34}}},
        {"Marimba", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", 1}, {"lpfCutoff", 0.3}, {"ampDecay", 0.42}}},
        {"Clav", {{"vco1Waveform", 2}, {"vco1Shape", 0.2}, {"lpfCutoff", 0.4}, {"ampDecay", 0.37}}},
        {"Plastic Pluck", {{"vco1Waveform", 2}, {"vco1Shape", 0.7}, {"lpfCutoff", 0.3}, {"ampDecay", 0.34}}},
        {"Square Stab", {{"vco1Waveform", 2}, {"lpfCutoff", 0.4}, {"ampDecay", 0.26}, {"modIntensity", 0.5}}},
        {"Glocken", {{"vco1Waveform", 0}, {"vco2Octave", 3}, {"ampDecay", 0.49}, {"delayMix", 0.4}}},
        {"Shorty", {{"vco1Waveform", 1}, {"lpfCutoff", 0.1}, {"ampDecay", 0.19}}},

        // --- KEYS (80-99) ---
        {"Organ", {{"vco1Waveform", 2}, {"vco2Waveform", 2}, {"vco2Octave", 1}, {"mixLevel1", 0.8}, {"mixLevel2", 0.6}, {"ampAttack", 0.00}, {"ampSustain", 1.0}}},
        {"Classic Poly", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Octave", 0}, {"vco2Pitch", 2}, {"mixLevel1", 1.0}, {"mixLevel2", 1.0}, {"lpfCutoff", 0.6}, {"ampAttack", 0.68}, {"ampDecay", 0.42}, {"ampSustain", 0.6}, {"ampRelease", 0.52}}},
        {"Rhodesy", {{"vco1Waveform", 0}, {"vco2Waveform", 2}, {"vco2Octave", 1}, {"mixLevel1", 1.0}, {"mixLevel2", 0.2}, {"lpfCutoff", 0.4}, {"ampDecay", 0.50}}},
        {"Wurly", {{"vco1Waveform", 2}, {"vco1Shape", 0.1}, {"vco2Waveform", 1}, {"lpfCutoff", 0.3}, {"ampDecay", 0.49}}},
        {"EPiano", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", 1}, {"vco2Pitch", 5}, {"lpfCutoff", 0.5}, {"ampDecay", 0.52}}},
        {"Jazz Organ", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", 2}, {"mixLevel1", 0.8}, {"mixLevel2", 0.5}}},
        {"Church", {{"vco1Waveform", 1}, {"vco2Waveform", 2}, {"vco2Octave", 1}, {"voiceMode", 1}, {"ampAttack", 0.68}}},
        {"Digital Key", {{"vco1Waveform", 2}, {"vco2Waveform", 2}, {"vco2Octave", 2}, {"lpfCutoff", 0.6}, {"delayMix", 0.3}}},
        {"Bright Keys", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 12}, {"lpfCutoff", 0.7}, {"ampDecay", 0.47}}},
        {"Mellow Keys", {{"vco1Waveform", 0}, {"lpfCutoff", 0.3}, {"ampAttack", 0.64}, {"ampSustain", 0.7}}},
        {"Synth Brass", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 5}, {"lpfCutoff", 0.4}, {"ampAttack", 0.68}, {"ampSustain", 0.8}}},
        {"Brass Swell", {{"vco1Waveform", 1}, {"modTarget", 2}, {"modIntensity", 0.6}, {"modAttack", 0.73}, {"modDecay", 0.45}}},
        {"Detuned Keys", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 10}, {"voiceMode", 1}}},
        {"Pulse Keys", {{"vco1Waveform", 2}, {"vco1Shape", 0.3}, {"lpfCutoff", 0.5}, {"lfoMode", 1}, {"lfoTarget", 1}, {"lfoIntensity", 0.5}, {"lfoRate", 0.4}}},
        {"Analog Keys", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 3}, {"lpfCutoff", 0.4}}},
        {"Vibe", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Octave", 1}, {"lpfCutoff", 0.2}, {"ampDecay", 0.45}}},
        {"Gospel", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Octave", -1}, {"mixLevel2", 0.4}}},
        {"Funky", {{"vco1Waveform", 2}, {"vco1Shape", 0.6}, {"lpfCutoff", 0.6}, {"lpfResonance", 0.5}}},
        {"Harpsi", {{"vco1Waveform", 2}, {"vco1Shape", 0.8}, {"ampDecay", 0.39}}},
        {"Boutique", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 1200}, {"lpfCutoff", 0.4}}},

        // --- FX / TEXTURE (100-119) ---
        {"Wind", {{"vco1Waveform", 2}, {"vco1Shape", 0.5}, {"lpfCutoff", 0.2}, {"lpfResonance", 0.9}, {"ampAttack", 0.82}}},
        {"Rain", {{"vco1Waveform", 2}, {"vco1Shape", 0.9}, {"hpfCutoff", 0.7}, {"delayMix", 0.6}}},
        {"Static", {{"vco1Waveform", 2}, {"vco1Shape", 0.95}, {"hpfCutoff", 0.5}}},
        {"Engine", {{"vco1Waveform", 1}, {"vco1Shape", 0.5}, {"vco2Sync", 1}, {"vco2Pitch", -24}}},
        {"Aliens", {{"vco1Waveform", 0}, {"vco2Waveform", 1}, {"vco2Sync", 1}, {"modTarget", 1}, {"modIntensity", 0.9}, {"modDecay", 0.53}}},
        {"Siren", {{"vco1Waveform", 1}, {"modTarget", 0}, {"modIntensity", 1.0}, {"modAttack", 0.78}, {"modDecay", 0.45}}},
        {"Laser", {{"vco1Waveform", 1}, {"modTarget", 0}, {"modIntensity", 1.0}, {"modDecay", 0.26}}},
        {"UFO", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Sync", 1}, {"delayMix", 0.8}}},
        {"Rumble", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 1}, {"lpfCutoff", 0.1}, {"voiceMode", 1}}},
        {"Birds", {{"vco1Waveform", 0}, {"vco2Waveform", 0}, {"vco2Pitch", 36}, {"modIntensity", 0.8}, {"delayMix", 0.4}}},
        {"Computer", {{"vco1Waveform", 2}, {"vco2Sync", 1}, {"modIntensity", 0.9}, {"delayMix", 0.6}}},
        {"Drone", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 1}, {"ampAttack", 0.82}, {"ampSustain", 1.0}}},
        {"Sweep FX", {{"vco1Waveform", 1}, {"lpfCutoff", 0.1}, {"modTarget", 2}, {"modIntensity", 1.0}, {"modAttack", 0.81}}},
        {"Glitch", {{"vco1Waveform", 2}, {"vco2Sync", 1}, {"vco2Pitch", 48}, {"modDecay", 0.19}}},
        {"Ocean", {{"vco1Waveform", 2}, {"lpfCutoff", 0.1}, {"ampAttack", 0.82}, {"ampRelease", 0.63}}},
        {"Alarm", {{"vco1Waveform", 2}, {"vco2Waveform", 2}, {"vco2Pitch", 7}, {"modIntensity", 0.7}}},
        {"Robotic", {{"vco1Waveform", 2}, {"vco1Shape", 0.4}, {"vco2Sync", 1}, {"lpfResonance", 0.6}}},
        {"Falling", {{"vco1Waveform", 1}, {"modTarget", 0}, {"modIntensity", -0.8}, {"modAttack", 0.00}, {"modDecay", 0.53}}},
        {"Rising", {{"vco1Waveform", 1}, {"modTarget", 0}, {"modIntensity", 0.8}, {"modAttack", 0.82}}},
        {"Metallic", {{"vco1Waveform", 1}, {"vco2Sync", 1}, {"vco2Pitch", 14}, {"lpfCutoff", 0.5}}},

        // --- OTHERS (120-126) ---
        {"Accordion", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 2}, {"ampAttack", 0.68}, {"lpfCutoff", 0.6}}},
        {"Harmonica", {{"vco1Waveform", 2}, {"vco1Shape", 0.4}, {"lpfCutoff", 0.4}, {"ampAttack", 0.64}}},
        {"Whistle 2", {{"vco1Waveform", 0}, {"portamento", 0.3}, {"lpfCutoff", 0.8}}},
        {"Bagpipes", {{"vco1Waveform", 1}, {"vco2Waveform", 1}, {"vco2Pitch", 1}, {"voiceMode", 1}, {"ampAttack", 0.73}}},
        {"Flute", {{"vco1Waveform", 0}, {"ampAttack", 0.68}, {"delayMix", 0.2}}},
        {"Oboe", {{"vco1Waveform", 1}, {"vco1Shape", 0.7}, {"lpfCutoff", 0.3}, {"ampAttack", 0.68}}},
        {"End of Line", {{"vco1Waveform", 2}, {"vco1Shape", 0.95}, {"lpfCutoff", 0.1}, {"delayMix", 0.8}, {"delayFeedback", 0.8}}}
    };

    return presetList;
}

SynthPreset SynthPresets::initPreset()
{
    return { "Init", {} };
}

} // namespace noteahead
