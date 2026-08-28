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

#ifndef SPEECH_CONTROLLER_HPP
#define SPEECH_CONTROLLER_HPP

#include "device_controller.hpp"

#include <QString>
#include <memory>

namespace noteahead {

class SpeechDevice;

class SpeechController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int rate READ rate WRITE setRate NOTIFY rateChanged)
    Q_PROPERTY(int glide READ glide WRITE setGlide NOTIFY glideChanged)
    Q_PROPERTY(int formantShift READ formantShift WRITE setFormantShift NOTIFY formantShiftChanged)
    Q_PROPERTY(int breathiness READ breathiness WRITE setBreathiness NOTIFY breathinessChanged)
    Q_PROPERTY(int consonantLevel READ consonantLevel WRITE setConsonantLevel NOTIFY consonantLevelChanged)
    Q_PROPERTY(int sibilance READ sibilance WRITE setSibilance NOTIFY sibilanceChanged)
    Q_PROPERTY(int voiceType READ voiceType WRITE setVoiceType NOTIFY voiceTypeChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)
    Q_PROPERTY(int intonation READ intonation WRITE setIntonation NOTIFY intonationChanged)
    Q_PROPERTY(int vibratoRate READ vibratoRate WRITE setVibratoRate NOTIFY vibratoRateChanged)
    Q_PROPERTY(int vibratoDepth READ vibratoDepth WRITE setVibratoDepth NOTIFY vibratoDepthChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int triggerMode READ triggerMode WRITE setTriggerMode NOTIFY triggerModeChanged)
    Q_PROPERTY(int syncMode READ syncMode WRITE setSyncMode NOTIFY syncModeChanged)
    Q_PROPERTY(int syncLength READ syncLength WRITE setSyncLength NOTIFY syncLengthChanged)
    Q_PROPERTY(int syncDivision READ syncDivision WRITE setSyncDivision NOTIFY syncDivisionChanged)
    Q_PROPERTY(QString phrase READ phrase WRITE setPhrase NOTIFY phraseChanged)
    Q_PROPERTY(QString phrasePhonemes READ phrasePhonemes NOTIFY phraseChanged)
    Q_PROPERTY(int syllableCount READ syllableCount NOTIFY phraseChanged)

public:
    explicit SpeechController(std::shared_ptr<SpeechDevice> device, QObject * parent = nullptr);
    ~SpeechController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int rate() const;
    void setRate(int value);
    int glide() const;
    void setGlide(int value);
    int formantShift() const;
    void setFormantShift(int value);
    int breathiness() const;
    void setBreathiness(int value);
    int consonantLevel() const;
    void setConsonantLevel(int value);
    int sibilance() const;
    void setSibilance(int value);
    int voiceType() const;
    void setVoiceType(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int intonation() const;
    void setIntonation(int value);
    int vibratoRate() const;
    void setVibratoRate(int value);
    int vibratoDepth() const;
    void setVibratoDepth(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    int triggerMode() const;
    void setTriggerMode(int value);
    int syncMode() const;
    void setSyncMode(int value);
    int syncLength() const;
    void setSyncLength(int value);
    int syncDivision() const;
    void setSyncDivision(int value);

    QString phrase() const;
    void setPhrase(const QString & phrase);

    //! What the letter-to-sound rules made of the phrase. Read-only, and the reason the dialog can
    //! show the user why a word came out the way it did.
    QString phrasePhonemes() const;

    int syllableCount() const;

    Q_INVOKABLE void requestSettings() override;

signals:
    void rateChanged();
    void glideChanged();
    void formantShiftChanged();
    void breathinessChanged();
    void consonantLevelChanged();
    void sibilanceChanged();
    void voiceTypeChanged();
    void velocitySensitivityChanged();
    void intonationChanged();
    void vibratoRateChanged();
    void vibratoDepthChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();
    void triggerModeChanged();
    void syncModeChanged();
    void syncLengthChanged();
    void syncDivisionChanged();
    void phraseChanged();
    void deviceChanged();

private:
    void setDevice(std::shared_ptr<SpeechDevice> device);

    std::shared_ptr<SpeechDevice> m_device;
};

} // namespace noteahead

#endif // SPEECH_CONTROLLER_HPP
