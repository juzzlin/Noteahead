#include "sampler_controller.hpp"
#include "../models/sampler/sampler_pad_model.hpp"
#include "../../common/waveform_generator.hpp"
#include "../../domain/devices/sampler_device.hpp"

namespace noteahead {

SamplerController::SamplerController(std::shared_ptr<SamplerDevice> sampler, QObject * parent)
  : QObject { parent }
  , m_sampler { std::move(sampler) }
  , m_padModel { std::make_unique<SamplerPadModel>(m_sampler, this) }
  , m_selectedPad { -1 }
{
}

SamplerController::~SamplerController() = default;

SamplerPadModel * SamplerController::padModel() const
{
    return m_padModel.get();
}

std::shared_ptr<SamplerDevice> SamplerController::sampler() const
{
    return m_sampler;
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
        emit selectedPadDurationChanged();
    }
}

double SamplerController::playbackPosition() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return m_sampler->playbackPosition(static_cast<uint8_t>(36 + m_selectedPad));
}

bool SamplerController::isFinished() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return true;
    }
    return m_sampler->isFinished(static_cast<uint8_t>(36 + m_selectedPad));
}

double SamplerController::selectedPadPan() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.5;
    }
    return static_cast<double>(m_sampler->samplePan(static_cast<uint8_t>(36 + m_selectedPad)));
}

void SamplerController::setSelectedPadPan(double pan)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSamplePan(static_cast<uint8_t>(36 + m_selectedPad), static_cast<float>(pan));
        emit selectedPadPanChanged();
    }
}

double SamplerController::selectedPadVolume() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 1.0;
    }
    return static_cast<double>(m_sampler->sampleVolume(static_cast<uint8_t>(36 + m_selectedPad)));
}

void SamplerController::setSelectedPadVolume(double volume)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleVolume(static_cast<uint8_t>(36 + m_selectedPad), static_cast<float>(volume));
        emit selectedPadVolumeChanged();
    }
}

double SamplerController::selectedPadCutoff() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 1.0;
    }
    return static_cast<double>(m_sampler->sampleCutoff(static_cast<uint8_t>(36 + m_selectedPad)));
}

void SamplerController::setSelectedPadCutoff(double cutoff)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleCutoff(static_cast<uint8_t>(36 + m_selectedPad), static_cast<float>(cutoff));
        emit selectedPadCutoffChanged();
    }
}

double SamplerController::selectedPadHpfCutoff() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return static_cast<double>(m_sampler->sampleHpfCutoff(static_cast<uint8_t>(36 + m_selectedPad)));
}

void SamplerController::setSelectedPadHpfCutoff(double cutoff)
{
    if (m_sampler && m_selectedPad >= 0) {
        m_sampler->setSampleHpfCutoff(static_cast<uint8_t>(36 + m_selectedPad), static_cast<float>(cutoff));
        emit selectedPadHpfCutoffChanged();
    }
}

int SamplerController::selectedPadStartOffsetSeconds() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0;
    }
    return static_cast<int>(m_sampler->sampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad)));
}

void SamplerController::setSelectedPadStartOffsetSeconds(int seconds)
{
    if (m_sampler && m_selectedPad >= 0) {
        const double currentOffset = m_sampler->sampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad));
        const double milliseconds = (currentOffset - std::floor(currentOffset)) * 1000.0;
        m_sampler->setSampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad), static_cast<double>(seconds) + milliseconds / 1000.0);
        emit selectedPadStartOffsetChanged();
    }
}

int SamplerController::selectedPadStartOffsetMilliseconds() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0;
    }
    const double offset = m_sampler->sampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad));
    return static_cast<int>(std::round((offset - std::floor(offset)) * 1000.0));
}

void SamplerController::setSelectedPadStartOffsetMilliseconds(int milliseconds)
{
    if (m_sampler && m_selectedPad >= 0) {
        const double currentOffset = m_sampler->sampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad));
        const double seconds = std::floor(currentOffset);
        m_sampler->setSampleStartOffset(static_cast<uint8_t>(36 + m_selectedPad), seconds + static_cast<double>(milliseconds) / 1000.0);
        emit selectedPadStartOffsetChanged();
    }
}

double SamplerController::selectedPadDuration() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return m_sampler->sampleDuration(static_cast<uint8_t>(36 + m_selectedPad));
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
    if (m_sampler) {
        m_sampler->setChannelMode(enabled);
        emit channelModeChanged();
    }
}

QVariantList SamplerController::getWaveformData(int numPoints)
{
    if (!m_sampler || m_selectedPad < 0) {
        return {};
    }
    const int note = 36 + m_selectedPad;
    const auto filePath = QString::fromStdString(m_sampler->absoluteFilePath(static_cast<uint8_t>(note)));
    return WaveformGenerator::getWaveformData(filePath, numPoints);
}

void SamplerController::initialize()
{
    if (m_sampler) {
        m_sampler->saveState();
    }
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
        emit selectedPadDurationChanged();
    }
    emit channelModeChanged();
}

void SamplerController::accept()
{
    // State already updated in domain, nothing to do but close dialog which is handled by QML
}

void SamplerController::reject()
{
    if (m_sampler) {
        m_sampler->restoreState();
    }
}

void SamplerController::loadSample(int padIndex, const QString & filePath)
{
    if (!m_sampler) {
        return;
    }
    const int note = 36 + padIndex;
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
    const int note = 36 + padIndex;
    m_sampler->clearSample(static_cast<uint8_t>(note));
    m_padModel->updatePad(padIndex);
}

void SamplerController::playSample(int padIndex, double velocity)
{
    if (!m_sampler) {
        return;
    }
    const int note = 36 + padIndex;
    m_sampler->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
}

void SamplerController::stopSample(int padIndex)
{
    if (!m_sampler) {
        return;
    }
    const int note = 36 + padIndex;
    m_sampler->processMidiNoteOff(static_cast<uint8_t>(note));
}

void SamplerController::updatePlaybackStatus()
{
    emit playbackPositionChanged();
    emit isFinishedChanged();
}

} // namespace noteahead
