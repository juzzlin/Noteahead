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

#ifndef STRING_VOICE_CONTROLLER_HPP
#define STRING_VOICE_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class StringVoiceDevice;

class StringVoiceController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(int stringsLevel8 READ stringsLevel8 WRITE setStringsLevel8 NOTIFY stringsLevel8Changed)
    Q_PROPERTY(int stringsLevel4 READ stringsLevel4 WRITE setStringsLevel4 NOTIFY stringsLevel4Changed)
    Q_PROPERTY(int stringsAttack READ stringsAttack WRITE setStringsAttack NOTIFY stringsAttackChanged)
    Q_PROPERTY(int stringsRelease READ stringsRelease WRITE setStringsRelease NOTIFY stringsReleaseChanged)

    Q_PROPERTY(int voiceMale8 READ voiceMale8 WRITE setVoiceMale8 NOTIFY voiceMale8Changed)
    Q_PROPERTY(int voiceMale4 READ voiceMale4 WRITE setVoiceMale4 NOTIFY voiceMale4Changed)
    Q_PROPERTY(int voiceFemale8 READ voiceFemale8 WRITE setVoiceFemale8 NOTIFY voiceFemale8Changed)
    Q_PROPERTY(int voiceFemale4 READ voiceFemale4 WRITE setVoiceFemale4 NOTIFY voiceFemale4Changed)
    Q_PROPERTY(int voiceAttack READ voiceAttack WRITE setVoiceAttack NOTIFY voiceAttackChanged)
    Q_PROPERTY(int voiceRelease READ voiceRelease WRITE setVoiceRelease NOTIFY voiceReleaseChanged)

    Q_PROPERTY(int vibratoRate READ vibratoRate WRITE setVibratoRate NOTIFY vibratoRateChanged)
    Q_PROPERTY(int vibratoDepth READ vibratoDepth WRITE setVibratoDepth NOTIFY vibratoDepthChanged)
    Q_PROPERTY(int vibratoDelay READ vibratoDelay WRITE setVibratoDelay NOTIFY vibratoDelayChanged)

    Q_PROPERTY(bool ensembleEnabled READ ensembleEnabled WRITE setEnsembleEnabled NOTIFY ensembleEnabledChanged)
    Q_PROPERTY(int ensembleMode READ ensembleMode WRITE setEnsembleMode NOTIFY ensembleModeChanged)

    Q_PROPERTY(bool vocoderEnabled READ vocoderEnabled WRITE setVocoderEnabled NOTIFY vocoderEnabledChanged)
    Q_PROPERTY(int vocoderSidechain READ vocoderSidechain WRITE setVocoderSidechain NOTIFY vocoderSidechainChanged)

    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)

public:
    explicit StringVoiceController(std::shared_ptr<StringVoiceDevice> device, QObject * parent = nullptr);
    ~StringVoiceController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int stringsLevel8() const;
    void setStringsLevel8(int value);

    int stringsLevel4() const;
    void setStringsLevel4(int value);

    int stringsAttack() const;
    void setStringsAttack(int value);

    int stringsRelease() const;
    void setStringsRelease(int value);

    int voiceMale8() const;
    void setVoiceMale8(int value);

    int voiceMale4() const;
    void setVoiceMale4(int value);

    int voiceFemale8() const;
    void setVoiceFemale8(int value);

    int voiceFemale4() const;
    void setVoiceFemale4(int value);

    int voiceAttack() const;
    void setVoiceAttack(int value);

    int voiceRelease() const;
    void setVoiceRelease(int value);

    int vibratoRate() const;
    void setVibratoRate(int value);

    int vibratoDepth() const;
    void setVibratoDepth(int value);

    int vibratoDelay() const;
    void setVibratoDelay(int value);

    bool ensembleEnabled() const;
    void setEnsembleEnabled(bool value);

    int ensembleMode() const;
    void setEnsembleMode(int value);

    bool vocoderEnabled() const;
    void setVocoderEnabled(bool value);

    int vocoderSidechain() const;
    void setVocoderSidechain(int value);

    int lpfCutoff() const;
    void setLpfCutoff(int value);

    int hpfCutoff() const;
    void setHpfCutoff(int value);

    int panSpread() const;
    void setPanSpread(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void stringsLevel8Changed();
    void stringsLevel4Changed();
    void stringsAttackChanged();
    void stringsReleaseChanged();
    void voiceMale8Changed();
    void voiceMale4Changed();
    void voiceFemale8Changed();
    void voiceFemale4Changed();
    void voiceAttackChanged();
    void voiceReleaseChanged();
    void vibratoRateChanged();
    void vibratoDepthChanged();
    void vibratoDelayChanged();
    void ensembleEnabledChanged();
    void ensembleModeChanged();
    void vocoderEnabledChanged();
    void vocoderSidechainChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();
    void panSpreadChanged();

public:
    void setDevice(std::shared_ptr<StringVoiceDevice> device);

private:
    std::shared_ptr<StringVoiceDevice> m_device;
};

} // namespace noteahead

#endif // STRING_VOICE_CONTROLLER_HPP
