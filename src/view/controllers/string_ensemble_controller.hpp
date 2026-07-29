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

#ifndef STRING_ENSEMBLE_CONTROLLER_HPP
#define STRING_ENSEMBLE_CONTROLLER_HPP

#include "device_controller.hpp"
#include <memory>

namespace noteahead {

class StringEnsembleDevice;

class StringEnsembleController : public DeviceController
{
    Q_OBJECT

    Q_PROPERTY(bool contrabassEnabled READ contrabassEnabled WRITE setContrabassEnabled NOTIFY contrabassEnabledChanged)
    Q_PROPERTY(bool celloEnabled READ celloEnabled WRITE setCelloEnabled NOTIFY celloEnabledChanged)
    Q_PROPERTY(bool violaEnabled READ violaEnabled WRITE setViolaEnabled NOTIFY violaEnabledChanged)
    Q_PROPERTY(bool violinEnabled READ violinEnabled WRITE setViolinEnabled NOTIFY violinEnabledChanged)
    Q_PROPERTY(bool trumpetEnabled READ trumpetEnabled WRITE setTrumpetEnabled NOTIFY trumpetEnabledChanged)
    Q_PROPERTY(bool hornEnabled READ hornEnabled WRITE setHornEnabled NOTIFY hornEnabledChanged)

    Q_PROPERTY(bool modulationEnabled READ modulationEnabled WRITE setModulationEnabled NOTIFY modulationEnabledChanged)
    Q_PROPERTY(bool phaserEnabled READ phaserEnabled WRITE setPhaserEnabled NOTIFY phaserEnabledChanged)

    Q_PROPERTY(int volumeBass READ volumeBass WRITE setVolumeBass NOTIFY volumeBassChanged)
    Q_PROPERTY(int crescendo READ crescendo WRITE setCrescendo NOTIFY crescendoChanged)
    Q_PROPERTY(int sustainLength READ sustainLength WRITE setSustainLength NOTIFY sustainLengthChanged)
    Q_PROPERTY(int phaserColor READ phaserColor WRITE setPhaserColor NOTIFY phaserColorChanged)
    Q_PROPERTY(int phaserRate READ phaserRate WRITE setPhaserRate NOTIFY phaserRateChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY velocitySensitivityChanged)

    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)

public:
    explicit StringEnsembleController(std::shared_ptr<StringEnsembleDevice> device, QObject * parent = nullptr);
    ~StringEnsembleController() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    bool contrabassEnabled() const;
    void setContrabassEnabled(bool value);

    bool celloEnabled() const;
    void setCelloEnabled(bool value);

    bool violaEnabled() const;
    void setViolaEnabled(bool value);

    bool violinEnabled() const;
    void setViolinEnabled(bool value);

    bool trumpetEnabled() const;
    void setTrumpetEnabled(bool value);

    bool hornEnabled() const;
    void setHornEnabled(bool value);

    bool modulationEnabled() const;
    void setModulationEnabled(bool value);

    bool phaserEnabled() const;
    void setPhaserEnabled(bool value);

    int volumeBass() const;
    void setVolumeBass(int value);

    int crescendo() const;
    void setCrescendo(int value);

    int sustainLength() const;
    void setSustainLength(int value);

    int phaserColor() const;
    void setPhaserColor(int value);

    int phaserRate() const;
    void setPhaserRate(int value);

    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);

    int lpfCutoff() const;
    void setLpfCutoff(int value);

    int hpfCutoff() const;
    void setHpfCutoff(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void contrabassEnabledChanged();
    void celloEnabledChanged();
    void violaEnabledChanged();
    void violinEnabledChanged();
    void trumpetEnabledChanged();
    void hornEnabledChanged();
    void modulationEnabledChanged();
    void phaserEnabledChanged();
    void volumeBassChanged();
    void crescendoChanged();
    void sustainLengthChanged();
    void phaserColorChanged();
    void phaserRateChanged();
    void velocitySensitivityChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();

public:
    void setDevice(std::shared_ptr<StringEnsembleDevice> device);

private:
    std::shared_ptr<StringEnsembleDevice> m_device;
};

} // namespace noteahead

#endif // STRING_ENSEMBLE_CONTROLLER_HPP
