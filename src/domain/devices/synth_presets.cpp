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
        {"Fat Bass", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.8}, {"lpfcutoff", 0.3}, {"lpfresonance", 0.4}, {"ampattack", 0.0}, {"ampdecay", 0.4}, {"ampsustain", 0.5}, {"amprelease", 0.2}, {"voiceMode", 1}, {"voiceDepth", 0.2}, {"lfomode", 2}, {"lfotarget", 2}, {"lfointensity", 0.4}, {"lforate", 0.6}}},
        {"Sub Bass", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", -1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.5}, {"lpfcutoff", 0.2}, {"ampdecay", 0.5}, {"ampsustain", 0.4}}},
        {"Acid Line", {{"vco1waveform", 1}, {"lpfcutoff", 0.2}, {"lpfresonance", 0.8}, {"modtarget", 2}, {"modintensity", 0.7}, {"modattack", 0.0}, {"moddecay", 0.3}, {"lfotarget", 2}, {"lfointensity", 0.2}, {"lforate", 0.8}}},
        {"Reese Bass", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 2}, {"mixlevel1", 0.8}, {"mixlevel2", 0.8}, {"lpfcutoff", 0.2}, {"voiceMode", 1}, {"voiceDepth", 0.5}}},
        {"Slap Bass", {{"vco1waveform", 2}, {"mixlevel1", 1.0}, {"lpfcutoff", 0.4}, {"modtarget", 2}, {"modintensity", 0.5}, {"modattack", 0.0}, {"moddecay", 0.1}, {"ampdecay", 0.2}}},
        {"Analog Moog", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2octave", -1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.6}, {"lpfcutoff", 0.3}, {"ampattack", 0.01}}},
        {"Dark FM Bass", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 1}, {"mixlevel1", 0.8}, {"vco2sync", 1}, {"lpfcutoff", 0.2}}},
        {"Driving Bass", {{"vco1waveform", 1}, {"vco2waveform", 2}, {"vco2pitch", -2}, {"mixlevel1", 0.9}, {"mixlevel2", 0.7}, {"lpfcutoff", 0.3}, {"ampdecay", 0.5}}},
        {"Clicky Sub", {{"vco1waveform", 0}, {"mixlevel1", 1.0}, {"lpfcutoff", 0.15}, {"ampattack", 0.0}, {"ampdecay", 0.3}, {"modtarget", 2}, {"modintensity", 0.8}, {"moddecay", 0.05}}},
        {"Pulse Bass", {{"vco1waveform", 2}, {"vco1shape", 0.4}, {"lpfcutoff", 0.3}, {"ampdecay", 0.4}}},
        {"Dirt Bass", {{"vco1waveform", 1}, {"vco1shape", 0.8}, {"mixlevel1", 1.0}, {"lpfcutoff", 0.25}, {"lpfresonance", 0.3}}},
        {"Organ Bass", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", 1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.4}}},
        {"Wide Sub", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 1}, {"mixlevel1", 0.8}, {"mixlevel2", 0.8}, {"panspread", 0.8}}},
        {"Grit Bass", {{"vco1waveform", 1}, {"vco1shape", 0.5}, {"vco2sync", 1}, {"mixlevel1", 0.7}, {"mixlevel2", 0.5}}},
        {"Techno Bass", {{"vco1waveform", 1}, {"lpfcutoff", 0.2}, {"ampdecay", 0.3}, {"delaymix", 0.2}}},
        {"Soft Moog", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"mixlevel1", 0.6}, {"mixlevel2", 0.4}, {"lpfcutoff", 0.2}}},
        {"Deep Sine", {{"vco1waveform", 0}, {"lpfcutoff", 0.1}, {"mastervolume", 1.0}}},
        {"Mod Bass", {{"vco1waveform", 1}, {"modtarget", 1}, {"modintensity", 0.4}, {"moddecay", 0.6}}},
        {"Reso Bass", {{"vco1waveform", 1}, {"lpfresonance", 0.9}, {"lpfcutoff", 0.1}, {"modtarget", 2}, {"modintensity", 0.8}}},

        // --- LEADS (20-39) ---
        {"Power Lead", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 2}, {"mixlevel1", 1.0}, {"mixlevel2", 1.0}, {"lpfcutoff", 0.8}, {"voicemode", 1}, {"voiceDepth", 0.3}}},
        {"Sync Lead", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2sync", 1}, {"mixlevel1", 0.5}, {"mixlevel2", 1.0}, {"modtarget", 1}, {"modintensity", 0.8}, {"modattack", 0.2}, {"moddecay", 0.4}}},
        {"Square Lead", {{"vco1waveform", 2}, {"vco1shape", 0.1}, {"ampattack", 0.05}, {"ampsustain", 0.8}}},
        {"Tri Lead", {{"vco1waveform", 0}, {"portamento", 0.2}, {"delaymix", 0.3}}},
        {"Folding Lead", {{"vco1waveform", 1}, {"vco1shape", 0.7}, {"lpfcutoff", 0.6}, {"ampattack", 0.02}}},
        {"Soft Lead", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 5}, {"mixlevel1", 0.8}, {"mixlevel2", 0.4}, {"lpfcutoff", 0.4}, {"ampattack", 0.1}}},
        {"Screamer", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2sync", 1}, {"vco2pitch", 24}, {"lpfresonance", 0.7}}},
        {"Detuned Saw", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 15}, {"mixlevel1", 1.0}, {"mixlevel2", 1.0}, {"voicemode", 1}, {"voiceDepth", 0.6}}},
        {"Wobble Lead", {{"vco1waveform", 1}, {"modtarget", 2}, {"modintensity", 0.5}, {"modattack", 0.2}, {"moddecay", 0.5}, {"lfomode", 1}, {"lfotarget", 2}, {"lfointensity", 0.6}, {"lforate", 0.2}}},
        {"Fifth Lead", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 700}, {"mixlevel1", 0.8}, {"mixlevel2", 0.6}}},
        {"Plastic Lead", {{"vco1waveform", 2}, {"vco1shape", 0.8}, {"lpfcutoff", 0.5}, {"ampdecay", 0.4}}},
        {"Whistle", {{"vco1waveform", 0}, {"lpfcutoff", 0.9}, {"lpfresonance", 0.9}, {"portamento", 0.4}}},
        {"Hero Lead", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 2}, {"mixlevel1", 1.0}, {"mixlevel2", 0.8}, {"delaymix", 0.4}, {"delayfeedback", 0.5}}},
        {"Buzzy Lead", {{"vco1waveform", 2}, {"vco1shape", 0.9}, {"vco2waveform", 1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.5}}},
        {"Ghostly", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 1200}, {"mixlevel1", 0.4}, {"mixlevel2", 0.3}, {"delaymix", 0.6}}},
        {"Gritty Mono", {{"vco1waveform", 1}, {"vco1shape", 0.6}, {"voicemode", 1}, {"portamento", 0.1}}},
        {"Classic 80s", {{"vco1waveform", 1}, {"vco2waveform", 2}, {"vco2shape", 0.2}, {"lpfcutoff", 0.5}, {"ampattack", 0.05}}},
        {"Saw Swell", {{"vco1waveform", 1}, {"modtarget", 2}, {"modintensity", 0.8}, {"modattack", 0.6}, {"moddecay", 0.8}}},
        {"Space Lead", {{"vco1waveform", 0}, {"vco2waveform", 1}, {"vco2sync", 1}, {"delaytype", 2}, {"delaymix", 0.5}}},
        {"Aggro Sync", {{"vco1waveform", 1}, {"vco2waveform", 2}, {"vco2sync", 1}, {"modintensity", 1.0}, {"moddecay", 0.2}}},

        // --- PADS (40-59) ---
        {"Soft Pad", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"mixlevel1", 0.8}, {"mixlevel2", 0.8}, {"vco2pitch", 10}, {"lpfcutoff", 0.4}, {"ampattack", 0.6}, {"ampdecay", 0.5}, {"ampsustain", 0.8}, {"amprelease", 0.6}, {"panSpread", 0.5}, {"lfotarget", 1}, {"lfointensity", 0.3}, {"lforate", 0.1}}},
        {"Deep Sea", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", -1}, {"lpfcutoff", 0.2}, {"ampattack", 0.8}, {"amprelease", 1.0}, {"delaymix", 0.4}}},
        {"Saw Pad", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 5}, {"mixlevel1", 1.0}, {"mixlevel2", 0.9}, {"lpfcutoff", 0.3}, {"ampattack", 0.4}, {"ampsustain", 0.7}}},
        {"Strings", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 12}, {"mixlevel1", 0.8}, {"mixlevel2", 0.8}, {"lpfcutoff", 0.4}, {"ampattack", 0.4}, {"amprelease", 0.5}, {"panspread", 0.8}}},
        {"Organ Pad", {{"vco1waveform", 2}, {"vco1shape", 0.5}, {"vco2waveform", 0}, {"vco2octave", 1}, {"ampattack", 0.3}, {"ampsustain", 1.0}}},
        {"Glassy", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 2}, {"lpfcutoff", 0.5}, {"ampattack", 0.2}, {"delaymix", 0.5}}},
        {"Ethereal", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 12}, {"lpfcutoff", 0.3}, {"ampattack", 0.9}, {"delaytype", 5}, {"delaymix", 0.6}, {"lfotarget", 2}, {"lfointensity", 0.4}, {"lforate", 0.05}}},
        {"Warmth", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 2}, {"lpfcutoff", 0.2}, {"ampattack", 0.5}, {"ampsustain", 0.9}}},
        {"Pulsing", {{"vco1waveform", 2}, {"vco1shape", 0.5}, {"modtarget", 2}, {"modintensity", 0.4}, {"modattack", 0.5}, {"moddecay", 0.5}}},
        {"Cloudy", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"voicemode", 1}, {"voiceDepth", 0.8}, {"lpfcutoff", 0.2}, {"ampattack", 0.7}}},
        {"Vocaloid", {{"vco1waveform", 1}, {"vco1shape", 0.8}, {"lpfcutoff", 0.4}, {"lpfresonance", 0.7}, {"ampattack", 0.3}}},
        {"Dreamy", {{"vco1waveform", 0}, {"vco2pitch", 7}, {"lpfcutoff", 0.3}, {"delaytype", 2}, {"delaymix", 0.4}}},
        {"Ice Pad", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 2}, {"hpfcutoff", 0.4}, {"ampattack", 0.5}}},
        {"Aurora", {{"vco1waveform", 1}, {"vco2pitch", 3}, {"lpfcutoff", 0.2}, {"delaytype", 4}, {"delaymix", 0.5}}},
        {"Majestic", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2octave", -1}, {"voicemode", 1}, {"ampattack", 0.4}}},
        {"Airy", {{"vco1waveform", 0}, {"hpfcutoff", 0.6}, {"ampattack", 0.6}, {"delaymix", 0.3}}},
        {"Twilight", {{"vco1waveform", 1}, {"vco2waveform", 0}, {"lpfcutoff", 0.25}, {"ampattack", 0.4}}},
        {"Retro Pad", {{"vco1waveform", 1}, {"vco2waveform", 2}, {"vco2shape", 0.3}, {"lpfcutoff", 0.3}, {"ampattack", 0.2}}},
        {"Darkness", {{"vco1waveform", 1}, {"lpfcutoff", 0.15}, {"ampattack", 0.8}, {"delaymix", 0.4}}},
        {"Heavenly", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 19}, {"lpfcutoff", 0.4}, {"ampattack", 0.5}}},

        // --- PLUCKS (60-79) ---
        {"Bright Pluck", {{"vco1waveform", 2}, {"mixlevel1", 1.0}, {"lpfcutoff", 0.2}, {"modtarget", 2}, {"modintensity", 0.6}, {"modattack", 0.0}, {"moddecay", 0.2}, {"ampattack", 0.0}, {"ampdecay", 0.3}, {"ampsustain", 0.0}}},
        {"Bell", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 2}, {"mixlevel1", 0.5}, {"mixlevel2", 0.5}, {"ampattack", 0.0}, {"ampdecay", 0.8}, {"ampsustain", 0.0}}},
        {"Short Saw", {{"vco1waveform", 1}, {"lpfcutoff", 0.3}, {"ampdecay", 0.2}, {"ampsustain", 0.0}}},
        {"Digital Pluck", {{"vco1waveform", 2}, {"vco1shape", 0.8}, {"lpfcutoff", 0.4}, {"ampdecay", 0.15}}},
        {"Woody", {{"vco1waveform", 0}, {"lpfcutoff", 0.2}, {"modintensity", 0.5}, {"moddecay", 0.1}, {"ampdecay", 0.2}}},
        {"Crystal", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 2}, {"delaymix", 0.4}, {"ampdecay", 0.5}}},
        {"Perc Bass", {{"vco1waveform", 1}, {"lpfcutoff", 0.2}, {"ampdecay", 0.1}, {"modintensity", 0.6}, {"moddecay", 0.05}}},
        {"Stab", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 2}, {"lpfcutoff", 0.5}, {"ampdecay", 0.3}}},
        {"Mallet", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 1}, {"ampdecay", 0.4}, {"lpfcutoff", 0.3}}},
        {"Tines", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 3}, {"mixlevel2", 0.3}, {"ampdecay", 0.8}}},
        {"Koto", {{"vco1waveform", 1}, {"vco1shape", 0.9}, {"lpfcutoff", 0.4}, {"ampdecay", 0.2}}},
        {"Harp", {{"vco1waveform", 0}, {"vco2pitch", 12}, {"ampdecay", 0.6}, {"delaymix", 0.3}}},
        {"Chirp", {{"vco1waveform", 2}, {"modtarget", 0}, {"modintensity", 0.8}, {"moddecay", 0.05}, {"ampdecay", 0.1}}},
        {"Bamboo", {{"vco1waveform", 0}, {"vco2waveform", 1}, {"vco2pitch", 19}, {"ampdecay", 0.2}}},
        {"Marimba", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", 1}, {"lpfcutoff", 0.3}, {"ampdecay", 0.4}}},
        {"Clav", {{"vco1waveform", 2}, {"vco1shape", 0.2}, {"lpfcutoff", 0.4}, {"ampdecay", 0.25}}},
        {"Plastic Pluck", {{"vco1waveform", 2}, {"vco1shape", 0.7}, {"lpfcutoff", 0.3}, {"ampdecay", 0.2}}},
        {"Square Stab", {{"vco1waveform", 2}, {"lpfcutoff", 0.4}, {"ampdecay", 0.1}, {"modintensity", 0.5}}},
        {"Glocken", {{"vco1waveform", 0}, {"vco2octave", 3}, {"ampdecay", 0.7}, {"delaymix", 0.4}}},
        {"Shorty", {{"vco1waveform", 1}, {"lpfcutoff", 0.1}, {"ampdecay", 0.05}}},

        // --- KEYS (80-99) ---
        {"Organ", {{"vco1waveform", 2}, {"vco2waveform", 2}, {"vco2octave", 1}, {"mixlevel1", 0.8}, {"mixlevel2", 0.6}, {"ampattack", 0.0}, {"ampsustain", 1.0}}},
        {"Classic Poly", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2octave", 0}, {"vco2pitch", 2}, {"mixlevel1", 1.0}, {"mixlevel2", 1.0}, {"lpfcutoff", 0.6}, {"ampattack", 0.1}, {"ampdecay", 0.4}, {"ampsustain", 0.6}, {"amprelease", 0.3}}},
        {"Rhodesy", {{"vco1waveform", 0}, {"vco2waveform", 2}, {"vco2octave", 1}, {"mixlevel1", 1.0}, {"mixlevel2", 0.2}, {"lpfcutoff", 0.4}, {"ampdecay", 0.8}}},
        {"Wurly", {{"vco1waveform", 2}, {"vco1shape", 0.1}, {"vco2waveform", 1}, {"lpfcutoff", 0.3}, {"ampdecay", 0.7}}},
        {"EPiano", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", 1}, {"vco2pitch", 5}, {"lpfcutoff", 0.5}, {"ampdecay", 0.9}}},
        {"Jazz Organ", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", 2}, {"mixlevel1", 0.8}, {"mixlevel2", 0.5}}},
        {"Church", {{"vco1waveform", 1}, {"vco2waveform", 2}, {"vco2octave", 1}, {"voicemode", 1}, {"ampattack", 0.1}}},
        {"Digital Key", {{"vco1waveform", 2}, {"vco2waveform", 2}, {"vco2octave", 2}, {"lpfcutoff", 0.6}, {"delaymix", 0.3}}},
        {"Bright Keys", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 12}, {"lpfcutoff", 0.7}, {"ampdecay", 0.6}}},
        {"Mellow Keys", {{"vco1waveform", 0}, {"lpfcutoff", 0.3}, {"ampattack", 0.05}, {"ampsustain", 0.7}}},
        {"Synth Brass", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 5}, {"lpfcutoff", 0.4}, {"ampattack", 0.1}, {"ampsustain", 0.8}}},
        {"Brass Swell", {{"vco1waveform", 1}, {"modtarget", 2}, {"modintensity", 0.6}, {"modattack", 0.2}, {"moddecay", 0.5}}},
        {"Detuned Keys", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 10}, {"voicemode", 1}}},
        {"Pulse Keys", {{"vco1waveform", 2}, {"vco1shape", 0.3}, {"lpfcutoff", 0.5}, {"lfomode", 1}, {"lfotarget", 1}, {"lfointensity", 0.5}, {"lforate", 0.4}}},
        {"Analog Keys", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 3}, {"lpfcutoff", 0.4}}},
        {"Vibe", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2octave", 1}, {"lpfcutoff", 0.2}, {"ampdecay", 0.5}}},
        {"Gospel", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2octave", -1}, {"mixlevel2", 0.4}}},
        {"Funky", {{"vco1waveform", 2}, {"vco1shape", 0.6}, {"lpfcutoff", 0.6}, {"lpfresonance", 0.5}}},
        {"Harpsi", {{"vco1waveform", 2}, {"vco1shape", 0.8}, {"ampdecay", 0.3}}},
        {"Boutique", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 1200}, {"lpfcutoff", 0.4}}},

        // --- FX / TEXTURE (100-119) ---
        {"Wind", {{"vco1waveform", 2}, {"vco1shape", 0.5}, {"lpfcutoff", 0.2}, {"lpfresonance", 0.9}, {"ampattack", 1.0}}},
        {"Rain", {{"vco1waveform", 2}, {"vco1shape", 0.9}, {"hpfcutoff", 0.7}, {"delaymix", 0.6}}},
        {"Static", {{"vco1waveform", 2}, {"vco1shape", 0.95}, {"hpfcutoff", 0.5}}},
        {"Engine", {{"vco1waveform", 1}, {"vco1shape", 0.5}, {"vco2sync", 1}, {"vco2pitch", -24}}},
        {"Aliens", {{"vco1waveform", 0}, {"vco2waveform", 1}, {"vco2sync", 1}, {"modtarget", 1}, {"modintensity", 0.9}, {"moddecay", 1.0}}},
        {"Siren", {{"vco1waveform", 1}, {"modtarget", 0}, {"modintensity", 1.0}, {"modattack", 0.5}, {"moddecay", 0.5}}},
        {"Laser", {{"vco1waveform", 1}, {"modtarget", 0}, {"modintensity", 1.0}, {"moddecay", 0.1}}},
        {"UFO", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2sync", 1}, {"delaymix", 0.8}}},
        {"Rumble", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 1}, {"lpfcutoff", 0.1}, {"voicemode", 1}}},
        {"Birds", {{"vco1waveform", 0}, {"vco2waveform", 0}, {"vco2pitch", 36}, {"modintensity", 0.8}, {"delaymix", 0.4}}},
        {"Computer", {{"vco1waveform", 2}, {"vco2sync", 1}, {"modintensity", 0.9}, {"delaymix", 0.6}}},
        {"Drone", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 1}, {"ampattack", 1.0}, {"ampsustain", 1.0}}},
        {"Sweep FX", {{"vco1waveform", 1}, {"lpfcutoff", 0.1}, {"modtarget", 2}, {"modintensity", 1.0}, {"modattack", 0.8}}},
        {"Glitch", {{"vco1waveform", 2}, {"vco2sync", 1}, {"vco2pitch", 48}, {"moddecay", 0.05}}},
        {"Ocean", {{"vco1waveform", 2}, {"lpfcutoff", 0.1}, {"ampattack", 1.0}, {"amprelease", 1.0}}},
        {"Alarm", {{"vco1waveform", 2}, {"vco2waveform", 2}, {"vco2pitch", 7}, {"modintensity", 0.7}}},
        {"Robotic", {{"vco1waveform", 2}, {"vco1shape", 0.4}, {"vco2sync", 1}, {"lpfresonance", 0.6}}},
        {"Falling", {{"vco1waveform", 1}, {"modtarget", 0}, {"modintensity", -0.8}, {"modattack", 0.0}, {"moddecay", 1.0}}},
        {"Rising", {{"vco1waveform", 1}, {"modtarget", 0}, {"modintensity", 0.8}, {"modattack", 1.0}}},
        {"Metallic", {{"vco1waveform", 1}, {"vco2sync", 1}, {"vco2pitch", 14}, {"lpfcutoff", 0.5}}},

        // --- OTHERS (120-126) ---
        {"Accordion", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 2}, {"ampattack", 0.1}, {"lpfcutoff", 0.6}}},
        {"Harmonica", {{"vco1waveform", 2}, {"vco1shape", 0.4}, {"lpfcutoff", 0.4}, {"ampattack", 0.05}}},
        {"Whistle 2", {{"vco1waveform", 0}, {"portamento", 0.3}, {"lpfcutoff", 0.8}}},
        {"Bagpipes", {{"vco1waveform", 1}, {"vco2waveform", 1}, {"vco2pitch", 1}, {"voicemode", 1}, {"ampattack", 0.2}}},
        {"Flute", {{"vco1waveform", 0}, {"ampattack", 0.1}, {"delaymix", 0.2}}},
        {"Oboe", {{"vco1waveform", 1}, {"vco1shape", 0.7}, {"lpfcutoff", 0.3}, {"ampattack", 0.1}}},
        {"End of Line", {{"vco1waveform", 2}, {"vco1shape", 0.95}, {"lpfcutoff", 0.1}, {"delaymix", 0.8}, {"delayfeedback", 0.8}}}
    };

    return presetList;
}

} // namespace noteahead
