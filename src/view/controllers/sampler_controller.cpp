#include "sampler_controller.hpp"
#include "../../application/models/sampler/sampler_pad_model.hpp"
#include "../../application/note_converter.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/waveform_generator.hpp"
#include "../../domain/devices/sampler_device.hpp"

#include <QFileInfo>

#include <cmath>

namespace noteahead {

SamplerController::SamplerController(SamplerDevice::SamplerDeviceS sampler, QObject * parent)
  : DeviceController { parent }
  , m_sampler { std::move(sampler) }
  , m_padModel { std::make_unique<SamplerPadModel>(m_sampler, this) }
  , m_selectedPad { -1 }
{
    connectDeviceSignals();
}

SamplerController::~SamplerController() = default;

DeviceController::DeviceS SamplerController::device() const
{
    return m_sampler;
}

bool SamplerController::setDevice(DeviceS device)
{
    if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device)) {
        setSampler(sampler);
        return true;
    }
    return false;
}

SamplerPadModel * SamplerController::padModel() const
{
    return m_padModel.get();
}

SamplerDevice::SamplerDeviceS SamplerController::sampler() const
{
    return m_sampler;
}

void SamplerController::setSampler(SamplerDevice::SamplerDeviceS sampler)
{
    if (m_sampler != sampler) {
        if (m_sampler) {
            disconnect(m_sampler.get(), nullptr, this, nullptr);
        }
        m_sampler = std::move(sampler);
        connectDeviceSignals();
        m_padModel->setSampler(m_sampler);
        emit samplerChanged();
        // Refresh the global switches so they reflect the newly selected instance instead of
        // retaining the previous one's state.
        emit chromaticModeChanged();
        emit channelModeChanged();
        emit embedWaveDataChanged();
        setSelectedPad(m_selectedPad); // Trigger updates for properties
    }
}

int SamplerController::selectedPad() const
{
    return m_selectedPad;
}

void SamplerController::setSelectedPad(int selectedPad)
{
    if (m_selectedPad != selectedPad) {
        m_selectedPad = selectedPad;
        emit selectedPadChanged();
        emit playbackPositionChanged();
        emit isFinishedChanged();
        emit selectedPadPanChanged();
        emit selectedPadVolumeChanged();
        emit selectedPadCutoffChanged();
        emit selectedPadHpfCutoffChanged();
        emit selectedPadStartOffsetChanged();
        emit selectedPadEndOffsetChanged();
        emit selectedPadTuneChanged();
        emit selectedPadDetuneChanged();
        emit selectedPadAttackChanged();
        emit selectedPadDecayChanged();
        emit selectedPadSustainChanged();
        emit selectedPadReleaseChanged();
        emit selectedPadReverseChanged();
        emit selectedPadLoopChanged();
        emit selectedPadChokeGroupChanged();
        emit selectedPadDurationChanged();
    }
}

double SamplerController::playbackPosition() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return m_sampler->playbackPosition(static_cast<uint8_t>(noteForPad(m_selectedPad)));
}

bool SamplerController::isFinished() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return true;
    }
    return m_sampler->isFinished(static_cast<uint8_t>(noteForPad(m_selectedPad)));
}

double SamplerController::selectedPadPan() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.5;
    }
    return static_cast<double>(m_sampler->samplePan(static_cast<uint8_t>(noteForPad(m_selectedPad))));
}

void SamplerController::setSelectedPadPan(double pan)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSamplePan(static_cast<uint8_t>(noteForPad(m_selectedPad)), static_cast<float>(pan));
    }
}

double SamplerController::selectedPadVolume() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 1.0;
    }
    return static_cast<double>(m_sampler->sampleVolume(static_cast<uint8_t>(noteForPad(m_selectedPad))));
}

void SamplerController::setSelectedPadVolume(double volume)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleVolume(static_cast<uint8_t>(noteForPad(m_selectedPad)), static_cast<float>(volume));
    }
}

double SamplerController::selectedPadCutoff() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 1.0;
    }
    return static_cast<double>(m_sampler->sampleCutoff(static_cast<uint8_t>(noteForPad(m_selectedPad))));
}

void SamplerController::setSelectedPadCutoff(double cutoff)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleCutoff(static_cast<uint8_t>(noteForPad(m_selectedPad)), static_cast<float>(cutoff));
    }
}

double SamplerController::selectedPadHpfCutoff() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return static_cast<double>(m_sampler->sampleHpfCutoff(static_cast<uint8_t>(noteForPad(m_selectedPad))));
}

void SamplerController::setSelectedPadHpfCutoff(double cutoff)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleHpfCutoff(static_cast<uint8_t>(noteForPad(m_selectedPad)), static_cast<float>(cutoff));
    }
}

int SamplerController::selectedPadStartOffsetSeconds() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0;
    }
    return static_cast<int>(m_sampler->sampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad))));
}

void SamplerController::setSelectedPadStartOffsetSeconds(int seconds)
{
    if (m_sampler && m_selectedPad >= 0) {
        const double currentOffset = m_sampler->sampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad)));
        const double milliseconds = (currentOffset - std::floor(currentOffset)) * 1000.0;
        m_sampler->setSampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad)), static_cast<double>(seconds) + milliseconds / 1000.0);
    }
}

int SamplerController::selectedPadStartOffsetMilliseconds() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0;
    }
    const double offset = m_sampler->sampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad)));
    return static_cast<int>(std::round((offset - std::floor(offset)) * 1000.0));
}

void SamplerController::setSelectedPadStartOffsetMilliseconds(int milliseconds)
{
    if (m_sampler && m_selectedPad >= 0) {
        const double currentOffset = m_sampler->sampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad)));
        const double seconds = std::floor(currentOffset);
        m_sampler->setSampleStartOffset(static_cast<uint8_t>(noteForPad(m_selectedPad)), seconds + static_cast<double>(milliseconds) / 1000.0);
    }
}

std::optional<uint8_t> SamplerController::selectedNote() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(noteForPad(m_selectedPad));
}

bool SamplerController::selectedPadEndOffsetEnabled() const
{
    const auto note = selectedNote();
    return note && m_sampler->sampleEndOffset(*note).has_value();
}

void SamplerController::setSelectedPadEndOffsetEnabled(bool enabled)
{
    const auto note = selectedNote();
    if (!note) {
        return;
    }
    if (enabled == m_sampler->sampleEndOffset(*note).has_value()) {
        return;
    }
    // Switching the trim on with nothing set yet would leave it at second zero, which plays nothing.
    // The end of the sample is the only sensible place to start dragging it back from.
    m_sampler->setSampleEndOffset(*note, enabled ? std::optional<double> { m_sampler->sampleDuration(*note) } : std::nullopt);
    emit selectedPadEndOffsetChanged();
}

int SamplerController::selectedPadEndOffsetSeconds() const
{
    const auto note = selectedNote();
    if (!note) {
        return 0;
    }
    return static_cast<int>(std::floor(m_sampler->sampleEndOffset(*note).value_or(0.0)));
}

void SamplerController::setSelectedPadEndOffsetSeconds(int seconds)
{
    const auto note = selectedNote();
    if (!note) {
        return;
    }
    const double current = m_sampler->sampleEndOffset(*note).value_or(0.0);
    m_sampler->setSampleEndOffset(*note, static_cast<double>(seconds) + (current - std::floor(current)));
    emit selectedPadEndOffsetChanged();
}

int SamplerController::selectedPadEndOffsetMilliseconds() const
{
    const auto note = selectedNote();
    if (!note) {
        return 0;
    }
    const double offset = m_sampler->sampleEndOffset(*note).value_or(0.0);
    return static_cast<int>(std::round((offset - std::floor(offset)) * 1000.0));
}

void SamplerController::setSelectedPadEndOffsetMilliseconds(int milliseconds)
{
    const auto note = selectedNote();
    if (!note) {
        return;
    }
    const double current = m_sampler->sampleEndOffset(*note).value_or(0.0);
    m_sampler->setSampleEndOffset(*note, std::floor(current) + static_cast<double>(milliseconds) / 1000.0);
    emit selectedPadEndOffsetChanged();
}

double SamplerController::selectedPadTune() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleTune(*note)) : 0.5;
}

void SamplerController::setSelectedPadTune(double tune)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleTune(*note, static_cast<float>(tune));
        emit selectedPadTuneChanged();
    }
}

double SamplerController::selectedPadDetune() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleDetune(*note)) : 0.5;
}

void SamplerController::setSelectedPadDetune(double detune)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleDetune(*note, static_cast<float>(detune));
        emit selectedPadDetuneChanged();
    }
}

double SamplerController::selectedPadAttack() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleAttack(*note)) : 0.0;
}

void SamplerController::setSelectedPadAttack(double attack)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleAttack(*note, static_cast<float>(attack));
        emit selectedPadAttackChanged();
    }
}

double SamplerController::selectedPadDecay() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleDecay(*note)) : 0.0;
}

void SamplerController::setSelectedPadDecay(double decay)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleDecay(*note, static_cast<float>(decay));
        emit selectedPadDecayChanged();
    }
}

double SamplerController::selectedPadSustain() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleSustain(*note)) : 1.0;
}

void SamplerController::setSelectedPadSustain(double sustain)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleSustain(*note, static_cast<float>(sustain));
        emit selectedPadSustainChanged();
    }
}

double SamplerController::selectedPadRelease() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleRelease(*note)) : 0.0;
}

void SamplerController::setSelectedPadRelease(double release)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleRelease(*note, static_cast<float>(release));
        emit selectedPadReleaseChanged();
    }
}

bool SamplerController::selectedPadReverse() const
{
    const auto note = selectedNote();
    return note && m_sampler->sampleReverse(*note);
}

void SamplerController::setSelectedPadReverse(bool reverse)
{
    const auto note = selectedNote();
    if (note && m_sampler->sampleReverse(*note) != reverse) {
        m_sampler->setSampleReverse(*note, reverse);
        emit selectedPadReverseChanged();
    }
}

bool SamplerController::selectedPadLoop() const
{
    const auto note = selectedNote();
    return note && m_sampler->sampleLoop(*note);
}

void SamplerController::setSelectedPadLoop(bool loop)
{
    const auto note = selectedNote();
    if (note && m_sampler->sampleLoop(*note) != loop) {
        m_sampler->setSampleLoop(*note, loop);
        emit selectedPadLoopChanged();
    }
}

int SamplerController::selectedPadChokeGroup() const
{
    const auto note = selectedNote();
    return note ? m_sampler->sampleChokeGroup(*note) : 0;
}

void SamplerController::setSelectedPadChokeGroup(int group)
{
    const auto note = selectedNote();
    if (note && m_sampler->sampleChokeGroup(*note) != group) {
        m_sampler->setSampleChokeGroup(*note, group);
        emit selectedPadChokeGroupChanged();
    }
}

double SamplerController::selectedPadDuration() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return m_sampler->sampleDuration(static_cast<uint8_t>(noteForPad(m_selectedPad)));
}

bool SamplerController::channelMode() const
{
    if (!m_sampler) {
        return false;
    }
    return m_sampler->channelMode();
}

void SamplerController::setChannelMode(bool enabled)
{
    if (m_sampler && m_sampler->channelMode() != enabled) {
        m_sampler->setChannelMode(enabled);
        emit channelModeChanged();
    }
}

bool SamplerController::chromaticMode() const
{
    return m_sampler && m_sampler->chromaticMode();
}

void SamplerController::setChromaticMode(bool enabled)
{
    if (m_sampler && m_sampler->chromaticMode() != enabled) {
        m_sampler->setChromaticMode(enabled);
        emit chromaticModeChanged();
        // The pad-to-note mapping and labels change with the mode; refresh the selected-pad properties.
        emit selectedPadChanged();
        emit selectedPadDurationChanged();
    }
}

// Maps a pad index to a MIDI note. See SamplerDevice::noteForPad() for the two layouts.
int SamplerController::noteForPad(int padIndex) const
{
    // Without a device there is nothing to be in chromatic mode, so the drum layout is the sane default.
    return m_sampler ? m_sampler->noteForPad(padIndex) : SamplerDevice::padStartNote + padIndex;
}

bool SamplerController::embedWaveData() const
{
    return m_sampler && m_sampler->embedWaveData();
}

void SamplerController::setEmbedWaveData(bool enabled)
{
    if (m_sampler && m_sampler->embedWaveData() != enabled) {
        m_sampler->setEmbedWaveData(enabled);
        emit embedWaveDataChanged();
    }
}

QVariantList SamplerController::getWaveformData(int numPoints)
{
    if (!m_sampler || m_selectedPad < 0) {
        return {};
    }
    const int note = noteForPad(m_selectedPad);
    const auto filePath = QString::fromStdString(m_sampler->absoluteFilePath(static_cast<uint8_t>(note)));
    return WaveformGenerator::getWaveformData(filePath, numPoints);
}

void SamplerController::initialize()
{
    requestSettings();
}

void SamplerController::requestSettings()
{
    if (m_selectedPad < 0) {
        setSelectedPad(0);
    } else {
        emit selectedPadChanged();
        emit playbackPositionChanged();
        emit isFinishedChanged();
        emit selectedPadPanChanged();
        emit selectedPadVolumeChanged();
        emit selectedPadCutoffChanged();
        emit selectedPadHpfCutoffChanged();
        emit selectedPadStartOffsetChanged();
        emit selectedPadEndOffsetChanged();
        emit selectedPadTuneChanged();
        emit selectedPadDetuneChanged();
        emit selectedPadAttackChanged();
        emit selectedPadDecayChanged();
        emit selectedPadSustainChanged();
        emit selectedPadReleaseChanged();
        emit selectedPadReverseChanged();
        emit selectedPadLoopChanged();
        emit selectedPadChokeGroupChanged();
        emit selectedPadDurationChanged();
    }
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit channelModeChanged();
    emit embedWaveDataChanged();
    emit sampleRateChanged();
}

void SamplerController::loadSample(int padIndex, const QString & filePath)
{
    if (!m_sampler) {
        return;
    }
    const int note = noteForPad(padIndex);
    m_sampler->loadSample(static_cast<uint8_t>(note), filePath.toStdString());
    m_padModel->updatePad(padIndex);
    if (padIndex == m_selectedPad) {
        emit selectedPadDurationChanged();
    }
}

void SamplerController::clearSample(int padIndex)
{
    if (!m_sampler) {
        return;
    }
    const int note = noteForPad(padIndex);
    m_sampler->clearSample(static_cast<uint8_t>(note));
    m_padModel->updatePad(padIndex);
}

void SamplerController::copyPad(int sourcePad, int targetPad)
{
    if (!m_sampler || sourcePad == targetPad) {
        return;
    }
    // The device's dataChanged is wired to requestSettings(), which re-reads the whole selected pad,
    // so the pad settings of a copy landing under the cursor refresh on their own.
    m_sampler->copySample(static_cast<uint8_t>(noteForPad(sourcePad)), static_cast<uint8_t>(noteForPad(targetPad)));
    m_padModel->updatePad(targetPad);
}

QVariantList SamplerController::loadedPads() const
{
    QVariantList list;
    if (!m_sampler) {
        return list;
    }
    for (int padIndex = 0; padIndex < m_padModel->rowCount(); padIndex++) {
        const auto note = noteForPad(padIndex);
        if (note >= static_cast<int>(SamplerDevice::maxSamples)) {
            continue;
        }
        if (const auto sample = m_sampler->sample(static_cast<uint8_t>(note)); sample) {
            QVariantMap map;
            map["padIndex"] = padIndex;
            map["note"] = note;
            map["noteName"] = QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(note)));
            map["fileName"] = QFileInfo { QString::fromStdString(sample->filePath) }.fileName();
            list.append(map);
        }
    }
    return list;
}

void SamplerController::playSample(int padIndex, double velocity)
{
    if (!m_sampler) {
        return;
    }
    const int note = noteForPad(padIndex);
    m_sampler->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
}

void SamplerController::stopSample(int padIndex)
{
    if (!m_sampler) {
        return;
    }
    const int note = noteForPad(padIndex);
    m_sampler->processMidiNoteOff(static_cast<uint8_t>(note));
}

void SamplerController::updatePlaybackStatus()
{
    emit playbackPositionChanged();
    emit isFinishedChanged();
}

} // namespace noteahead
