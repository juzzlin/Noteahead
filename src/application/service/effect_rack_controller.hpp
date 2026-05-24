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

#ifndef EFFECT_RACK_CONTROLLER_HPP
#define EFFECT_RACK_CONTROLLER_HPP

#include "device_service.hpp"
#include "editor_service.hpp"

#include <QObject>

#include <functional>
#include <memory>
#include <optional>

namespace noteahead {

class EffectRackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int effectCount READ effectCount NOTIFY effectCountChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QString targetDeviceName READ targetDeviceName WRITE setTargetDeviceName NOTIFY targetDeviceNameChanged)
    Q_PROPERTY(bool isInsertRack READ isInsertRack WRITE setIsInsertRack NOTIFY isInsertRackChanged)

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);

    int effectCount() const;
    int revision() const;

    QString targetDeviceName() const;
    void setTargetDeviceName(const QString & name);

    bool isInsertRack() const;
    void setIsInsertRack(bool isInsert);

    Q_INVOKABLE float parameterValue(int effectIndex, const QString & paramName) const;
    Q_INVOKABLE void setParameterValue(int effectIndex, const QString & paramName, float value);

    Q_INVOKABLE void setEffect(int slotIndex, const QString & typeId);
    Q_INVOKABLE void clearEffect(int slotIndex);
    Q_INVOKABLE QVariantList availableEffects() const;

    Q_INVOKABLE QStringList parameterNames(int effectIndex) const;
    Q_INVOKABLE QString effectType(int effectIndex) const;
    Q_INVOKABLE QString effectParametersSummary(int effectIndex) const;

    Q_INVOKABLE QString reverbSizeKey() const;
    Q_INVOKABLE QString reverbDecayKey() const;
    Q_INVOKABLE QString reverbDampingKey() const;
    Q_INVOKABLE QString reverbPreDelayKey() const;
    Q_INVOKABLE QString reverbWidthKey() const;
    Q_INVOKABLE QString reverbMixKey() const;

    Q_INVOKABLE QString compressorThresholdKey() const;
    Q_INVOKABLE QString compressorRatioKey() const;
    Q_INVOKABLE QString compressorAttackKey() const;
    Q_INVOKABLE QString compressorReleaseKey() const;
    Q_INVOKABLE QString compressorKneeKey() const;
    Q_INVOKABLE QString compressorMakeupKey() const;
    Q_INVOKABLE QString compressorLookaheadKey() const;

    Q_INVOKABLE QString eq8BandParametricTypeKey(int bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricFreqKey(int bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricGainKey(int bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricQKey(int bandIndex) const;

    Q_INVOKABLE float compressorReductionDb(int effectIndex) const;

    Q_INVOKABLE QStringList reverbPresets() const;
    Q_INVOKABLE void applyReverbPreset(int effectIndex, int presetIndex);

    Q_INVOKABLE float deviceSend(const QString & deviceName, int effectIndex) const;
    Q_INVOKABLE void setDeviceSend(const QString & deviceName, int effectIndex, float send);

signals:
    void effectCountChanged();
    void revisionChanged();
    void targetDeviceNameChanged();
    void isInsertRackChanged();
    void parameterChanged(int effectIndex, const QString & paramName);

private:
    std::optional<std::reference_wrapper<EffectRack>> currentRack() const;

    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
    int m_revision = 0;
    QString m_targetDeviceName;
    bool m_isInsertRack = false;
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_HPP
