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

#include "drum_synth_v2_device.hpp"

#include "../../common/constants.hpp"

#include "../dsp/drum/clap_engine.hpp"

namespace noteahead {

using namespace DrumSynthV2;

DrumSynthV2Device::DrumSynthV2Device(std::string name)
  : DrumPadDevice { std::move(name) }
{
    initializeEngines();

    for (int i { 0 }; i < NumVoices; i++) {
        addVoiceParameters(i);
    }

    DrumSynthV2Device::syncParameters();
}

std::string DrumSynthV2Device::typeName() const
{
    return Constants::drumSynthV2DeviceName().toStdString();
}

std::string DrumSynthV2Device::typeIdString()
{
    return "cf30a9f9-3709-45e3-bf96-80676a5f524b";
}

std::string DrumSynthV2Device::typeId() const
{
    return typeIdString();
}

std::unique_ptr<DrumPadDevice> DrumSynthV2Device::newInstance(std::string name) const
{
    return std::make_unique<DrumSynthV2Device>(std::move(name));
}

//! Puts the amp envelope back the way it shipped, before a project is read.
//!
//! A kit saved before the sustain stage existed carries no sustain attribute, and an absent
//! parameter keeps whatever the container holds -- which is now an envelope that does nothing. Such
//! a kit was voiced against an envelope parked just past each voice's tail, so reading it without
//! putting those values back first would quietly lengthen every drum in it.
//!
//! The same shape as SpeechDevice's legacy voice engine, and for the same reason.
void DrumSynthV2Device::restoreLegacyParameters()
{
    struct AmpEnvelopeDefaults
    {
        float hold {};
        float decay {};
    };

    static constexpr std::array<AmpEnvelopeDefaults, NumVoices> legacy { {
      { 0.4661f, 0.6443f }, // Kick: hold 810 ms, decay 580 ms
      { 0.3832f, 0.5759f }, // Snare: hold 450 ms, decay 350 ms
      { 0.1957f, 0.2819f }, // ClosedHiHat: hold 60 ms, decay 40 ms
      { 0.3714f, 0.5129f }, // Clap: hold 410 ms, decay 220 ms
      { 0.3969f, 0.5719f }, // OpenHiHat: hold 500 ms, decay 340 ms
      { 0.5499f, 0.7068f }, // LowTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // MidTom: hold 1330 ms, decay 920 ms
      { 0.5499f, 0.7068f }, // HighTom: hold 1330 ms, decay 920 ms
      { 0.6937f, 0.8724f }, // Crash: hold 2670 ms, decay 3120 ms
      { 0.7570f, 0.8345f }, // Ride: hold 3470 ms, decay 2360 ms
      { 0.6300f, 0.5940f }, // ReverseCrash: hold 2000 ms, decay 400 ms
      { 0.1957f, 0.2819f }, // Rim: hold 60 ms, decay 40 ms -- it is over almost at once
    } };

    // writeVoiceParameter() rather than updateVoiceParameter(), which emits dataChanged(). This
    // runs from deserializeFromXml() with the device mutex held, and that signal reaches
    // MidiService, whose handler goes out to the worker thread: emitting it from under the lock
    // deadlocks the load. The same trap the Speech device's legacy engine fell into.
    for (int index = 0; index < NumVoices; index++) {
        const auto & voiceLegacy = legacy.at(static_cast<size_t>(index));
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpHold().toStdString(), voiceLegacy.hold, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpDecay().toStdString(), voiceLegacy.decay, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpSustain().toStdString(), 0.0f, true);
        writeVoiceParameter(index, Constants::NahdXml::xmlKeyAmpRelease().toStdString(), 0.0f, true);
    }
}

void DrumSynthV2Device::initializeEngines()
{
    for (int i { 0 }; i < NumVoices; i++) {
        const auto voiceIdx { static_cast<VoiceIndex>(i) };
        if (voiceIdx == VoiceIndex::Kick)
            setVoiceEngine(i, std::make_unique<KickEngine>());
        else if (voiceIdx == VoiceIndex::Snare) {
            auto snare = std::make_unique<SnareEngine>();
            snare->setVoicing(SnareEngine::Voicing::Rd9);
            setVoiceEngine(i, std::move(snare));
        } else if (voiceIdx == VoiceIndex::Clap)
            setVoiceEngine(i, std::make_unique<ClapEngine>());
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            setVoiceEngine(i, std::make_unique<HiHatEngine>());
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom) {
            auto tom = std::make_unique<TomEngine>();
            tom->setVoicing(TomEngine::Voicing::Rd9);
            setVoiceEngine(i, std::move(tom));
        } else if (voiceIdx == VoiceIndex::Rim)
            setVoiceEngine(i, std::make_unique<RimEngine>());
        else if (voiceIdx == VoiceIndex::Crash) {
            // The cymbals are the two voices V2 does not share with V1. Both were fitted to a
            // recording: the crash had four tenths of steady noise against four of metal, which
            // measured as a wash rather than as struck metal, and neither of them had any body
            // below 600 Hz at all.
            auto crash = std::make_unique<CrashEngine>();
            crash->setVoicing(CrashEngine::Voicing::Rd9);
            setVoiceEngine(i, std::move(crash));
        } else if (voiceIdx == VoiceIndex::Ride) {
            auto ride = std::make_unique<RideEngine>();
            ride->setVoicing(RideEngine::Voicing::Rd9);
            setVoiceEngine(i, std::move(ride));
        } else if (voiceIdx == VoiceIndex::ReverseCrash) {
            auto crash = std::make_unique<CrashEngine>();
            crash->setMode(CrashEngine::Mode::Reverse);
            crash->setVoicing(CrashEngine::Voicing::Rd9);
            setVoiceEngine(i, std::move(crash));
        }
    }
}

void DrumSynthV2Device::addEngineParameters(int index, const std::string & prefix)
{
    float defTune = 0.5f;
    float defDecay = 0.5f;

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::ClosedHiHat) {
        defTune = 0.7f;
        defDecay = 0.1f;
    } else if (voiceIdx == VoiceIndex::OpenHiHat) {
        defTune = 0.6f;
        defDecay = 0.8f;
    } else if (voiceIdx == VoiceIndex::LowTom) {
        defTune = 0.2f;
    } else if (voiceIdx == VoiceIndex::MidTom) {
        defTune = 0.5f;
    } else if (voiceIdx == VoiceIndex::HighTom) {
        defTune = 0.8f;
    }

    // Common engine parameters
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTune().toStdString(), defTune, 0, 10000, static_cast<int>(defTune * 10000), 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyDecay().toStdString(), defDecay, 0, 10000, static_cast<int>(defDecay * 10000), 100 });

    if (voiceIdx == VoiceIndex::Kick)
        addKickParameters(prefix);
    else if (voiceIdx == VoiceIndex::Snare)
        addSnareParameters(prefix);
    else if (voiceIdx == VoiceIndex::Clap) {
    } else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
        addHiHatParameters(prefix);
    else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
        addTomParameters(prefix);
    else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash)
        addCymbalParameters(prefix);
    else if (voiceIdx == VoiceIndex::Rim)
        addRimParameters(prefix);
}

void DrumSynthV2Device::addKickParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyClickTune().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addRimParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyClick().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addSnareParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeySnappy().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyTone().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addTomParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString(), 0.5f, 0, 10000, 5000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString(), 0.5f, 0, 10000, 5000, 100 });
}

void DrumSynthV2Device::addHiHatParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
}

void DrumSynthV2Device::addCymbalParameters(const std::string & prefix)
{
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyResonance().toStdString(), 0.3f, 0, 10000, 3000, 100 });
    addParameter(Parameter { prefix + Constants::NahdXml::xmlKeyAttack().toStdString(), 0.0f, 0, 10000, 0, 100 });
}

void DrumSynthV2Device::syncEngineParameters(int index, const std::string & prefix)
{
    syncCommonEngineParameters(index, prefix);

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::Kick)
        syncKickParameters(prefix);
    else if (voiceIdx == VoiceIndex::Snare)
        syncSnareParameters(prefix);
    else if (voiceIdx == VoiceIndex::Clap)
        syncClapParameters(prefix);
    else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
        syncHiHatParameters(index, prefix);
    else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
        syncTomParameters(index, prefix);
    else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash)
        syncCymbalParameters(index, prefix);
    else if (voiceIdx == VoiceIndex::Rim)
        syncRimParameters(index, prefix);
}

void DrumSynthV2Device::syncCommonEngineParameters(int index, const std::string & prefix)
{
    DrumEngine & engine { voiceEngine(index) };

    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTune().toStdString()); p) {
        const float val { p->get().value() };
        if (voiceIdx == VoiceIndex::Kick)
            static_cast<KickEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::Snare)
            static_cast<SnareEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::Clap)
            static_cast<ClapEngine &>(engine).setTune(val);
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            static_cast<HiHatEngine &>(engine).setTune(val);
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
            static_cast<TomEngine &>(engine).setTune(val);
        else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash) {
            if (voiceIdx == VoiceIndex::Ride)
                static_cast<RideEngine &>(engine).setTune(val);
            else
                static_cast<CrashEngine &>(engine).setTune(val);
        } else if (voiceIdx == VoiceIndex::Rim)
            static_cast<RimEngine &>(engine).setTune(val);
    }

    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyDecay().toStdString()); p) {
        const float val { p->get().value() };
        if (voiceIdx == VoiceIndex::Kick)
            static_cast<KickEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::Snare)
            static_cast<SnareEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::Clap)
            static_cast<ClapEngine &>(engine).setDecay(val);
        else if (voiceIdx == VoiceIndex::ClosedHiHat || voiceIdx == VoiceIndex::OpenHiHat)
            static_cast<HiHatEngine &>(engine).setDecay(val);
        else if (voiceIdx >= VoiceIndex::LowTom && voiceIdx <= VoiceIndex::HighTom)
            static_cast<TomEngine &>(engine).setDecay(val);
        else if (voiceIdx >= VoiceIndex::Crash && voiceIdx <= VoiceIndex::ReverseCrash) {
            if (voiceIdx == VoiceIndex::Ride)
                static_cast<RideEngine &>(engine).setDecay(val);
            else
                static_cast<CrashEngine &>(engine).setDecay(val);
        } else if (voiceIdx == VoiceIndex::Rim)
            static_cast<RimEngine &>(engine).setDecay(val);
    }
}

void DrumSynthV2Device::syncKickParameters(const std::string & prefix)
{
    auto & engine { static_cast<KickEngine &>(voiceEngine(static_cast<int>(VoiceIndex::Kick))) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p)
        engine.setAttack(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyClickTune().toStdString()); p)
        engine.setClickTune(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p)
        engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p)
        engine.setPitchDecay(p->get().value());
}

void DrumSynthV2Device::syncSnareParameters(const std::string & prefix)
{
    auto & engine { static_cast<SnareEngine &>(voiceEngine(static_cast<int>(VoiceIndex::Snare))) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeySnappy().toStdString()); p)
        engine.setSnappy(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyTone().toStdString()); p)
        engine.setTone(p->get().value());
}

void DrumSynthV2Device::syncClapParameters(const std::string & /*prefix*/)
{
    // Clap currently has no specific parameters beyond Tune and Decay
}

void DrumSynthV2Device::syncTomParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<TomEngine &>(voiceEngine(index)) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDepth().toStdString()); p)
        engine.setPitchDepth(p->get().value());
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyPitchDecay().toStdString()); p)
        engine.setPitchDecay(p->get().value());
}

void DrumSynthV2Device::syncHiHatParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<HiHatEngine &>(voiceEngine(index)) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
        engine.setResonance(p->get().value());
}

void DrumSynthV2Device::syncRimParameters(int index, const std::string & prefix)
{
    auto & engine { static_cast<RimEngine &>(voiceEngine(index)) };
    if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyClick().toStdString()); p)
        engine.setClick(p->get().value());
}

void DrumSynthV2Device::syncCymbalParameters(int index, const std::string & prefix)
{
    const auto voiceIdx { static_cast<VoiceIndex>(index) };
    if (voiceIdx == VoiceIndex::Crash || voiceIdx == VoiceIndex::ReverseCrash) {
        auto & engine { static_cast<CrashEngine &>(voiceEngine(index)) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
            engine.setResonance(p->get().value());
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyAttack().toStdString()); p)
            engine.setAttack(p->get().value());
    } else if (voiceIdx == VoiceIndex::Ride) {
        auto & engine { static_cast<RideEngine &>(voiceEngine(index)) };
        if (auto p = parameter(prefix + Constants::NahdXml::xmlKeyResonance().toStdString()); p)
            engine.setResonance(p->get().value());
    }
}

} // namespace noteahead
