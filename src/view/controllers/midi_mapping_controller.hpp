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

#ifndef MIDI_MAPPING_CONTROLLER_HPP
#define MIDI_MAPPING_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace noteahead {

class DeviceService;
class EditorService;

//! A device's MIDI mappings, by rack slot, and the controller numbers a mapping that is learning
//! hears.
class MidiMappingController : public QObject
{
    Q_OBJECT

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;

    MidiMappingController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);
    ~MidiMappingController() override;

    //! The device's controller mappings as { controller, target, min, max }, min and max being knob
    //! positions 0..1.
    Q_INVOKABLE QVariantList deviceMidiMappings(int slotIndex) const;
    Q_INVOKABLE void setDeviceMidiMappings(int slotIndex, const QVariantList & mappings);
    //! Highest CC a mapping can take.
    Q_INVOKABLE int maxMappableMidiCc() const;
    //! For MIDI Learn: a controller CC came in from the controller keyboard.
    void notifyControllerCcReceived(int controller);

signals:
    //! A CC came in from the controller keyboard, for a mapping that is learning its controller.
    void controllerCcReceived(int controller);

private:
    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // MIDI_MAPPING_CONTROLLER_HPP
