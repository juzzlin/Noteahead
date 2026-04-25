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

#ifndef DEVICE_SERVICE_HPP
#define DEVICE_SERVICE_HPP

#include <QObject>
#include <memory>
#include <string>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class AudioEngine;
class Device;

class DeviceService : public QObject
{
    Q_OBJECT

public:
    using AudioEngineS = std::shared_ptr<AudioEngine>;
    explicit DeviceService(AudioEngineS audioEngine, QObject * parent = nullptr);
    ~DeviceService() override;

    using DeviceS = std::shared_ptr<Device>;
    void registerDevice(DeviceS device);
    void unregisterDevice(const std::string & name);
    DeviceS device(const std::string & name) const;

    bool isInternalDevice(const QString & portName) const;
    void processMidiNoteOn(const QString & portName, uint8_t note, uint8_t velocity);
    void processMidiNoteOff(const QString & portName, uint8_t note);
    void processMidiCc(const QString & portName, uint8_t controller, uint8_t value);
    void processMidiAllNotesOff(const QString & portName);

    QStringList internalDeviceNames() const;

    void setProjectPath(const QString & projectPath);

    void serializeToXml(QXmlStreamWriter & writer) const;
    void deserializeFromXml(QXmlStreamReader & reader);

private:
    AudioEngineS m_audioEngine;
};

} // namespace noteahead

#endif // DEVICE_SERVICE_HPP
