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

#include "property_service.hpp"

#include "../../common/constants.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../../domain/devices/drum_synth_device.hpp"

#include <QVariantMap>

namespace noteahead {

PropertyService::PropertyService(QObject * parent)
  : QObject { parent }
{
}

QVariantList PropertyService::availableMidiControllers() const
{
    return getAvailableMidiControllers();
}

QVariantList PropertyService::getAvailableMidiControllers(const QString & portName) const
{
    using namespace MidiCcMapping;
    QVariantList list;

    const auto addController = [&](uint8_t i, const QString & customName = {}) {
        QString name;
        if (!customName.isEmpty()) {
            name = QString { "%1: %2" }.arg(i).arg(customName);
        } else {
            name = controllerToString(static_cast<Controller>(i));
            if (name == "Undefined") {
                name = QString { "%1" }.arg(i);
            } else {
                name = QString { "%1: %2" }.arg(i).arg(name);
            }
        }
        list.append(QVariantMap {
            { "number", i },
            { "name", name },
            { "minValue", 0 },
            { "maxValue", 127 }
        });
    };

    if (!portName.isEmpty()) {
        if (portName.startsWith(Constants::samplerDeviceName()) || portName.startsWith(Constants::synthDeviceName())) {
            addController(static_cast<uint8_t>(Controller::ChannelVolumeMSB));
            addController(static_cast<uint8_t>(Controller::PanMSB));
            addController(static_cast<uint8_t>(Controller::SoundController5)); // LPF
            addController(81); // HPF
            return list;
        } else if (portName.startsWith(Constants::drumSynthDeviceName())) {
            addController(static_cast<uint8_t>(Controller::ChannelVolumeMSB));
            addController(static_cast<uint8_t>(Controller::PanMSB));

            // Range 1: Voices 0-5
            for (int voice { 0 }; voice < DrumSynth::NumVoicesRange1; voice++) {
                const uint8_t baseCc = DrumSynth::CcStartRange1 + (voice * 3);
                const QString voiceName = DrumSynth::voiceName(voice);
                addController(baseCc, voiceName + " Pan");
                addController(baseCc + 1, voiceName + " LPF");
                addController(baseCc + 2, voiceName + " HPF");
            }

            // Range 2: Voices 6-10
            for (int voice { 0 }; voice < DrumSynth::NumVoicesRange2; voice++) {
                const uint8_t baseCc = DrumSynth::CcStartRange2 + (voice * 3);
                const QString voiceName = DrumSynth::voiceName(static_cast<int>(DrumSynth::NumVoicesRange1) + voice);
                addController(baseCc, voiceName + " Pan");
                addController(baseCc + 1, voiceName + " LPF");
                addController(baseCc + 2, voiceName + " HPF");
            }
            return list;
        }
    }

    for (uint8_t i { 0 }; i < 128; ++i) {
        addController(i);
    }
    return list;
}

int PropertyService::minValue(int controller) const
{
    Q_UNUSED(controller)

    return 0;
}

int PropertyService::maxValue(int controller) const
{
    Q_UNUSED(controller)

    return 127;
}

} // namespace noteahead
