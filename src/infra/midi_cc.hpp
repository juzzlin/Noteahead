// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_CC_HPP
#define MIDI_CC_HPP

namespace noteahead {

enum class MidiCc
{
    BankSelectMSB = 0,
    ModulationWheelMSB = 1,
    BreathControllerMSB = 2,
    FootControllerMSB = 4,
    PortamentoTimeMSB = 5,
    DataEntryMSB = 6,
    ChannelVolumeMSB = 7,
    BalanceMSB = 8,
    PanMSB = 10,
    ExpressionControllerMSB = 11,
    EffectControl1MSB = 12,
    EffectControl2MSB = 13,
    GeneralPurpose1MSB = 16,
    GeneralPurpose2MSB = 17,
    GeneralPurpose3MSB = 18,
    GeneralPurpose4MSB = 19,
    BankSelectLSB = 32,
    ModulationWheelLSB = 33,
    BreathControllerLSB = 34,
    FootControllerLSB = 36,
    PortamentoTimeLSB = 37,
    DataEntryLSB = 38,
    ChannelVolumeLSB = 39,
    BalanceLSB = 40,
    PanLSB = 42,
    ExpressionControllerLSB = 43,
    EffectControl1LSB = 44,
    EffectControl2LSB = 45,
    GeneralPurpose1LSB = 48,
    GeneralPurpose2LSB = 49,
    GeneralPurpose3LSB = 50,
    GeneralPurpose4LSB = 51,
    SustainPedal = 64,
    Portamento = 65,
    Sostenuto = 66,
    SoftPedal = 67,
    LegatoFootswitch = 68,
    Hold2 = 69,
    SoundController1 = 70, // (default: Sound Variation)
    SoundController2 = 71, // (default: Timbre/Harmonic Intensity)
    SoundController3 = 72, // (default: Release Time)
    SoundController4 = 73, // (default: Attack Time)
    SoundController5 = 74, // (default: Brightness)
    SoundController6 = 75,
    SoundController7 = 76,
    SoundController8 = 77,
    SoundController9 = 78,
    SoundController10 = 79,
    GeneralPurpose5 = 80,
    GeneralPurpose6 = 81,
    GeneralPurpose7 = 82,
    GeneralPurpose8 = 83,
    PortamentoControl = 84,
    Effects1Depth = 91, // (default: Reverb Send Level)
    Effects2Depth = 92, // (default: Tremolo Depth)
    Effects3Depth = 93, // (default: Chorus Send Level)
    Effects4Depth = 94, // (default: Detune Depth)
    Effects5Depth = 95, // (default: Phaser Depth)
    DataIncrement = 96,
    DataDecrement = 97,
    NRPNLSB = 98,
    NRPNMSB = 99,
    RPNLSB = 100,
    RPNMSB = 101,
    AllSoundOff = 120,
    ResetAllControllers = 121,
    LocalControl = 122,
    AllNotesOff = 123,
    OmniModeOff = 124,
    OmniModeOn = 125,
    MonoModeOn = 126,
    PolyModeOn = 127
};
}

#endif // MIDI_CC_HPP
