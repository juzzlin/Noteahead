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

#include "noise_fx_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "noise_fx_device.hpp"
#include "noise_fx_presets.hpp"
#include "noise_fx_sweep_controller.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

namespace {

int toKnob(float value)
{
    return static_cast<int>(std::round(value * Constants::uiInternalScaling()));
}

float fromKnob(int value)
{
    return static_cast<float>(value) / Constants::uiInternalScaling();
}

//! @p beats as a note value: a quarter note is "1/4", four bars "4/1".
QString noteValue(double beats)
{
    const double wholeNotes = beats / 4.0;
    return wholeNotes >= 1.0 ? QString { "%1/1" }.arg(std::lround(wholeNotes)) : QString { "1/%1" }.arg(std::lround(1.0 / wholeNotes));
}

QStringList noteValues(const std::vector<double> & beats)
{
    QStringList names;
    for (auto && length : beats) {
        names.append(noteValue(length));
    }
    return names;
}

} // namespace

NoiseFxDialogModel::NoiseFxDialogModel(std::shared_ptr<NoiseFxDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    for (size_t sweep = 0; sweep < NoiseFxDevice::SweepCount; sweep++) {
        m_sweeps.push_back(new NoiseFxSweepController { sweep, *this });
    }
    connectDeviceSignals();
}

NoiseFxDialogModel::~NoiseFxDialogModel() = default;

DeviceDialogModel::DeviceS NoiseFxDialogModel::device() const
{
    return m_device;
}

bool NoiseFxDialogModel::setDevice(DeviceS device)
{
    if (const auto noiseFx = std::dynamic_pointer_cast<NoiseFxDevice>(device)) {
        setDevice(noiseFx);
        return true;
    }
    return false;
}

void NoiseFxDialogModel::setDevice(std::shared_ptr<NoiseFxDevice> device)
{
    if (m_device != device) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = std::move(device);
        connectDeviceSignals();
        applyScopeActive();
        emit deviceChanged();
        requestSettings();
    }
}

std::shared_ptr<NoiseFxDevice> NoiseFxDialogModel::noiseFxDevice() const
{
    return m_device;
}

QStringList NoiseFxDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (auto && preset : NoiseFxPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void NoiseFxDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int NoiseFxDialogModel::colour() const
{
    return m_device ? toKnob(m_device->colour()) : 0;
}

void NoiseFxDialogModel::setColour(int value)
{
    if (m_device) {
        m_device->setColour(fromKnob(value));
    }
}

int NoiseFxDialogModel::sampleHoldRate() const
{
    return m_device ? toKnob(m_device->sampleHoldRate()) : 0;
}

void NoiseFxDialogModel::setSampleHoldRate(int value)
{
    if (m_device) {
        m_device->setSampleHoldRate(fromKnob(value));
    }
}

int NoiseFxDialogModel::crackle() const
{
    return m_device ? toKnob(m_device->crackle()) : 0;
}

void NoiseFxDialogModel::setCrackle(int value)
{
    if (m_device) {
        m_device->setCrackle(fromKnob(value));
    }
}

int NoiseFxDialogModel::noiseLevel() const
{
    return m_device ? toKnob(m_device->noiseLevel()) : 0;
}

void NoiseFxDialogModel::setNoiseLevel(int value)
{
    if (m_device) {
        m_device->setNoiseLevel(fromKnob(value));
    }
}

int NoiseFxDialogModel::width() const
{
    return m_device ? toKnob(m_device->width()) : 0;
}

void NoiseFxDialogModel::setWidth(int value)
{
    if (m_device) {
        m_device->setWidth(fromKnob(value));
    }
}

int NoiseFxDialogModel::toneWaveform() const
{
    return m_device ? static_cast<int>(m_device->toneWaveform()) : 0;
}

void NoiseFxDialogModel::setToneWaveform(int value)
{
    if (m_device) {
        m_device->setToneWaveform(static_cast<PolyBlepOscillator::Waveform>(value));
    }
}

int NoiseFxDialogModel::pitch() const
{
    return m_device ? toKnob(m_device->pitch()) : 0;
}

void NoiseFxDialogModel::setPitch(int value)
{
    if (m_device) {
        m_device->setPitch(fromKnob(value));
    }
}

int NoiseFxDialogModel::blend() const
{
    return m_device ? toKnob(m_device->blend()) : 0;
}

void NoiseFxDialogModel::setBlend(int value)
{
    if (m_device) {
        m_device->setBlend(fromKnob(value));
    }
}

int NoiseFxDialogModel::ringMod() const
{
    return m_device ? toKnob(m_device->ringMod()) : 0;
}

void NoiseFxDialogModel::setRingMod(int value)
{
    if (m_device) {
        m_device->setRingMod(fromKnob(value));
    }
}

int NoiseFxDialogModel::filterType() const
{
    return m_device ? static_cast<int>(m_device->filterType()) : 0;
}

void NoiseFxDialogModel::setFilterType(int value)
{
    if (m_device) {
        m_device->setFilterType(static_cast<NoiseFxDevice::FilterType>(value));
    }
}

int NoiseFxDialogModel::cutoff() const
{
    return m_device ? toKnob(m_device->cutoff()) : 0;
}

void NoiseFxDialogModel::setCutoff(int value)
{
    if (m_device) {
        m_device->setCutoff(fromKnob(value));
    }
}

int NoiseFxDialogModel::resonance() const
{
    return m_device ? toKnob(m_device->resonance()) : 0;
}

void NoiseFxDialogModel::setResonance(int value)
{
    if (m_device) {
        m_device->setResonance(fromKnob(value));
    }
}

int NoiseFxDialogModel::drive() const
{
    return m_device ? toKnob(m_device->drive()) : 0;
}

void NoiseFxDialogModel::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(fromKnob(value));
    }
}

int NoiseFxDialogModel::keyTrack() const
{
    return m_device ? toKnob(m_device->keyTrack()) : 0;
}

void NoiseFxDialogModel::setKeyTrack(int value)
{
    if (m_device) {
        m_device->setKeyTrack(fromKnob(value));
    }
}

int NoiseFxDialogModel::lpfCutoff() const
{
    return m_device ? toKnob(m_device->lpfCutoff()) : 0;
}

void NoiseFxDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(fromKnob(value));
    }
}

int NoiseFxDialogModel::hpfCutoff() const
{
    return m_device ? toKnob(m_device->hpfCutoff()) : 0;
}

void NoiseFxDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(fromKnob(value));
    }
}

int NoiseFxDialogModel::ampAttack() const
{
    return m_device ? toKnob(m_device->ampAttack()) : 0;
}

void NoiseFxDialogModel::setAmpAttack(int value)
{
    if (m_device) {
        m_device->setAmpAttack(fromKnob(value));
    }
}

int NoiseFxDialogModel::ampDecay() const
{
    return m_device ? toKnob(m_device->ampDecay()) : 0;
}

void NoiseFxDialogModel::setAmpDecay(int value)
{
    if (m_device) {
        m_device->setAmpDecay(fromKnob(value));
    }
}

int NoiseFxDialogModel::ampSustain() const
{
    return m_device ? toKnob(m_device->ampSustain()) : 0;
}

void NoiseFxDialogModel::setAmpSustain(int value)
{
    if (m_device) {
        m_device->setAmpSustain(fromKnob(value));
    }
}

int NoiseFxDialogModel::ampRelease() const
{
    return m_device ? toKnob(m_device->ampRelease()) : 0;
}

void NoiseFxDialogModel::setAmpRelease(int value)
{
    if (m_device) {
        m_device->setAmpRelease(fromKnob(value));
    }
}

int NoiseFxDialogModel::velocitySensitivity() const
{
    return m_device ? toKnob(m_device->velocitySensitivity()) : 0;
}

void NoiseFxDialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(fromKnob(value));
    }
}

bool NoiseFxDialogModel::playToEnd() const
{
    return m_device && m_device->playToEnd();
}

void NoiseFxDialogModel::setPlayToEnd(bool value)
{
    if (m_device) {
        m_device->setPlayToEnd(value);
    }
}

bool NoiseFxDialogModel::chromatic() const
{
    return m_device && m_device->chromatic();
}

void NoiseFxDialogModel::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

int NoiseFxDialogModel::lfoWaveform() const
{
    return m_device ? static_cast<int>(std::lround(m_device->lfoParameterValue(NahdXml::xmlKeyWaveform()))) : 0;
}

void NoiseFxDialogModel::setLfoWaveform(int value)
{
    if (m_device) {
        m_device->setLfoParameterValue(NahdXml::xmlKeyWaveform(), static_cast<float>(value));
    }
}

bool NoiseFxDialogModel::lfoSync() const
{
    return m_device && m_device->lfoParameterValue(NahdXml::xmlKeySync()) > 0.5f;
}

void NoiseFxDialogModel::setLfoSync(bool value)
{
    if (m_device) {
        m_device->setLfoParameterValue(NahdXml::xmlKeySync(), value ? 1.0f : 0.0f);
    }
}

int NoiseFxDialogModel::lfoRate() const
{
    return m_device ? toKnob(m_device->lfoParameterValue(NahdXml::xmlKeyRate())) : 0;
}

void NoiseFxDialogModel::setLfoRate(int value)
{
    if (m_device) {
        m_device->setLfoParameterValue(NahdXml::xmlKeyRate(), fromKnob(value));
    }
}

int NoiseFxDialogModel::lfoTarget1() const
{
    return m_device ? static_cast<int>(m_device->lfoTarget(0)) : 0;
}

void NoiseFxDialogModel::setLfoTarget1(int value)
{
    if (m_device) {
        m_device->setLfoTarget(0, static_cast<NoiseFxDevice::Destination>(value));
    }
}

int NoiseFxDialogModel::lfoDepth1() const
{
    return m_device ? toKnob(m_device->lfoDepth(0)) : 0;
}

void NoiseFxDialogModel::setLfoDepth1(int value)
{
    if (m_device) {
        m_device->setLfoDepth(0, fromKnob(value));
    }
}

int NoiseFxDialogModel::lfoTarget2() const
{
    return m_device ? static_cast<int>(m_device->lfoTarget(1)) : 0;
}

void NoiseFxDialogModel::setLfoTarget2(int value)
{
    if (m_device) {
        m_device->setLfoTarget(1, static_cast<NoiseFxDevice::Destination>(value));
    }
}

int NoiseFxDialogModel::lfoDepth2() const
{
    return m_device ? toKnob(m_device->lfoDepth(1)) : 0;
}

void NoiseFxDialogModel::setLfoDepth2(int value)
{
    if (m_device) {
        m_device->setLfoDepth(1, fromKnob(value));
    }
}

int NoiseFxDialogModel::gateRate() const
{
    return m_device ? static_cast<int>(std::lround(m_device->gateParameterValue(NahdXml::xmlKeyRate()))) : 0;
}

void NoiseFxDialogModel::setGateRate(int value)
{
    if (m_device) {
        m_device->setGateParameterValue(NahdXml::xmlKeyRate(), static_cast<float>(value));
    }
}

int NoiseFxDialogModel::gateDepth() const
{
    return m_device ? toKnob(m_device->gateParameterValue(NahdXml::xmlKeyDepth())) : 0;
}

void NoiseFxDialogModel::setGateDepth(int value)
{
    if (m_device) {
        m_device->setGateParameterValue(NahdXml::xmlKeyDepth(), fromKnob(value));
    }
}

int NoiseFxDialogModel::gateWidth() const
{
    return m_device ? toKnob(m_device->gateParameterValue(NahdXml::xmlKeyWidth())) : 0;
}

void NoiseFxDialogModel::setGateWidth(int value)
{
    if (m_device) {
        m_device->setGateParameterValue(NahdXml::xmlKeyWidth(), fromKnob(value));
    }
}

int NoiseFxDialogModel::gateSmoothing() const
{
    return m_device ? toKnob(m_device->gateParameterValue(NahdXml::xmlKeySmoothing())) : 0;
}

void NoiseFxDialogModel::setGateSmoothing(int value)
{
    if (m_device) {
        m_device->setGateParameterValue(NahdXml::xmlKeySmoothing(), fromKnob(value));
    }
}

int NoiseFxDialogModel::lfoSyncDivision() const
{
    const auto last = static_cast<double>(NoiseFxDevice::lfoSyncBars().size() - 1);
    return m_device ? static_cast<int>(std::lround(m_device->lfoParameterValue(NahdXml::xmlKeyRate()) * last)) : 0;
}

void NoiseFxDialogModel::setLfoSyncDivision(int value)
{
    if (m_device) {
        const auto last = static_cast<float>(NoiseFxDevice::lfoSyncBars().size() - 1);
        m_device->setLfoParameterValue(NahdXml::xmlKeyRate(), static_cast<float>(value) / last);
    }
}

QVariantList NoiseFxDialogModel::sweeps() const
{
    QVariantList list;
    for (auto * sweep : m_sweeps) {
        list << QVariant::fromValue(static_cast<QObject *>(sweep));
    }
    return list;
}

QStringList NoiseFxDialogModel::syncLengthNames() const
{
    return noteValues(NoiseFxDevice::syncLengthBeats());
}

QStringList NoiseFxDialogModel::gateDivisionNames() const
{
    return noteValues(NoiseFxDevice::gateDivisionBeats());
}

QStringList NoiseFxDialogModel::lfoSyncNames() const
{
    std::vector<double> beats;
    for (auto && bars : NoiseFxDevice::lfoSyncBars()) {
        beats.push_back(bars * 4.0);
    }
    return noteValues(beats);
}

void NoiseFxDialogModel::requestSettings()
{
    emit colourChanged();
    emit sampleHoldRateChanged();
    emit crackleChanged();
    emit noiseLevelChanged();
    emit widthChanged();
    emit toneWaveformChanged();
    emit pitchChanged();
    emit blendChanged();
    emit ringModChanged();
    emit filterTypeChanged();
    emit cutoffChanged();
    emit resonanceChanged();
    emit driveChanged();
    emit keyTrackChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit ampAttackChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit velocitySensitivityChanged();
    emit playToEndChanged();
    emit chromaticChanged();
    emit lfoWaveformChanged();
    emit lfoSyncChanged();
    emit lfoRateChanged();
    emit lfoTarget1Changed();
    emit lfoDepth1Changed();
    emit lfoTarget2Changed();
    emit lfoDepth2Changed();
    emit gateRateChanged();
    emit gateDepthChanged();
    emit gateWidthChanged();
    emit gateSmoothingChanged();
    for (auto * sweep : m_sweeps) {
        emit sweep->changed();
    }
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
