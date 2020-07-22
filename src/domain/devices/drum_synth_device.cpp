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

#include "drum_synth_device.hpp"

#include "../../common/constants.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../dsp/drum/clap_engine.hpp"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

void DrumSynthDevice::Pad::updateEffects()
{
    volumeEffect->setVolume(level);
    panningEffect->setPan(pan);
    lpf->setCutoff(lpfCutoff);
    hpf->setCutoff(hpfCutoff);
}

DrumSynthDevice::DrumSynthDevice(std::string name)
  : m_name { std::move(name) }
{
    initializePads();

    for (int i { 0 }; i < NumPads; ++i) {
        addPadParameters(i);
    }

    setManualPan(panInternal());
    setManualVolume(volumeInternal());
    setManualGain(gainInternal());

    syncParameters();
}

std::string DrumSynthDevice::name() const
{
    return m_name;
}

std::string DrumSynthDevice::category() const
{
    return "Drums";
}

std::string DrumSynthDevice::typeId() const
{
    return "a7f5a47e-4786-11f1-92b0-0b3f3bef9f74";
}

void DrumSynthDevice::processMidiNoteOn(uint8_t note, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && pad : m_pads) {
        if (pad.midiNote == note) {
            const float vel { static_cast<float>(velocity) / 127.0f };
            pad.engine->trigger(vel);
            break;
        }
    }
}

void DrumSynthDevice::processMidiNoteOff(uint8_t note)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    // Closed Hat choke logic
    if (note == 42 || note == 44) { // Closed Hat or Pedal Hat
        m_pads[4].engine->reset(); // Reset Open Hat (46)
    }
}

void DrumSynthDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t /*channel*/)
{
    using namespace MidiCcMapping;

    bool changed { false };
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            updatePanParameter(manualPanInternal(), false);
            updateVolumeParameter(manualVolumeInternal(), false);
            updateGainParameter(manualGainInternal(), false);
            changed = true;
        } else {
            const float val { static_cast<float>(value) / 127.0f };

            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController1)) { // CC 70: Focus Pad
                m_selectedPad = std::clamp(static_cast<int>(value), 0, NumPads - 1);
                changed = true;
            } else if (controller == static_cast<uint8_t>(Controller::SoundController2)) { // CC 71: Level
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyLevel().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController3)) { // CC 72: Pan
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPan().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController4)) { // CC 73: LPF
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyCutoff().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController5)) { // CC 74: HPF
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController6)) { // CC 75: Tune
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyTune().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController7)) { // CC 76: Decay
                changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyDecay().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController8)) { // CC 77: Specific 1
                if (m_selectedPad == 0) changed |= updatePadParameter(0, Constants::NahdXml::xmlKeyAttack().toStdString(), val);
                else if (m_selectedPad == 1) changed |= updatePadParameter(1, Constants::NahdXml::xmlKeySnappy().toStdString(), val);
                else if (m_selectedPad >= 5 && m_selectedPad <= 7) changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), val);
                else if (m_selectedPad >= 2 && m_selectedPad <= 4) changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyResonance().toStdString(), val);
                else if (m_selectedPad >= 8 && m_selectedPad <= 10) changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyResonance().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController9)) { // CC 78: Specific 2
                if (m_selectedPad == 0) changed |= updatePadParameter(0, Constants::NahdXml::xmlKeyPitchDepth().toStdString(), val);
                else if (m_selectedPad == 1) changed |= updatePadParameter(1, Constants::NahdXml::xmlKeyTone().toStdString(), val);
                else if (m_selectedPad >= 5 && m_selectedPad <= 7) changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), val);
                else if (m_selectedPad == PadIndex::Crash || m_selectedPad == PadIndex::ReverseCrash) changed |= updatePadParameter(m_selectedPad, Constants::NahdXml::xmlKeyAttack().toStdString(), val);
            } else if (controller == static_cast<uint8_t>(Controller::SoundController10)) { // CC 79: Specific 3
                if (m_selectedPad == 0) changed |= updatePadParameter(0, Constants::NahdXml::xmlKeyPitchDecay().toStdString(), val);
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void DrumSynthDevice::processMidiAllNotesOff()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && pad : m_pads) {
        pad.engine->reset();
    }
}

void DrumSynthDevice::processAudio(float * output, uint32_t nFrames, uint32_t sampleRate)
{
    setSampleRate(sampleRate);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    for (auto && pad : m_pads) {
        pad.engine->setSampleRate(sampleRate);
        pad.lpf->setSampleRate(sampleRate);
        pad.hpf->setSampleRate(sampleRate);
    }

    const float globalVol { volumeInternal() * linearGainInternal() };

    for (uint32_t i { 0 }; i < nFrames; i++) {
        float mixL { 0.0f };
        float mixR { 0.0f };

        for (auto && pad : m_pads) {
            if (pad.engine->isActive()) {
                float sample { pad.engine->nextSample() };
                float l { sample };
                float r { sample };

                pad.lpf->process(l, r);
                pad.hpf->process(l, r);
                pad.volumeEffect->process(l, r);
                pad.panningEffect->process(l, r);

                mixL += l;
                mixR += r;
            }
        }

        output[i * 2] += mixL * globalVol;
        output[i * 2 + 1] += mixR * globalVol;
    }
}

void DrumSynthDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    for (auto && pad : m_pads) {
        pad.engine->reset();
    }
}

void DrumSynthDevice::serializeToXml(QXmlStreamWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeParametersToXml(writer);
    writer.writeEndElement();
}

void DrumSynthDevice::deserializeFromXml(QXmlStreamReader & reader)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    deserializeAttributesFromXml(reader);
    deserializeParametersFromXml(reader);
    syncParameters();
}

int DrumSynthDevice::selectedPad() const
{
    return m_selectedPad;
}

void DrumSynthDevice::setSelectedPad(int index)
{
    m_selectedPad = std::clamp(index, 0, NumPads - 1);
}

uint8_t DrumSynthDevice::padNote(int index) const
{
    if (index >= 0 && index < NumPads) {
        return m_pads.at(index).midiNote;
    }
    return 0;
}

void DrumSynthDevice::initializePads()
{
    // Define GM mapping
    const std::array<uint8_t, NumPads> notes { 36, 38, 42, 39, 46, 41, 43, 45, 49, 51, 52 }; // Kick, Snare, ClHat, Clap, OpHat, LowTom, MidTom, HiTom, Crash, Ride, RevCrash

    for (int i { 0 }; i < NumPads; i++) {
        m_pads.at(i).midiNote = notes.at(i);
        m_pads.at(i).lpf = std::make_shared<LowPassFilterEffect>();
        m_pads.at(i).hpf = std::make_shared<HighPassFilterEffect>();
        m_pads.at(i).volumeEffect = std::make_shared<VolumeEffect>();
        m_pads.at(i).panningEffect = std::make_shared<PanningEffect>();

        if (i == PadIndex::Kick) m_pads.at(i).engine = std::make_unique<KickEngine>();
        else if (i == PadIndex::Snare) m_pads.at(i).engine = std::make_unique<SnareEngine>();
        else if (i == PadIndex::Clap) m_pads.at(i).engine = std::make_unique<ClapEngine>();
        else if (i == PadIndex::ClosedHiHat || i == PadIndex::OpenHiHat) m_pads.at(i).engine = std::make_unique<HiHatEngine>();
        else if (i >= PadIndex::LowTom && i <= PadIndex::HighTom) m_pads.at(i).engine = std::make_unique<TomEngine>();
        else if (i == PadIndex::Crash) {
            m_pads.at(i).engine = std::make_unique<CrashEngine>();
        } else if (i == PadIndex::Ride) {
            m_pads.at(i).engine = std::make_unique<RideEngine>();
        } else if (i == PadIndex::ReverseCrash) {
            auto crash = std::make_unique<CrashEngine>();
            crash->setMode(CrashEngine::Mode::Reverse);
            m_pads.at(i).engine = std::move(crash);
        }
    }
}

void DrumSynthDevice::addPadParameters(int index)
{
    const std::string prefix { "Pad" + std::to_string(index) + "_" };
    
    float defTune = 0.5f;
    float defDecay = 0.5f;

    if (index == 2) { defTune = 0.7f; defDecay = 0.1f; } // ClHat
    else if (index == 4) { defTune = 0.6f; defDecay = 0.8f; } // OpHat
    else if (index == 5) { defTune = 0.2f; } // Low Tom
    else if (index == 6) { defTune = 0.5f; } // Mid Tom
    else if (index == 7) { defTune = 0.8f; } // Hi Tom

    // Standard pad parameters
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyLevel().toStdString(), 0.8f, 0, 100, 80 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyCutoff().toStdString(), 1.0f, 0, 100, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.0f, 0, 100, 0 });

    // Common engine parameters
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTune().toStdString(), defTune, 0, 100, static_cast<int>(defTune * 100) });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyDecay().toStdString(), defDecay, 0, 100, static_cast<int>(defDecay * 100) });

    if (index == 0) addKickParameters(prefix);
    else if (index == 1) addSnareParameters(prefix);
    else if (index == 3) {} // Clap
    else if (index == 2 || index == 4) addHiHatParameters(prefix);
    else if (index >= 5 && index <= 7) addTomParameters(prefix);
    else if (index >= 8 && index <= 10) addCymbalParameters(prefix);
}

void DrumSynthDevice::addKickParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyClickTune().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 100, 50 });
}

void DrumSynthDevice::addSnareParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeySnappy().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTone().toStdString(), 0.5f, 0, 100, 50 });
}

void DrumSynthDevice::addTomParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 100, 50 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 100, 50 });
}

void DrumSynthDevice::addHiHatParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 100, 30 });
}

void DrumSynthDevice::addCymbalParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 100, 30 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f, 0, 100, 0 });
}

void DrumSynthDevice::syncParameters()
{
    Device::syncParameters();

    for (int i { 0 }; i < NumPads; ++i) {
        syncPadParameters(i);
    }
}

void DrumSynthDevice::syncPadParameters(int index)
{
    const std::string prefix { "Pad" + std::to_string(index) + "_" };
    
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyLevel().toStdString()); p) m_pads.at(index).level = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPan().toStdString()); p) m_pads.at(index).pan = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyCutoff().toStdString()); p) m_pads.at(index).lpfCutoff = p->get().value();
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyHpfCutoff().toStdString()); p) m_pads.at(index).hpfCutoff = p->get().value();

    m_pads.at(index).updateEffects();

    syncCommonEngineParameters(index, prefix);

    if (index == PadIndex::Kick) syncKickParameters(prefix);
    else if (index == PadIndex::Snare) syncSnareParameters(prefix);
    else if (index == PadIndex::Clap) syncClapParameters(prefix);
    else if (index == PadIndex::ClosedHiHat || index == PadIndex::OpenHiHat) syncHiHatParameters(index, prefix);
    else if (index >= PadIndex::LowTom && index <= PadIndex::HighTom) syncTomParameters(index, prefix);
    else if (index >= PadIndex::Crash && index <= PadIndex::ReverseCrash) syncCymbalParameters(index, prefix);
}

void DrumSynthDevice::syncCommonEngineParameters(int index, const std::string & prefix)
{
    DrumEngine & engine { *m_pads.at(index).engine };

    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTune().toStdString()); p) {
        const float val { p->get().value() };
        if (index == PadIndex::Kick) static_cast<KickEngine &>(engine).setTune(val);
        else if (index == PadIndex::Snare) static_cast<SnareEngine &>(engine).setTune(val);
        else if (index == PadIndex::Clap) static_cast<ClapEngine &>(engine).setTune(val);
        else if (index == PadIndex::ClosedHiHat || index == PadIndex::OpenHiHat) static_cast<HiHatEngine &>(engine).setTune(val);
        else if (index >= PadIndex::LowTom && index <= PadIndex::HighTom) static_cast<TomEngine &>(engine).setTune(val);
        else if (index >= PadIndex::Crash && index <= PadIndex::ReverseCrash) {
            if (index == PadIndex::Ride) static_cast<RideEngine &>(engine).setTune(val);
            else static_cast<CrashEngine &>(engine).setTune(val);
        }
    }

    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        const float val { p->get().value() };
        if (index == PadIndex::Kick) static_cast<KickEngine &>(engine).setDecay(val);
        else if (index == PadIndex::Snare) static_cast<SnareEngine &>(engine).setDecay(val);
        else if (index == PadIndex::Clap) static_cast<ClapEngine &>(engine).setDecay(val);
        else if (index == PadIndex::ClosedHiHat || index == PadIndex::OpenHiHat) static_cast<HiHatEngine &>(engine).setDecay(val);
        else if (index >= PadIndex::LowTom && index <= PadIndex::HighTom) static_cast<TomEngine &>(engine).setDecay(val);
        else if (index >= PadIndex::Crash && index <= PadIndex::ReverseCrash) {
            if (index == PadIndex::Ride) static_cast<RideEngine &>(engine).setDecay(val);
            else static_cast<CrashEngine &>(engine).setDecay(val);
        }
    }
}

void DrumSynthDevice::syncKickParameters(const std::string & prefix)
{
    auto & engine { static_cast<KickEngine &>(*m_pads.at(0).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p) engine.setAttack(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyClickTune().toStdString()); p) engine.setClickTune(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p) engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p) engine.setPitchDecay(p->get().value());
}

void DrumSynthDevice::syncSnareParameters(const std::string & prefix)
{
    auto & engine { static_cast<SnareEngine &>(*m_pads.at(1).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeySnappy().toStdString()); p) engine.setSnappy(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTone().toStdString()); p) engine.setTone(p->get().value());
}

void DrumSynthDevice::syncClapParameters(const std::string & /*prefix*/)
{
    // Clap currently has no specific parameters beyond Tune and Decay
}

void DrumSynthDevice::syncTomParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<TomEngine &>(*m_pads.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p) engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p) engine.setPitchDecay(p->get().value());
}

void DrumSynthDevice::syncHiHatParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<HiHatEngine &>(*m_pads.at(index).engine) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p) engine.setResonance(p->get().value());
}

void DrumSynthDevice::syncCymbalParameters(int index, const std::string & prefix)
{
    if (index == PadIndex::Crash || index == PadIndex::ReverseCrash) {
        auto & engine { static_cast<CrashEngine &>(*m_pads.at(index).engine) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p) engine.setResonance(p->get().value());
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p) engine.setAttack(p->get().value());
    } else if (index == PadIndex::Ride) {
        auto & engine { static_cast<RideEngine &>(*m_pads.at(index).engine) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p) engine.setResonance(p->get().value());
    }
}

bool DrumSynthDevice::updatePadParameter(int padIndex, const std::string & paramName, float value)
{
    const std::string prefix { "Pad" + std::to_string(padIndex) + "_" };
    if (auto p = parameter(prefix + paramName); p) {
        p->get().setValue(value);
        syncPadParameters(padIndex);
        emit dataChanged();
        return true;
    }
    return false;
}

} // namespace noteahead
