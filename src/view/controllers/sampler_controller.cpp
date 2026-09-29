#include "sampler_controller.hpp"

#include "../../application/models/sampler/sampler_pad_model.hpp"
#include "../../application/note_converter.hpp"
#include "../../application/service/audio_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/waveform_generator.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include <QDateTime>
#include <QDir>
#include <QFile>

#include <QFileInfo>

#include <algorithm>
#include <cmath>

namespace noteahead {

static const auto TAG = "SamplerController";

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
        emit lpfSlopeChanged();
        emit hpfSlopeChanged();
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
        emit selectedPadLoopStartChanged();
        emit selectedPadTuneChanged();
        emit selectedPadDetuneChanged();
        emit selectedPadAttackChanged();
        emit selectedPadHoldChanged();
        emit selectedPadDecayChanged();
        emit selectedPadSustainChanged();
        emit selectedPadReleaseChanged();
        emit selectedPadCurveChanged();
        emit selectedPadReverseChanged();
        emit selectedPadNormalizeChanged();
        emit selectedPadLoopChanged();
        emit selectedPadMonoChanged();
        emit selectedPadChokeGroupChanged();
        emit selectedPadDurationChanged();
    }
}

double SamplerController::playbackPosition() const
{
    const auto note = selectedNote();
    if (!note) {
        return 0.0;
    }
    const auto position = m_sampler->playbackPosition(*note);
    // The view draws a reversed pad back to front, so the playhead has to be mirrored with it or it
    // would run the wrong way across the waveform the user is looking at.
    return m_sampler->sampleReverse(*note) ? 1.0 - position : position;
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

SamplerController::OffsetParts SamplerController::splitSeconds(double seconds)
{
    // Rounded to whole milliseconds before it is split, not floored: an offset is stored as a
    // fraction of a minute in a float, which does not land exactly on the second it was set to, and
    // flooring a value a hair short of one reads it as no seconds and a thousand milliseconds.
    const auto milliseconds = std::llround(std::max(0.0, seconds) * 1000.0);
    return { static_cast<int>(milliseconds / 1000), static_cast<int>(milliseconds % 1000) };
}

int SamplerController::selectedPadStartOffsetSeconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleStartOffset(*note)).seconds : 0;
}

void SamplerController::setSelectedPadStartOffsetSeconds(int seconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleStartOffset(*note));
        m_sampler->setSampleStartOffset(*note, seconds + current.milliseconds / 1000.0);
        // The device clamps the offset to the length of the sample, so what it holds afterwards is
        // not necessarily what was asked for. Saying so is what lets the field snap back to it.
        emit selectedPadStartOffsetChanged();
    }
}

int SamplerController::selectedPadStartOffsetMilliseconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleStartOffset(*note)).milliseconds : 0;
}

void SamplerController::setSelectedPadStartOffsetMilliseconds(int milliseconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleStartOffset(*note));
        m_sampler->setSampleStartOffset(*note, current.seconds + milliseconds / 1000.0);
        emit selectedPadStartOffsetChanged();
    }
}

std::optional<uint8_t> SamplerController::selectedNote() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return std::nullopt;
    }
    return static_cast<uint8_t>(noteForPad(m_selectedPad));
}

int SamplerController::selectedPadEndOffsetSeconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleEndOffset(*note)).seconds : 0;
}

void SamplerController::setSelectedPadEndOffsetSeconds(int seconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleEndOffset(*note));
        m_sampler->setSampleEndOffset(*note, seconds + current.milliseconds / 1000.0);
        emit selectedPadEndOffsetChanged();
    }
}

int SamplerController::selectedPadEndOffsetMilliseconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleEndOffset(*note)).milliseconds : 0;
}

void SamplerController::setSelectedPadEndOffsetMilliseconds(int milliseconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleEndOffset(*note));
        m_sampler->setSampleEndOffset(*note, current.seconds + milliseconds / 1000.0);
        emit selectedPadEndOffsetChanged();
    }
}

int SamplerController::selectedPadLoopStartSeconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleLoopStart(*note)).seconds : 0;
}

void SamplerController::setSelectedPadLoopStartSeconds(int seconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleLoopStart(*note));
        m_sampler->setSampleLoopStart(*note, seconds + current.milliseconds / 1000.0);
        emit selectedPadLoopStartChanged();
    }
}

int SamplerController::selectedPadLoopStartMilliseconds() const
{
    const auto note = selectedNote();
    return note ? splitSeconds(m_sampler->sampleLoopStart(*note)).milliseconds : 0;
}

void SamplerController::setSelectedPadLoopStartMilliseconds(int milliseconds)
{
    if (const auto note = selectedNote(); note) {
        const auto current = splitSeconds(m_sampler->sampleLoopStart(*note));
        m_sampler->setSampleLoopStart(*note, current.seconds + milliseconds / 1000.0);
        emit selectedPadLoopStartChanged();
    }
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

double SamplerController::selectedPadHold() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleHold(*note)) : 0.0;
}

void SamplerController::setSelectedPadHold(double hold)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleHold(*note, static_cast<float>(hold));
        emit selectedPadHoldChanged();
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

double SamplerController::selectedPadCurve() const
{
    const auto note = selectedNote();
    return note ? static_cast<double>(m_sampler->sampleCurve(*note)) : 0.0;
}

void SamplerController::setSelectedPadCurve(double curve)
{
    if (const auto note = selectedNote(); note) {
        m_sampler->setSampleCurve(*note, static_cast<float>(curve));
        emit selectedPadCurveChanged();
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

double SamplerController::selectedPadAttackSeconds() const
{
    const auto note = selectedNote();
    return note ? SamplerDevice::attackSeconds(m_sampler->sampleAttack(*note)) : 0.0;
}

double SamplerController::selectedPadHoldSeconds() const
{
    const auto note = selectedNote();
    return note ? SamplerDevice::holdSeconds(m_sampler->sampleHold(*note)) : 0.0;
}

double SamplerController::selectedPadDecaySeconds() const
{
    const auto note = selectedNote();
    return note ? SamplerDevice::decaySeconds(m_sampler->sampleDecay(*note)) : 0.0;
}

double SamplerController::selectedPadReleaseSeconds() const
{
    const auto note = selectedNote();
    return note ? SamplerDevice::releaseSeconds(m_sampler->sampleRelease(*note)) : 0.0;
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
        emit selectedPadNormalizeChanged();
    }
}

bool SamplerController::selectedPadNormalize() const
{
    const auto note = selectedNote();
    return note && m_sampler->sampleNormalize(*note);
}

void SamplerController::setSelectedPadNormalize(bool normalize)
{
    const auto note = selectedNote();
    if (note && m_sampler->sampleNormalize(*note) != normalize) {
        m_sampler->setSampleNormalize(*note, normalize);
        emit selectedPadNormalizeChanged();
    }
}

bool SamplerController::selectedPadMono() const
{
    const auto note = selectedNote();
    return note && m_sampler->sampleMono(*note);
}

void SamplerController::setSelectedPadMono(bool mono)
{
    const auto note = selectedNote();
    if (note && m_sampler->sampleMono(*note) != mono) {
        m_sampler->setSampleMono(*note, mono);
        emit selectedPadMonoChanged();
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
        // A loop point at the beginning of the range sits underneath the start marker, where it can
        // be neither seen nor taken hold of. Turning looping on drops it in the middle of the range
        // instead, which is somewhere to drag it from. A point the pad already carries is its own.
        if (loop && m_sampler->sampleLoopStart(*note) <= 0.0) {
            const auto range = m_sampler->sampleDuration(*note) - m_sampler->sampleStartOffset(*note) - m_sampler->sampleEndOffset(*note);
            if (range > 0.0) {
                m_sampler->setSampleLoopStart(*note, range / 2.0);
                emit selectedPadLoopStartChanged();
            }
        }
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

double SamplerController::selectedPadAudibleLength() const
{
    if (!m_sampler || m_selectedPad < 0) {
        return 0.0;
    }
    return m_sampler->sampleAudibleLength(static_cast<uint8_t>(noteForPad(m_selectedPad)));
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

int SamplerController::lpfSlope() const
{
    return m_sampler ? m_sampler->lpfSlope() : 0;
}

void SamplerController::setLpfSlope(int value)
{
    if (m_sampler) {
        m_sampler->setLpfSlope(value);
        emit lpfSlopeChanged();
    }
}

int SamplerController::hpfSlope() const
{
    return m_sampler ? m_sampler->hpfSlope() : 0;
}

void SamplerController::setHpfSlope(int value)
{
    if (m_sampler) {
        m_sampler->setHpfSlope(value);
        emit hpfSlopeChanged();
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
    auto data = WaveformGenerator::getWaveformData(filePath, numPoints);
    // A reversed pad is drawn back to front, so that the offsets, which are read against the waveform
    // as it sounds, line up with the parts of it they actually trim.
    if (m_sampler->sampleReverse(static_cast<uint8_t>(note))) {
        std::reverse(data.begin(), data.end());
    }
    // Drawn at the gain it plays at, so a normalised pad does not look quieter than it sounds. The
    // picture is clamped like the peaks themselves are, since the view draws 0..1.
    if (const auto gain = m_sampler->sampleNormalizeGain(static_cast<uint8_t>(note)); gain != 1.0f) {
        for (auto && point : data) {
            point = std::min(1.0, point.toDouble() * static_cast<double>(gain));
        }
    }
    return data;
}

void SamplerController::setAudioService(AudioServiceS audioService)
{
    if (m_audioService) {
        disconnect(m_audioService.get(), &AudioService::recordingFinished, this, nullptr);
    }
    m_audioService = std::move(audioService);
    if (m_audioService) {
        connect(m_audioService.get(), &AudioService::recordingFinished, this, [this](QString filePath) {
            onRecordingFinished(filePath);
        });
    }
}

bool SamplerController::recording() const
{
    return m_recordingPad.has_value();
}

void SamplerController::startRecording()
{
    if (!m_audioService || !m_sampler || recording()) {
        return;
    }
    const auto note = selectedNote();
    if (!note) {
        return;
    }

    const auto project = QString::fromStdString(m_sampler->projectPath());
    auto target = project.isEmpty() ? QString {} : QDir { project }.absoluteFilePath("samples");
    m_recordingIsEphemeral = target.isEmpty();
    if (m_recordingIsEphemeral) {
        // Nowhere of its own to live yet. Recording into a directory of this session's own is far
        // better than refusing to record at all, which is what the song recorder does -- the pad is
        // marked so that saving the project writes it out.
        if (!m_recordingDirectory) {
            m_recordingDirectory = std::make_unique<QTemporaryDir>();
        }
        if (!m_recordingDirectory->isValid()) {
            juzzlin::L(TAG).error() << "Cannot create a directory to record into";
            return;
        }
        target = m_recordingDirectory->path();
    } else if (!QDir {}.mkpath(target)) {
        juzzlin::L(TAG).error() << "Cannot create " << std::quoted(target.toStdString());
        return;
    }

    const auto fileName = QString { "pad%1_%2.wav" }
                            .arg(m_selectedPad + 1, 2, 10, QChar { '0' })
                            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const auto filePath = QDir { target }.absoluteFilePath(fileName);

    m_recordingPad = m_selectedPad;
    juzzlin::L(TAG).info() << "Recording pad " << (m_selectedPad + 1) << " into " << std::quoted(filePath.toStdString());
    m_audioService->startRecording(filePath, 0, 0);
    emit recordingChanged();
}

void SamplerController::stopRecording()
{
    if (!m_audioService || !recording()) {
        return;
    }
    // The pad is loaded from onRecordingFinished() rather than here: this only asks the worker to
    // stop, and the file is still open until it says it has.
    m_audioService->stopRecording(0);
}

void SamplerController::onRecordingFinished(const QString & filePath)
{
    if (!m_recordingPad || !m_sampler) {
        return;
    }
    const auto pad = *m_recordingPad;
    m_recordingPad.reset();
    emit recordingChanged();

    if (!QFile::exists(filePath)) {
        juzzlin::L(TAG).error() << "Nothing was recorded to " << std::quoted(filePath.toStdString());
        return;
    }

    const auto note = static_cast<uint8_t>(noteForPad(pad));
    try {
        m_sampler->loadSample(note, filePath.toStdString());
    } catch (const std::exception & e) {
        juzzlin::L(TAG).error() << "Loading the recording failed: " << e.what();
        return;
    }
    if (m_recordingIsEphemeral) {
        m_sampler->markSampleEphemeral(note);
    }

    setSelectedPad(pad);
    emit selectedPadChanged();
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
        emit selectedPadLoopStartChanged();
        emit selectedPadTuneChanged();
        emit selectedPadDetuneChanged();
        emit selectedPadAttackChanged();
        emit selectedPadDecayChanged();
        emit selectedPadSustainChanged();
        emit selectedPadReleaseChanged();
        emit selectedPadReverseChanged();
        emit selectedPadNormalizeChanged();
        emit selectedPadLoopChanged();
        emit selectedPadMonoChanged();
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

void SamplerController::autoTrimPad(int padIndex)
{
    if (!m_sampler) {
        return;
    }
    if (m_sampler->autoTrimSample(static_cast<uint8_t>(noteForPad(padIndex)))) {
        emit selectedPadStartOffsetChanged();
        emit selectedPadEndOffsetChanged();
        emit selectedPadDurationChanged();
    }
}

QVariantMap SamplerController::meterLevels() const
{
    // Recording is the one time the input is the interesting signal: the pads are silent and the
    // question is whether what is arriving is loud enough to keep.
    if (recording() && m_audioService) {
        if (const auto levels = m_audioService->inputLevels(); !levels.isEmpty()) {
            return levels;
        }
    }
    if (!m_sampler) {
        return {};
    }
    const auto & meter = m_sampler->outputStereoMeter();
    return {
        { "leftPeakDb", meter.leftPeakDb() },
        { "leftRmsDb", meter.leftRmsDb() },
        { "rightPeakDb", meter.rightPeakDb() },
        { "rightRmsDb", meter.rightRmsDb() }
    };
}

void SamplerController::setMetersActive(bool active)
{
    if (m_sampler) {
        m_sampler->outputStereoMeter().setActive(active);
    }
    if (m_audioService) {
        m_audioService->setInputMeterActive(active);
    }
}

int SamplerController::padNote(int padIndex) const
{
    return m_sampler ? noteForPad(padIndex) : -1;
}

bool SamplerController::setPadNote(int padIndex, int midiNote)
{
    if (!m_sampler || !m_sampler->setPadNote(padIndex, midiNote)) {
        return false;
    }
    // Moving one pad moves the boundary it shares with its neighbours, so every tile's range label
    // can have changed, not just this one's.
    m_padModel->updateAllPads();
    emit selectedPadChanged();
    return true;
}

QString SamplerController::noteName(int midiNote) const
{
    if (midiNote < 0 || midiNote >= static_cast<int>(SamplerDevice::maxSamples)) {
        return {};
    }
    return QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(midiNote)));
}

void SamplerController::cropPadToTrim(int padIndex)
{
    if (!m_sampler) {
        return;
    }
    // Beside the project when there is one, so the cropped file is somewhere the project can keep
    // pointing at. With no project it lands beside whatever it was cropped from.
    const auto project = QString::fromStdString(m_sampler->projectPath());
    const auto target = project.isEmpty() ? QString {} : QDir { project }.absoluteFilePath("samples");
    if (m_sampler->cropSampleToTrim(static_cast<uint8_t>(noteForPad(padIndex)), target)) {
        if (project.isEmpty()) {
            // Nowhere of its own yet, so saving the project has to write it out.
            m_sampler->markSampleEphemeral(static_cast<uint8_t>(noteForPad(padIndex)));
        }
        setSelectedPad(padIndex);
        emit selectedPadChanged();
    }
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
