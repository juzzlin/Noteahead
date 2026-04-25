#include "sampler_controller.hpp"
#include "../models/sampler/sampler_pad_model.hpp"
#include "../../common/waveform_generator.hpp"
#include "../../domain/devices/sampler_device.hpp"

namespace noteahead {

SamplerController::SamplerController(std::shared_ptr<SamplerDevice> sampler, QObject * parent)
  : QObject { parent }
  , m_sampler { std::move(sampler) }
  , m_padModel { std::make_unique<SamplerPadModel>(m_sampler, this) }
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
    }
}

QVariantList SamplerController::getWaveformData(int numPoints)
{
    if (!m_sampler) {
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

} // namespace noteahead
