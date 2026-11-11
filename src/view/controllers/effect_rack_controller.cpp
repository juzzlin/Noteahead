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

#include "effect_rack_controller.hpp"

#include "../plugins/plugin_catalog.hpp"

#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/dsp/lfo.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/spectrum_source.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../domain/utility/rta_core.hpp"
#include "../../domain/utility/spectrum_taps.hpp"
#include "../../domain/utility/stereo_field_meter.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"
#include "../../plugins/devices/sampler/sampler_device.hpp"
#include "../../plugins/effects/air_band_eq/air_band_eq.hpp"
#include "../../plugins/effects/all_pass_filter/all_pass_filter.hpp"
#include "../../plugins/effects/ambience/ambience.hpp"
#include "../../plugins/effects/analog_fuzz/analog_fuzz.hpp"
#include "../../plugins/effects/auto_ducker/auto_ducker.hpp"
#include "../../plugins/effects/auto_filter/auto_filter.hpp"
#include "../../plugins/effects/auto_panner/auto_panner.hpp"
#include "../../plugins/effects/bass_grinder/bass_grinder.hpp"
#include "../../plugins/effects/bit_crusher/bit_crusher.hpp"
#include "../../plugins/effects/chorus/chorus.hpp"
#include "../../plugins/effects/clipper/clipper.hpp"
#include "../../plugins/effects/compressor/compressor.hpp"
#include "../../plugins/effects/compressor_2a/compressor_2a.hpp"
#include "../../plugins/effects/crossfeed/crossfeed.hpp"
#include "../../plugins/effects/delay/delay.hpp"
#include "../../plugins/effects/dimension/dimension.hpp"
#include "../../plugins/effects/doubler/doubler.hpp"
#include "../../plugins/effects/drive/drive.hpp"
#include "../../plugins/effects/dynamic_eq/dynamic_eq.hpp"
#include "../../plugins/effects/early_reflections/early_reflections.hpp"
#include "../../plugins/effects/eq_32_band_graphical/eq_32_band_graphical.hpp"
#include "../../plugins/effects/eq_8_band_parametric/eq_8_band_parametric.hpp"
#include "../../plugins/effects/flanger/flanger.hpp"
#include "../../plugins/effects/gain/gain.hpp"
#include "../../plugins/effects/gate/gate.hpp"
#include "../../plugins/effects/limiter/limiter.hpp"
#include "../../plugins/effects/monitor/monitor.hpp"
#include "../../plugins/effects/multiband_compressor/multiband_compressor.hpp"
#include "../../plugins/effects/panner/panner.hpp"
#include "../../plugins/effects/phase_scope/phase_scope.hpp"
#include "../../plugins/effects/phaser/phaser.hpp"
#include "../../plugins/effects/reference/reference.hpp"
#include "../../plugins/effects/reference/reference_environments.hpp"
#include "../../plugins/effects/reverb_224/reverb_224.hpp"
#include "../../plugins/effects/saturator/saturator.hpp"
#include "../../plugins/effects/simple_eq/simple_eq.hpp"
#include "../../plugins/effects/stereo_enhancer/stereo_enhancer.hpp"
#include "../../plugins/effects/stereo_exciter/stereo_exciter.hpp"
#include "../../plugins/effects/stereo_widener/stereo_widener.hpp"
#include "../../plugins/effects/tremolo/tremolo.hpp"
#include "../../plugins/effects/tube_stage/tube_stage.hpp"
#include "../../plugins/effects/vintage_passive_eq/vintage_passive_eq.hpp"
#include "../../plugins/effects/wave_designer/wave_designer.hpp"
#include "../qml/Dialogs/eq_curve_renderer.hpp"
#include "../qml/Dialogs/rta_renderer.hpp"
#include "knob_controller.hpp"

#include <QDateTime>
#include <QFile>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <ranges>

namespace noteahead {

static constexpr float dbtpFloor = -70.0f;

namespace {

//! One meter reading for the rack listing, padded so that a value crossing -10 does not shove
//! everything after it along the row. An ordinary space would not do it: the row is set in a
//! proportional font, where a space is much narrower than a digit, so a constant character count
//! still comes out a different width. U+2007 FIGURE SPACE is defined to be exactly as wide as a
//! digit, which lines the fields up without the row having to become monospaced.
QString formatMeterReading(float value)
{
    static constexpr int fieldWidth = 5; // "-70.0", the widest reading either meter can produce
    static const QChar figureSpace { 0x2007 };
    return value <= dbtpFloor ? QString { "-∞" }.rightJustified(fieldWidth, figureSpace) : QString { "%1" }.arg(value, fieldWidth, 'f', 1, figureSpace);
}

} // namespace

EffectRackController::EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, [this]() {
            m_revision++;
            emit revisionChanged();
            emit effectCountChanged();
        });
        connect(m_deviceService.get(), &DeviceService::parameterAutomationScheduled, this, &EffectRackController::handleParameterAutomation);
    }

    connect(&m_automationRefresh, &AutomationRefreshThrottle::refreshDue, this, [this]() {
        m_revision++;
        emit revisionChanged();
    });
}

bool EffectRackController::isInCurrentRack(quintptr target) const
{
    if (target == 0) {
        return true;
    }
    if (const auto rack = currentRack(); rack) {
        for (size_t i = 0; i < rack->get().effectCount(); i++) {
            const auto effect = rack->get().effect(i);
            if (effect && reinterpret_cast<quintptr>(static_cast<ParameterAutomationTarget *>(effect.get())) == target) {
                return true;
            }
        }
    }
    return false;
}

void EffectRackController::handleParameterAutomation(quintptr target, qint64 delayMs)
{
    if (isInCurrentRack(target)) {
        m_automationRefresh.schedule(delayMs);
    }
}

int EffectRackController::effectCount() const
{
    if (const auto rack = currentRack()) {
        return static_cast<int>(rack->get().effectCount());
    }
    return 0;
}

int EffectRackController::sendChainEffectCount(int busIndex) const
{
    if (busIndex < 0 || static_cast<size_t>(busIndex) >= m_deviceService->sendChainRackCount()) {
        return 0;
    }
    const auto effects = m_deviceService->sendChainRack(static_cast<size_t>(busIndex)).effects();
    return static_cast<int>(std::ranges::count_if(effects, [](const auto & effect) { return effect != nullptr; }));
}

int EffectRackController::revision() const
{
    return m_revision;
}

void EffectRackController::retranslate()
{
    m_revision++;
    emit revisionChanged();
}

QString EffectRackController::targetDeviceName() const
{
    return m_targetDeviceName;
}

void EffectRackController::setTargetDeviceName(const QString & name)
{
    m_targetDeviceName = name;
    emit targetDeviceNameChanged();
    emit rackEnabledChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

bool EffectRackController::isInsertRack() const
{
    return m_isInsertRack;
}

void EffectRackController::setIsInsertRack(bool isInsert)
{
    m_isInsertRack = isInsert;
    emit isInsertRackChanged();
    emit rackEnabledChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

int EffectRackController::targetSubIndex() const
{
    return m_targetSubIndex;
}

void EffectRackController::setTargetSubIndex(int index)
{
    m_targetSubIndex = index;
    emit targetSubIndexChanged();
    emit rackEnabledChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

EffectRackController::EffectRackOpt EffectRackController::rackAt(const QString & deviceName, bool isInsertRack, int subIndex) const
{
    if (deviceName.isEmpty()) {
        if (isInsertRack) {
            return std::ref(m_deviceService->insertEffectRack());
        } else if (subIndex >= 0) {
            // On the master send rack a sub-index is a send bus, and addresses the chain of effects
            // that bus runs after its own -- the same shape as a Sampler pad or a Drum Synth voice
            // below, so everything downstream of here works on a chain without knowing about them.
            if (static_cast<size_t>(subIndex) >= m_deviceService->sendChainRackCount()) {
                return std::nullopt;
            }
            return std::ref(m_deviceService->sendChainRack(static_cast<size_t>(subIndex)));
        } else {
            return std::ref(m_deviceService->sendEffectRack());
        }
    } else {
        if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
            if (subIndex >= 0) {
                return device->partEffectRack(static_cast<size_t>(subIndex));
            }
            return std::ref(device->insertEffectRack());
        }
    }
    return std::nullopt;
}

bool EffectRackController::isMasterSendRack() const
{
    return m_targetDeviceName.isEmpty() && !m_isInsertRack && m_targetSubIndex < 0;
}

EffectRackController::EffectRackOpt EffectRackController::currentRack() const
{
    return rackAt(m_targetDeviceName, m_isInsertRack, m_targetSubIndex);
}

float EffectRackController::parameterValue(quint32 effectIndex, const QString & paramName) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                return parameter->get().shownValue();
            }
        }
    }
    return 0.0f;
}

int EffectRackController::parameterMaximum(quint32 effectIndex, const QString & paramName) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                return parameter->get().xmlMax();
            }
        }
    }
    // Not there yet, as for a dialog bound before its slot is: one keeps a range valid.
    return 1;
}

void EffectRackController::setParameterValue(quint32 effectIndex, const QString & paramName, float value)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                if (parameter->get().update(value)) {
                    effect->sync();
                    m_editorService->setIsModified(true);
                    m_revision++;
                    emit revisionChanged();
                    emit parameterChanged(effectIndex, paramName);
                }
            }
        }
    }
}

void EffectRackController::setEffect(int slotIndex, const QString & typeId)
{
    if (const auto effect = EffectFactory::createEffect(typeId.toStdString()); effect) {
        // A send returns only what the effect adds, and the send amounts set how much of it: Mix
        // starts at the top there, where every effect is heard as itself. Only a start: the knob
        // is free to move after, and an effect loaded with a project keeps what it was saved with.
        if (isMasterSendRack()) {
            if (const auto mix = effect->parameter(effect->mixParameterKey()); mix) {
                mix->get().setValue(1.0f);
                effect->sync();
            }
        }
        if (const auto rack = currentRack(); rack) {
            rack->get().setEffect(static_cast<size_t>(slotIndex), std::move(effect));
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

void EffectRackController::clearEffect(int slotIndex)
{
    if (const auto rack = currentRack(); rack) {
        rack->get().setEffect(static_cast<size_t>(slotIndex), nullptr);
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectUp(int index)
{
    if (index <= 0) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        rack->get().swapEffects(static_cast<size_t>(index), static_cast<size_t>(index - 1));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectDown(int index)
{
    if (const auto rack = currentRack(); rack) {
        if (index >= static_cast<int>(rack->get().effectCount()) - 1) {
            return;
        }
        rack->get().swapEffects(static_cast<size_t>(index), static_cast<size_t>(index + 1));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectToTop(int index)
{
    if (index <= 0) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        rack->get().moveEffect(static_cast<size_t>(index), 0);
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectToBottom(int index)
{
    if (const auto rack = currentRack(); rack) {
        const auto lastIndex = static_cast<int>(rack->get().effectCount()) - 1;
        if (index < 0 || index >= lastIndex) {
            return;
        }
        rack->get().moveEffect(static_cast<size_t>(index), static_cast<size_t>(lastIndex));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

QVariantList EffectRackController::availableEffects() const
{
    QVariantList list;

    // The gallery's grouping, shown after each name
    const auto dynamics = tr("Dynamics");
    const auto eqAndFilter = tr("EQ & Filter");
    const auto distortion = tr("Distortion");
    const auto modulation = tr("Modulation");
    const auto space = tr("Space");
    const auto stereo = tr("Stereo");
    const auto utility = tr("Utility");
    const auto meters = tr("Meters");

    const auto addEffect = [&](const QString & name, const std::string & typeId, const QString & category) {
        QVariantMap map;
        map["name"] = name;
        map["typeId"] = QString::fromStdString(typeId);
        map["category"] = category;
        list.append(map);
    };

    addEffect("dBTP Meter", DbTpMeter::typeIdString(), meters);
    addEffect("LUFS Meter", LufsMeter::typeIdString(), meters);
    addEffect("RTA", Constants::RackEffectType::rta().toStdString(), meters);
    addEffect("Stereo Field Meter", Constants::RackEffectType::stereoFieldMeter().toStdString(), meters);

    // The effects that register themselves
    for (auto && effect : PluginCatalog::effects()) {
        const auto category = [&]() {
            switch (effect.category) {
            case PluginCatalog::EffectCategory::Dynamics:
                return dynamics;
            case PluginCatalog::EffectCategory::EqAndFilter:
                return eqAndFilter;
            case PluginCatalog::EffectCategory::Distortion:
                return distortion;
            case PluginCatalog::EffectCategory::Modulation:
                return modulation;
            case PluginCatalog::EffectCategory::Space:
                return space;
            case PluginCatalog::EffectCategory::Stereo:
                return stereo;
            case PluginCatalog::EffectCategory::Utility:
                return utility;
            case PluginCatalog::EffectCategory::Meters:
                return meters;
            }
            return utility;
        }();
        addEffect(effect.name, effect.typeId, category);
    }

    // Sorted here rather than by hand at the call sites above: the gallery is one flat list, so the
    // order it is built in is the order the user reads, and a list kept alphabetical by convention
    // alone drifts every time an effect is added. Case-insensitive so that "dBTP Meter" lands with
    // the D's rather than after every capital.
    std::ranges::sort(list, [](const QVariant & a, const QVariant & b) {
        return QString::compare(a.toMap()["name"].toString(), b.toMap()["name"].toString(), Qt::CaseInsensitive) < 0;
    });

    return list;
}

QString EffectRackController::effectDialogFile(const QString & effectType) const
{
    if (const auto effect = PluginCatalog::effectByType(effectType); effect) {
        return effect->dialogFile;
    }
    return {};
}

void EffectRackController::copyEffect(int sourceSlot, int targetSlot)
{
    if (const auto rack = currentRack(); rack) {
        if (rack->get().copyEffect(static_cast<size_t>(sourceSlot), static_cast<size_t>(targetSlot))) {
            copySendChain(sourceSlot, targetSlot);
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

void EffectRackController::copySendChain(int sourceBus, int targetBus)
{
    // The master send rack is the only rack whose slots are buses: a device's inserts, a pad's, a
    // voice's and a chain rack itself are all plain lists with nothing hanging off a slot.
    if (!m_targetDeviceName.isEmpty() || m_isInsertRack || m_targetSubIndex >= 0) {
        return;
    }

    const auto chainCount = m_deviceService->sendChainRackCount();
    if (sourceBus < 0 || targetBus < 0 || static_cast<size_t>(sourceBus) >= chainCount || static_cast<size_t>(targetBus) >= chainCount) {
        return;
    }

    // copyFrom() empties the target first, so a bus copied from one with no chain is left with none
    // of its own -- the target ends up being what it was copied from, chain and all.
    m_deviceService->sendChainRack(static_cast<size_t>(targetBus)).copyFrom(m_deviceService->sendChainRack(static_cast<size_t>(sourceBus)));
}

QVariantList EffectRackController::availableRackSources() const
{
    QVariantList list;

    const auto rackEntry = [this](const QString & deviceName, bool isInsertRack, const QString & name, const QString & description) {
        QVariantMap map;
        map["deviceName"] = deviceName;
        map["isInsertRack"] = isInsertRack;
        map["name"] = name;
        map["description"] = description;
        const auto rack = rackAt(deviceName, isInsertRack, -1);
        const auto effects = rack ? rack->get().effects() : std::vector<EffectRack::EffectS> {};
        map["effectCount"] = static_cast<int>(std::ranges::count_if(effects, [](const auto & effect) { return effect != nullptr; }));
        return map;
    };

    // The rack being copied into is not a source for itself. Only whole-device racks and the two
    // master racks are addressable here, so a pad or voice rack is never the target of this check.
    const auto isTarget = [this](const QString & deviceName, bool isInsertRack) {
        return m_targetSubIndex < 0 && m_targetDeviceName == deviceName && (!deviceName.isEmpty() || m_isInsertRack == isInsertRack);
    };

    if (!isTarget({}, true)) {
        list.append(rackEntry({}, true, tr("Master Insert Effects"), {}));
    }
    if (!isTarget({}, false)) {
        list.append(rackEntry({}, false, tr("Master Send Effects"), {}));
    }

    for (size_t slotIndex = 0; slotIndex < Constants::deviceRackSize(); slotIndex++) {
        if (const auto device = m_deviceService->device(slotIndex); device) {
            const auto deviceName = QString::fromStdString(device->name());
            if (!isTarget(deviceName, true)) {
                list.append(rackEntry(deviceName, true, deviceName, QString::fromStdString(device->typeName())));
            }
        }
    }

    return list;
}

bool EffectRackController::copyRackFrom(const QString & sourceDeviceName, bool sourceIsInsertRack)
{
    const auto target = currentRack();
    const auto source = rackAt(sourceDeviceName, sourceIsInsertRack, -1);
    if (!target || !source || &target->get() == &source->get()) {
        return false;
    }

    target->get().copyFrom(source->get());
    m_editorService->setIsModified(true);
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
    // copyFrom() brings the source's bypass state along with its effects.
    emit rackEnabledChanged();
    return true;
}

QVariantList EffectRackController::populatedEffects() const
{
    QVariantList list;
    if (const auto rack = currentRack(); rack) {
        const auto count = rack->get().effectCount();
        for (size_t i = 0; i < count; i++) {
            if (const auto effect = rack->get().effect(i)) {
                QVariantMap map;
                map["slotIndex"] = static_cast<int>(i);
                map["name"] = effectDisplayName(QString::fromStdString(effect->type()));
                list.append(map);
            }
        }
    }
    return list;
}

bool EffectRackController::isEffectEnabled(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            return effect->enabled();
        }
    }
    return false;
}

void EffectRackController::setIsEffectEnabled(quint32 effectIndex, bool enabled)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (effect->enabled() != enabled) {
                effect->setEnabled(enabled);
                m_editorService->setIsModified(true);
                m_revision++;
                emit revisionChanged();
            }
        }
    }
}

bool EffectRackController::rackEnabled() const
{
    if (const auto rack = currentRack(); rack) {
        return rack->get().enabled();
    }
    return true;
}

void EffectRackController::setRackEnabled(bool enabled)
{
    if (const auto rack = currentRack(); rack) {
        if (rack->get().enabled() != enabled) {
            rack->get().setEnabled(enabled);
            m_editorService->setIsModified(true);
            emit rackEnabledChanged();
            m_revision++;
            emit revisionChanged();
        }
    }
}

QStringList EffectRackController::parameterNames(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            QStringList names;
            for (const auto & name : effect->parameterNames()) {
                names.append(QString::fromStdString(name));
            }
            return names;
        }
    }
    return {};
}

QString EffectRackController::effectType(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            return QString::fromStdString(effect->type());
        }
    }
    return "";
}

QString EffectRackController::effectId(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            return QString::fromStdString(effect->id());
        }
    }
    return "";
}

QString EffectRackController::effectDisplayName(const QString & typeId) const
{
    for (const auto & item : availableEffects()) {
        const auto map = item.toMap();
        if (map["typeId"].toString() == typeId) {
            return map["name"].toString();
        }
    }
    return typeId;
}

QString EffectRackController::effectParametersSummary(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        return summaryFor(rack->get().effect(effectIndex));
    }
    return {};
}

QString EffectRackController::masterSendParametersSummary(quint32 busIndex) const
{
    // The master send rack rather than whatever the controller is pointed at: a device's Sends tab
    // names the buses while the controller is still targeting that device's own rack.
    return summaryFor(m_deviceService->sendEffectRack().effect(busIndex));
}

PluginCatalog::SummaryContext EffectRackController::summaryContext() const
{
    return {
        .device = [deviceService = m_deviceService](size_t slot) { return deviceService ? deviceService->device(slot) : nullptr; },
    };
}

QString EffectRackController::summaryFor(const EffectRack::EffectS & effect) const
{
    {
        if (effect) {
            const auto type = QString::fromStdString(effect->type());
            // An effect that registers itself gives its own summary
            if (const auto plugin = PluginCatalog::effectByType(type); plugin && plugin->summary) {
                return plugin->summary(effect, summaryContext());
            }
            if (type == Constants::RackEffectType::stereoFieldMeter()) {
                if (const auto meter { std::dynamic_pointer_cast<StereoFieldMeter>(effect) }; meter) {
                    const auto reading = meter->reading();
                    return QString { "(corr=%1, side=%2dB)" }
                      .arg(reading.correlation, 0, 'f', 2)
                      .arg(reading.sideDb, 0, 'f', 1);
                }
            } else if (type == Constants::RackEffectType::lufsMeter()) {
                if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                    return QString { "(M=%1 S=%2 I=%3 LUFS)" }
                      .arg(formatMeterReading(meter->momentaryLufs()))
                      .arg(formatMeterReading(meter->shortTermLufs()))
                      .arg(formatMeterReading(meter->integratedLufs()));
                }
            } else if (type == Constants::RackEffectType::dbtpMeter()) {
                if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                    return QString { "(L=%1 R=%2 dBTP)" }
                      .arg(formatMeterReading(meter->truePeakHoldL()))
                      .arg(formatMeterReading(meter->truePeakHoldR()));
                }
            }
        }
    }
    return {};
}

QStringList EffectRackController::referenceEnvironmentNames() const
{
    // Read from the table the effect itself selects with, so the list cannot name a system the
    // effect does not have or miss one it does.
    QStringList names;
    for (auto && environment : referenceEnvironments()) {
        names << QString::fromStdString(environment.name);
    }
    return names;
}

bool EffectRackController::gainClipped(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto gain = std::dynamic_pointer_cast<Gain>(effect); gain) {
                return gain->clipDetector().clipped();
            }
        }
    }
    return false;
}

void EffectRackController::clearGainClip(quint32 effectIndex)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto gain = std::dynamic_pointer_cast<Gain>(effect); gain) {
                gain->clipDetector().clear();
            }
        }
    }
}

float EffectRackController::eq8BandParametricDefaultQ(int bandType) const
{
    const auto q = Eq8BandParametric::defaultQParameterValue(static_cast<SvfFilter::Type>(bandType));
    return q.value_or(-1.0f);
}

namespace {

//! The frequencies a curve is drawn at, evenly spaced by octave from 20 Hz to 20 kHz.
//!
//! The spacing EqCurveRenderer expects, kept in one place so that every curve in every dialog is
//! sampled the same way.
std::vector<double> responseFrequencies(int points)
{
    std::vector<double> frequencies;
    if (points <= 1) {
        return frequencies;
    }
    frequencies.reserve(static_cast<size_t>(points));
    for (int i = 0; i < points; i++) {
        const double t = static_cast<double>(i) / static_cast<double>(points - 1);
        frequencies.push_back(20.0 * std::pow(1000.0, t));
    }
    return frequencies;
}

QVariantList sampleResponse(int points, const std::function<double(double)> & magnitudeDbAt)
{
    QVariantList response;
    for (const double hz : responseFrequencies(points)) {
        response.append(magnitudeDbAt(hz));
    }
    return response;
}

} // namespace

std::shared_ptr<Effect> EffectRackController::effectAt(quint32 effectIndex) const
{
    const auto rack = currentRack();
    return rack ? rack->get().effect(static_cast<size_t>(effectIndex)) : nullptr;
}

void EffectRackController::eq8BandParametricSetBandType(quint32 effectIndex, quint32 bandIndex, int bandType)
{
    const auto typeKey = Constants::NahdXml::xmlKeyBandType(bandIndex);
    if (static_cast<int>(std::round(parameterValue(effectIndex, typeKey))) == bandType) {
        return;
    }

    setParameterValue(effectIndex, typeKey, static_cast<float>(bandType));

    if (const auto q = Eq8BandParametric::defaultQParameterValue(static_cast<SvfFilter::Type>(bandType)); q) {
        setParameterValue(effectIndex, Constants::NahdXml::xmlKeyBandQ(bandIndex), *q);
    }
}

QVariantList EffectRackController::eq8BandParametricResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<Eq8BandParametric>(effectAt(effectIndex));
    if (!effect) {
        return {};
    }
    QVariantList response;
    const auto path = effect->isPathBypassed(Eq8BandParametric::Path::Mid) ? Eq8BandParametric::Path::Side : Eq8BandParametric::Path::Mid;
    for (const double db : effect->magnitudesDbAt(responseFrequencies(points), path)) {
        response.append(db);
    }
    return response;
}

double EffectRackController::eqAutoGainDb(quint32 effectIndex) const
{
    const auto effect = effectAt(effectIndex);
    if (const auto eq = std::dynamic_pointer_cast<Eq8BandParametric>(effect); eq) {
        return eq->autoGainDb();
    }
    if (const auto eq = std::dynamic_pointer_cast<Eq32BandGraphical>(effect); eq) {
        return eq->autoGainDb();
    }
    if (const auto eq = std::dynamic_pointer_cast<AirBandEq>(effect); eq) {
        return eq->autoGainDb();
    }
    if (const auto eq = std::dynamic_pointer_cast<VintagePassiveEq>(effect); eq) {
        return eq->autoGainDb();
    }
    if (const auto eq = std::dynamic_pointer_cast<SimpleEq>(effect); eq) {
        return eq->autoGainDb();
    }
    return 0.0;
}

QVariantList EffectRackController::eq8BandParametricPassThroughResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<Eq8BandParametric>(effectAt(effectIndex));
    if (!effect) {
        return {};
    }

    // Nothing to draw when the equalizer shapes both paths: a second curve lying exactly on the
    // first says only that there is a second curve.
    const auto bypassed = effect->isPathBypassed(Eq8BandParametric::Path::Mid)
      ? Eq8BandParametric::Path::Mid
      : (effect->isPathBypassed(Eq8BandParametric::Path::Side) ? Eq8BandParametric::Path::Side : std::optional<Eq8BandParametric::Path> {});
    if (!bypassed) {
        return {};
    }

    return sampleResponse(points, [&effect, path = *bypassed](double hz) { return effect->magnitudeDbAt(hz, path); });
}

QVariantList EffectRackController::airBandEqResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<AirBandEq>(effectAt(effectIndex));
    if (!effect) {
        return {};
    }
    QVariantList response;
    for (const double db : effect->magnitudesDbAt(responseFrequencies(points))) {
        response.append(db);
    }
    return response;
}

QVariantList EffectRackController::eq32BandGraphicalResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<Eq32BandGraphical>(effectAt(effectIndex));
    if (!effect) {
        return {};
    }
    // All points at once: the equalizer designs its bands from the parameters for every call, and
    // that is the expensive part
    QVariantList response;
    for (const double db : effect->magnitudesDbAt(responseFrequencies(points))) {
        response.append(db);
    }
    return response;
}

int EffectRackController::eq32BandGraphicalBandCount() const
{
    return static_cast<int>(Eq32BandGraphical::BandCount);
}

QString EffectRackController::eq32BandGraphicalBandLabel(quint32 bandIndex) const
{
    return QString::fromStdString(Eq32BandGraphical::nominalLabel(bandIndex));
}

double EffectRackController::eq32BandGraphicalLinearPhaseLatencyMs(quint32 effectIndex) const
{
    const auto effect = effectAt(effectIndex);
    if (!effect || effect->sampleRate() <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(Eq32BandGraphical::linearPhaseLatencyFrames(effect->sampleRate())) * 1000.0 / effect->sampleRate();
}

void EffectRackController::eq32BandGraphicalFlatten(quint32 effectIndex)
{
    const auto effect = std::dynamic_pointer_cast<Eq32BandGraphical>(effectAt(effectIndex));
    if (!effect) {
        return;
    }
    // One revision for the lot rather than one per slider, each of which would redraw the curve
    bool changed = false;
    for (size_t i = 0; i < Eq32BandGraphical::BandCount; i++) {
        const auto key = Constants::NahdXml::xmlKeyBandGain(i);
        if (const auto parameter = effect->parameter(key.toStdString()); parameter && parameter->get().update(Eq32BandGraphical::sliderParameterValue(0.0))) {
            changed = true;
            emit parameterChanged(effectIndex, key);
        }
    }
    if (changed) {
        effect->sync();
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

QStringList EffectRackController::lfoWaveformNames() const
{
    QStringList names;
    for (auto && name : Lfo::waveformNames()) {
        names.append(QString::fromStdString(name));
    }
    return names;
}

QStringList EffectRackController::lfoModeNames() const
{
    // Ordered by Lfo::Mode, which the parameter stores as its ordinal.
    return { tr("Normal"), tr("Sync"), tr("One-Shot") };
}

QVariantList EffectRackController::stereoFieldMeterPoints(quint32 effectIndex, int maxPoints) const
{
    QVariantList list;
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                const auto snapshot = meter->trace(static_cast<size_t>(std::max(maxPoints, 0)));
                const auto count = std::min(snapshot.left.size(), snapshot.right.size());
                list.reserve(static_cast<qsizetype>(count * 2));
                for (size_t i = 0; i < count; i++) {
                    list.append(snapshot.left[i]);
                    list.append(snapshot.right[i]);
                }
            }
        }
    }
    return list;
}

QVariantMap EffectRackController::stereoFieldMeterReading(quint32 effectIndex) const
{
    QVariantMap map;
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                const auto reading = meter->reading();
                QVariantList bands;
                for (const float value : reading.bandCorrelation) {
                    bands.append(value);
                }
                map["correlation"] = reading.correlation;
                map["bandCorrelations"] = bands;
                map["midDb"] = reading.midDb;
                map["sideDb"] = reading.sideDb;
                map["balance"] = reading.balance;
            }
        }
    }
    return map;
}

void EffectRackController::stereoFieldMeterSetActive(quint32 effectIndex, bool active)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                if (active) {
                    meter->addAnalysisViewer();
                } else {
                    meter->removeAnalysisViewer();
                }
            }
        }
    }
}

QString EffectRackController::allPassFilterType() const
{
    return Constants::RackEffectType::allPassFilter();
}

QString EffectRackController::lufsMeterType() const
{
    return Constants::RackEffectType::lufsMeter();
}

QString EffectRackController::dbtpMeterType() const
{
    return Constants::RackEffectType::dbtpMeter();
}

QString EffectRackController::rtaType() const
{
    return Constants::RackEffectType::rta();
}

QString EffectRackController::clipperType() const
{
    return Constants::RackEffectType::clipper();
}

QString EffectRackController::saturatorType() const
{
    return Constants::RackEffectType::saturator();
}

QString EffectRackController::waveDesignerType() const
{
    return Constants::RackEffectType::waveDesigner();
}

QString EffectRackController::stereoEnhancerType() const
{
    return Constants::RackEffectType::stereoEnhancer();
}

QString EffectRackController::stereoExciterType() const
{
    return Constants::RackEffectType::stereoExciter();
}

QString EffectRackController::analogFuzzType() const
{
    return Constants::RackEffectType::analogFuzz();
}

QString EffectRackController::tubeStageType() const
{
    return Constants::RackEffectType::tubeStage();
}

QString EffectRackController::driveType() const
{
    return Constants::RackEffectType::drive();
}

QString EffectRackController::bassGrinderType() const
{
    return Constants::RackEffectType::bassGrinder();
}

QString EffectRackController::limiterType() const
{
    return Constants::RackEffectType::limiter();
}

QString EffectRackController::monitorType() const
{
    return Constants::RackEffectType::monitor();
}

QString EffectRackController::gateType() const
{
    return Constants::RackEffectType::gate();
}

QString EffectRackController::bitCrusherType() const
{
    return Constants::RackEffectType::bitCrusher();
}

QString EffectRackController::flangerType() const
{
    return Constants::RackEffectType::flanger();
}

QString EffectRackController::tremoloType() const
{
    return Constants::RackEffectType::tremolo();
}

QString EffectRackController::crossfeedType() const
{
    return Constants::RackEffectType::crossfeed();
}

QString EffectRackController::referenceType() const
{
    return Constants::RackEffectType::reference();
}

QString EffectRackController::gainType() const
{
    return Constants::RackEffectType::gain();
}

QString EffectRackController::chorusType() const
{
    return Constants::RackEffectType::chorus();
}

QString EffectRackController::compressorType() const
{
    return Constants::RackEffectType::compressor();
}

QString EffectRackController::autoDuckerType() const
{
    return Constants::RackEffectType::autoDucker();
}

QString EffectRackController::autoFilterType() const
{
    return Constants::RackEffectType::autoFilter();
}

QString EffectRackController::phaserType() const
{
    return Constants::RackEffectType::phaser();
}

QString EffectRackController::multibandCompressorType() const
{
    return Constants::RackEffectType::multibandCompressor();
}

QString EffectRackController::stereoWidenerType() const
{
    return Constants::RackEffectType::stereoWidener();
}

QString EffectRackController::stereoFieldMeterType() const
{
    return Constants::RackEffectType::stereoFieldMeter();
}

QString EffectRackController::dimensionType() const
{
    return Constants::RackEffectType::dimension();
}

QString EffectRackController::compressor2AType() const
{
    return Constants::RackEffectType::compressor2A();
}

QString EffectRackController::doublerType() const
{
    return Constants::RackEffectType::doubler();
}

QString EffectRackController::earlyReflectionsType() const
{
    return Constants::RackEffectType::earlyReflections();
}

QString EffectRackController::delayType() const
{
    return Constants::RackEffectType::delay();
}

QString EffectRackController::eq8BandParametricType() const
{
    return Constants::RackEffectType::eq8BandParametric();
}

QString EffectRackController::vintagePassiveEqType() const
{
    return Constants::RackEffectType::vintagePassiveEq();
}

QString EffectRackController::airBandEqType() const
{
    return Constants::RackEffectType::airBandEq();
}

QString EffectRackController::ambienceType() const
{
    return Constants::RackEffectType::ambience();
}

QString EffectRackController::reverb224Type() const
{
    return Constants::RackEffectType::reverb224();
}

bool EffectRackController::reverb224HasEarlyReflections(int program) const
{
    return Reverb224::hasEarlyReflections(static_cast<Reverb224::Program>(std::clamp(program, 0, static_cast<int>(Reverb224::ProgramCount) - 1)));
}

QString EffectRackController::dynamicEqType() const
{
    return Constants::RackEffectType::dynamicEq();
}

namespace {

QVariantList dynamicEqResponseAt(const std::shared_ptr<DynamicEq> & effect, int points, double gainDb)
{
    QVariantList response;
    if (effect) {
        for (const double db : effect->magnitudesDbAt(responseFrequencies(points), gainDb)) {
            response.append(db);
        }
    }
    return response;
}

} // namespace

QVariantList EffectRackController::dynamicEqResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex));
    return dynamicEqResponseAt(effect, points, effect ? effect->currentGainDb() : 0.0);
}

QVariantList EffectRackController::dynamicEqRangeResponse(quint32 effectIndex, int points) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex));
    return dynamicEqResponseAt(effect, points, effect ? effect->rangeDb() : 0.0);
}

float EffectRackController::dynamicEqGainDb(quint32 effectIndex) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex));
    return effect ? effect->currentGainDb() : 0.0f;
}

float EffectRackController::dynamicEqDetectorLevelDb(quint32 effectIndex) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex));
    return effect ? effect->detectorLevelDb() : -120.0f;
}

float EffectRackController::dynamicEqRangeDb(quint32 effectIndex) const
{
    const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex));
    return effect ? static_cast<float>(effect->rangeDb()) : 0.0f;
}

QVariantList EffectRackController::dynamicEqGainHistory(quint32 effectIndex) const
{
    QVariantList list;
    if (const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex)); effect) {
        for (const float db : effect->gainHistory()) {
            list.append(db);
        }
    }
    return list;
}

void EffectRackController::dynamicEqSetActive(quint32 effectIndex, bool active)
{
    if (const auto effect = std::dynamic_pointer_cast<DynamicEq>(effectAt(effectIndex)); effect) {
        effect->setAnalysisEnabled(active);
    }
}

QString EffectRackController::phaseScopeType() const
{
    return Constants::RackEffectType::phaseScope();
}

int EffectRackController::phaseScopeDelaySamples(quint32 effectIndex) const
{
    const auto effect = std::dynamic_pointer_cast<PhaseScope>(effectAt(effectIndex));
    if (!effect) {
        return 0;
    }
    const auto delay = effect->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString());
    return delay ? static_cast<int>(PhaseScope::delaySamples(delay->get().shownValue(), effect->sampleRate())) : 0;
}

void EffectRackController::phaseScopeSetDelaySamples(quint32 effectIndex, int samples)
{
    const auto effect = std::dynamic_pointer_cast<PhaseScope>(effectAt(effectIndex));
    if (!effect || effect->sampleRate() <= 0.0) {
        return;
    }
    const auto maxSamples = static_cast<int>(PhaseScope::delaySamples(1.0f, effect->sampleRate()));
    const double ms = static_cast<double>(std::clamp(samples, 0, maxSamples)) * 1000.0 / effect->sampleRate();
    setParameterValue(effectIndex, Constants::NahdXml::xmlKeyDelayTime(), static_cast<float>(ms / PhaseScope::MaxDelayMs));
}

void EffectRackController::phaseScopeSetActive(quint32 effectIndex, bool active)
{
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(effectAt(effectIndex)); effect) {
        effect->setAnalysisEnabled(active);
    }
}

QVariantMap EffectRackController::phaseScopeReading(quint32 effectIndex) const
{
    QVariantMap map;
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(effectAt(effectIndex)); effect) {
        const auto reading = effect->reading();
        map["hasReference"] = reading.hasReference;
        map["correlation"] = reading.correlation;
        map["referenceDb"] = reading.referenceDb;
        map["signalDb"] = reading.signalDb;
        map["sumGainDb"] = reading.sumGainDb;
    }
    return map;
}

QVariantMap EffectRackController::phaseScopeCapture(quint32 effectIndex, int maxPoints) const
{
    QVariantMap map;
    if (const auto effect = std::dynamic_pointer_cast<PhaseScope>(effectAt(effectIndex)); effect) {
        const auto capture = effect->capture();
        // Every n-th sample, the same n for both, so that the two stay aligned
        const auto decimate = [](const std::vector<float> & samples, size_t stride) {
            QVariantList list;
            for (size_t i = 0; i < samples.size(); i += stride) {
                list.append(samples[i]);
            }
            return list;
        };
        const auto length = std::max(capture.reference.size(), capture.signal.size());
        const auto stride = std::max<size_t>(1, maxPoints > 0 ? (length + static_cast<size_t>(maxPoints) - 1) / static_cast<size_t>(maxPoints) : 1);
        map["reference"] = decimate(capture.reference, stride);
        map["signal"] = decimate(capture.signal, stride);
    }
    return map;
}

QString EffectRackController::eq32BandGraphicalType() const
{
    return Constants::RackEffectType::eq32BandGraphical();
}

QString EffectRackController::simpleEqType() const
{
    return Constants::RackEffectType::simpleEq();
}

QString EffectRackController::pannerType() const
{
    return Constants::RackEffectType::panner();
}

QString EffectRackController::autoPannerType() const
{
    return Constants::RackEffectType::autoPanner();
}

QString EffectRackController::reverbType() const
{
    return Constants::RackEffectType::reverb();
}

QString EffectRackController::endlessType() const
{
    return Constants::RackEffectType::endless();
}

float EffectRackController::compressorReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto compressor = std::dynamic_pointer_cast<Compressor>(effect)) {
                return compressor->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::multibandCompressorBandReductionDb(quint32 effectIndex, quint32 bandIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto compressor = std::dynamic_pointer_cast<MultibandCompressor>(effect); compressor) {
                return compressor->bandReductionDb(bandIndex);
            }
        }
    }

    return 0.0f;
}

float EffectRackController::stereoWidenerBandCorrelation(quint32 effectIndex, quint32 bandIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto width = std::dynamic_pointer_cast<StereoWidener>(effect); width) {
                return width->bandCorrelation(bandIndex);
            }
        }
    }

    // Nothing to read is reported as centred, which is where the meter rests.
    return 1.0f;
}

float EffectRackController::clipperReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto clipper = std::dynamic_pointer_cast<Clipper>(effect); clipper) {
                return clipper->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::limiterReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto limiter = std::dynamic_pointer_cast<Limiter>(effect); limiter) {
                return limiter->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::compressor2AReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto compressor = std::dynamic_pointer_cast<Compressor2A>(effect); compressor) {
                return compressor->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::autoDuckerGainDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto autoDucker = std::dynamic_pointer_cast<AutoDucker>(effect); autoDucker) {
                return autoDucker->gainDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::saturatorSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto saturator = std::dynamic_pointer_cast<Saturator>(effect); saturator) {
                return saturator->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::waveDesignerShapingDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto waveDesigner = std::dynamic_pointer_cast<WaveDesigner>(effect); waveDesigner) {
                return waveDesigner->shapingDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::stereoExciterHarmonicsDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto exciter = std::dynamic_pointer_cast<StereoExciter>(effect); exciter) {
                return exciter->harmonicsDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::analogFuzzSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto analogFuzz = std::dynamic_pointer_cast<AnalogFuzz>(effect); analogFuzz) {
                return analogFuzz->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::tubeStageSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto tubeStage = std::dynamic_pointer_cast<TubeStage>(effect); tubeStage) {
                return tubeStage->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::bassGrinderSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto bassGrinder = std::dynamic_pointer_cast<BassGrinder>(effect); bassGrinder) {
                return bassGrinder->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::lufsMeterMomentary(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->momentaryLufs();
            }
        }
    }
    return -70.0f;
}

float EffectRackController::lufsMeterShortTerm(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->shortTermLufs();
            }
        }
    }
    return -70.0f;
}

float EffectRackController::lufsMeterIntegrated(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->integratedLufs();
            }
        }
    }
    return -70.0f;
}

void EffectRackController::resetLufsMeter(quint32 effectIndex)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                meter->requestReset();
            }
        }
    }
}

float EffectRackController::dbtpMeterTruePeakL(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakL();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakR(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakR();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakHoldL(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakHoldL();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakHoldR(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakHoldR();
            }
        }
    }
    return dbtpFloor;
}

void EffectRackController::rtaUpdateRenderer(quint32 effectIndex, QObject * rendererObject) const
{
    const auto renderer = qobject_cast<RtaRenderer *>(rendererObject);
    if (!renderer) {
        return;
    }

    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto rta = std::dynamic_pointer_cast<Rta>(effect)) {
                // Layout first: a frame drawn with the levels of the new band count against the
                // positions of the old one would put every bar in the wrong place.
                if (const auto generation = rta->layoutGeneration(); generation != renderer->layoutGeneration()) {
                    renderer->setBandLayout(rta->bandLogPositions(), generation);
                }
                renderer->setBandLevels(rta->bandMagnitudesDb());
            }
        }
    }
}

void EffectRackController::rtaSetActive(quint32 effectIndex, bool active)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto rta = std::dynamic_pointer_cast<Rta>(effect)) {
                if (active) {
                    rta->addAnalysisViewer();
                } else {
                    rta->removeAnalysisViewer();
                }
            }
        }
    }
}

void EffectRackController::spectrumUpdateRenderer(quint32 effectIndex, QObject * rendererObject) const
{
    const auto renderer = qobject_cast<EqCurveRenderer *>(rendererObject);
    if (!renderer) {
        return;
    }

    if (const auto rack = currentRack()) {
        if (const auto source = std::dynamic_pointer_cast<SpectrumSource>(rack->get().effect(effectIndex))) {
            const auto & taps = source->spectrumTaps();
            const auto pre = renderer->spectrumUsesPre() ? taps.core(SpectrumTaps::Tap::Pre) : nullptr;
            const auto post = renderer->spectrumUsesPost() ? taps.core(SpectrumTaps::Tap::Post) : nullptr;
            // Both taps are built alike, so either one's layout is the other's
            if (const auto layout = post ? post : pre) {
                if (const auto generation = layout->layoutGeneration(); generation != renderer->spectrumLayoutGeneration()) {
                    renderer->setSpectrumLayout(layout->bandLogPositions(), generation);
                }
                renderer->setSpectrumLevels(pre ? pre->bandMagnitudesDb() : std::vector<float> {}, post ? post->bandMagnitudesDb() : std::vector<float> {});
            }
        }
    }
}

void EffectRackController::spectrumSetActive(quint32 effectIndex, QObject * rendererObject, bool active)
{
    const auto renderer = qobject_cast<EqCurveRenderer *>(rendererObject);
    if (const auto rack = currentRack()) {
        if (const auto source = std::dynamic_pointer_cast<SpectrumSource>(rack->get().effect(effectIndex))) {
            const bool pre = active && renderer && renderer->spectrumUsesPre();
            const bool post = active && renderer && renderer->spectrumUsesPost();
            source->spectrumTaps().setEnabled(pre, post);
        }
    }
}

void EffectRackController::setPresetService(PresetServiceS presetService)
{
    m_presetService = std::move(presetService);
}

QString EffectRackController::effectTypeId(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            return QString::fromStdString(effect->typeId());
        }
    }
    return {};
}

int EffectRackController::effectFactoryPresetCount(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            return static_cast<int>(effect->factoryPresets().size());
        }
    }
    return 0;
}

QStringList EffectRackController::effectUserPresetNames(quint32 effectIndex) const
{
    if (const auto typeId = effectTypeId(effectIndex); m_presetService && !typeId.isEmpty()) {
        return m_presetService->userPresetNames(typeId);
    }
    return {};
}

QStringList EffectRackController::effectPresetNames(quint32 effectIndex) const
{
    const auto rack = currentRack();
    if (!rack) {
        return {};
    }
    const auto effect = rack->get().effect(static_cast<size_t>(effectIndex));
    if (!effect) {
        return {};
    }

    QStringList names;
    const auto append = [&names](const QString & name, bool isUserPreset) {
        // The number is the position in this very list, so what the dropdown shows and what
        // loadEffectPreset() is given stay the same however many presets the user has added.
        names.append(QString { "%1: %2%3" }
                       .arg(names.size(), 3, 10, QChar { '0' })
                       .arg(name)
                       .arg(isUserPreset ? Constants::userPresetMarker() : QString {}));
    };
    for (const auto & preset : effect->factoryPresets()) {
        append(QString::fromStdString(preset.name), false);
    }
    for (const auto & name : effectUserPresetNames(effectIndex)) {
        append(name, true);
    }
    return names;
}

int EffectRackController::currentEffectPresetIndex(quint32 effectIndex) const
{
    return m_presetEffectIndex == static_cast<int>(effectIndex) ? m_presetIndex : 0;
}

bool EffectRackController::currentEffectPresetIsUserPreset(quint32 effectIndex) const
{
    const auto userIndex = currentEffectPresetIndex(effectIndex) - effectFactoryPresetCount(effectIndex);
    return userIndex >= 0 && userIndex < static_cast<int>(effectUserPresetNames(effectIndex).size());
}

QString EffectRackController::currentEffectUserPresetName(quint32 effectIndex) const
{
    if (!currentEffectPresetIsUserPreset(effectIndex)) {
        return {};
    }
    return effectUserPresetNames(effectIndex).at(currentEffectPresetIndex(effectIndex) - effectFactoryPresetCount(effectIndex));
}

void EffectRackController::loadEffectPreset(quint32 effectIndex, int presetIndex)
{
    const auto rack = currentRack();
    if (!rack) {
        return;
    }
    const auto effect = rack->get().effect(static_cast<size_t>(effectIndex));
    if (!effect) {
        return;
    }

    // The controller owns which preset is showing, so the dialog only has to ask for one to be
    // loaded. Left to the dialog, the combo box reads back its old value and snaps back.
    m_presetEffectIndex = static_cast<int>(effectIndex);
    m_presetIndex = presetIndex;

    // On a send, Mix belongs to the routing as much as to the sound, like the side-chain source a
    // preset leaves alone: the preset brings its sound and the Mix stays where it was set
    const auto mix = effect->parameter(effect->mixParameterKey());
    const auto keptMix = isMasterSendRack() && mix ? std::optional<float> { mix->get().shownValue() } : std::nullopt;

    const auto factoryCount = effectFactoryPresetCount(effectIndex);
    if (presetIndex < factoryCount) {
        effect->applyFactoryPreset(static_cast<size_t>(presetIndex));
    } else if (const auto names = effectUserPresetNames(effectIndex); m_presetService) {
        if (const auto userIndex = presetIndex - factoryCount; userIndex < static_cast<int>(names.size())) {
            m_presetService->applyUserPreset(effectTypeId(effectIndex), names.at(userIndex), *effect);
        }
    }
    if (keptMix) {
        mix->get().setValue(*keptMix);
        effect->sync();
    }

    m_editorService->setIsModified(true);
    m_revision++;
    emit revisionChanged();
    emit effectPresetsChanged(effectIndex);
    // Every parameter at once rather than one by one: a preset writes the whole panel.
    emit parameterChanged(effectIndex, "");
}

bool EffectRackController::effectUserPresetExists(quint32 effectIndex, const QString & presetName) const
{
    const auto typeId = effectTypeId(effectIndex);
    return m_presetService && !typeId.isEmpty() && m_presetService->userPresetExists(typeId, presetName);
}

bool EffectRackController::saveEffectUserPreset(quint32 effectIndex, const QString & presetName)
{
    const auto rack = currentRack();
    if (!rack || !m_presetService) {
        return false;
    }
    const auto effect = rack->get().effect(static_cast<size_t>(effectIndex));
    const auto typeId = effectTypeId(effectIndex);
    if (!effect || typeId.isEmpty()) {
        return false;
    }

    if (!m_presetService->saveUserPreset(typeId, presetName, *effect)) {
        return false;
    }

    // Show what was just saved as the selected preset: the user named this patch, so the dropdown
    // has to agree that this is the patch they are on.
    if (const auto index = effectUserPresetNames(effectIndex).indexOf(presetName); index >= 0) {
        m_presetEffectIndex = static_cast<int>(effectIndex);
        m_presetIndex = effectFactoryPresetCount(effectIndex) + static_cast<int>(index);
    }

    emit effectPresetsChanged(effectIndex);

    return true;
}

bool EffectRackController::deleteCurrentEffectUserPreset(quint32 effectIndex)
{
    if (!currentEffectPresetIsUserPreset(effectIndex) || !m_presetService) {
        return false;
    }

    if (!m_presetService->deleteUserPreset(effectTypeId(effectIndex), currentEffectUserPresetName(effectIndex))) {
        return false;
    }

    // The list just got shorter under the selection. Landing on the preset that took the deleted
    // one's place keeps the dropdown from pointing past the end of itself.
    const auto presetCount = effectFactoryPresetCount(effectIndex) + static_cast<int>(effectUserPresetNames(effectIndex).size());
    m_presetIndex = std::clamp(m_presetIndex, 0, std::max(0, presetCount - 1));

    emit effectPresetsChanged(effectIndex);

    return true;
}

void EffectRackController::snapshotEffect(int effectIndex)
{
    // An effect carries no memory of which preset it was loaded from, and after any edit it is no
    // longer that preset anyway, so a dialog opens on the first entry rather than on a stale name.
    m_presetEffectIndex = effectIndex;
    m_presetIndex = 0;
    emit effectPresetsChanged(static_cast<quint32>(effectIndex));

    m_snapshot.reset();
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            m_snapshot = EffectSnapshot { effectIndex, effect->parameterSnapshot(), effect->enabled() };
        }
    }
}

void EffectRackController::revertEffect(int effectIndex)
{
    if (!m_snapshot || m_snapshot->effectIndex != effectIndex) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            effect->restoreParameterSnapshot(m_snapshot->parameters);
            effect->setEnabled(m_snapshot->enabled);
            effect->sync();
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
            emit parameterChanged(static_cast<quint32>(effectIndex), ""); // Notify all parameters changed
        }
    }
    m_snapshot.reset();
}

void EffectRackController::exportEffectSettings(int index, const QUrl & fileUrl)
{
    auto filePath = fileUrl.toLocalFile();
    if (!filePath.endsWith(Constants::effectSettingsExtension())) {
        filePath += Constants::effectSettingsExtension();
    }
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    if (const auto rack = currentRack(); rack) {
        rack->get().exportEffectSettings(static_cast<size_t>(index), writer);
    }
}

void EffectRackController::importEffectSettings(int index, const QUrl & fileUrl)
{
    const auto fileInfo = peekEffectTypeInfo(fileUrl);
    const auto rack = currentRack();
    if (!rack) {
        return;
    }

    const auto currentEff = rack->get().effect(static_cast<size_t>(index));
    const auto currentType = currentEff ? QString::fromStdString(currentEff->type()) : QString {};
    const auto currentTypeId = currentEff ? QString::fromStdString(currentEff->typeId()) : QString {};
    const bool typeMismatch = currentEff && !fileInfo.typeId.isEmpty() && currentTypeId != fileInfo.typeId;

    emit importEffectSettingsConfirmationRequested(index, fileUrl, currentType, fileInfo.typeName, typeMismatch);
}

void EffectRackController::confirmImportEffectSettings(int index, const QUrl & fileUrl)
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    NahdXmlReader reader { file };
    if (const auto rack = currentRack(); rack) {
        if (rack->get().importEffectSettings(static_cast<size_t>(index), reader)) {
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

EffectRackController::EffectTypeInfo EffectRackController::peekEffectTypeInfo(const QUrl & fileUrl) const
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    NahdXmlReader reader { file };
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
                return {
                    reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString(),
                    reader.attribute(Constants::NahdXml::xmlKeyType()).toString()
                };
            }
        }
    }
    return {};
}

void EffectRackController::exportSettings(const QUrl & fileUrl)
{
    auto filePath = fileUrl.toLocalFile();
    if (!filePath.endsWith(Constants::effectRackSettingsExtension())) {
        filePath += Constants::effectRackSettingsExtension();
    }
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);
    exportSettings(writer);
}

bool EffectRackController::exportSettings(ProjectWriter & writer) const
{
    const auto rack = currentRack();
    if (!rack) {
        return false;
    }
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));
    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    rack->get().serializeEffectsToXml(writer);
    writer.writeEndElement(); // InsertEffects
    writer.writeEndElement(); // Settings
    writer.writeEndDocument();
    return true;
}

void EffectRackController::importSettings(const QUrl & fileUrl)
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    NahdXmlReader reader { file };
    if (importSettings(reader)) {
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
        emit effectCountChanged();
    }
}

bool EffectRackController::importSettings(ProjectReader & reader)
{
    const auto rack = currentRack();
    if (!rack) {
        return false;
    }
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                rack->get().deserializeEffectsFromXml(reader);
                // The file may have come from this very project, whose effects still hold its ids.
                rack->get().renewEffectIds();
                return !reader.hasError();
            }
        }
    }
    return false;
}

float EffectRackController::deviceSend(const QString & deviceName, quint32 effectIndex) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->reverbSend(effectIndex);
    }
    return 0.0f;
}

void EffectRackController::setDeviceSend(const QString & deviceName, quint32 effectIndex, float send)
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setReverbSend(effectIndex, send);
    }
}

float EffectRackController::partSend(const QString & deviceName, int subIndex, quint32 effectIndex) const
{
    if (subIndex < 0) {
        return deviceSend(deviceName, effectIndex);
    }
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->sendSourceLevel(static_cast<size_t>(subIndex), effectIndex);
    }
    return 0.0f;
}

void EffectRackController::setPartSend(const QString & deviceName, int subIndex, quint32 effectIndex, float send)
{
    if (subIndex < 0) {
        setDeviceSend(deviceName, effectIndex, send);
        return;
    }
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setSendSourceLevel(static_cast<size_t>(subIndex), effectIndex, send);
    }
}

bool EffectRackController::hasPartSends(const QString & deviceName) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->sendSourceCount() > 0;
    }
    return false;
}

int EffectRackController::masterSendCount() const
{
    return static_cast<int>(Constants::effectRackSize());
}

QString EffectRackController::masterSendEffectType(quint32 busIndex) const
{
    if (const auto effect = m_deviceService->sendEffectRack().effect(busIndex); effect) {
        return QString::fromStdString(effect->type());
    }
    return {};
}

bool EffectRackController::hasMasterSendEffects() const
{
    return m_deviceService->sendEffectRack().hasEffects();
}

QString EffectRackController::openSendBusEffect(quint32 busIndex)
{
    // The bus's effect lives in the master send rack, so the controller is pointed there before the
    // dialog is asked to open it: every effect dialog reads the rack the controller is targeting.
    setTargetDeviceName({});
    setIsInsertRack(false);
    setTargetSubIndex(-1);

    return effectType(busIndex);
}

} // namespace noteahead
