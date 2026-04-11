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

#ifndef KICK_808_DIALOG_MODEL_HPP
#define KICK_808_DIALOG_MODEL_HPP

#include "../../../view/controllers/device_dialog_model.hpp"
#include "../../../view/controllers/drum_hit_waveform_model.hpp"
#include <memory>

namespace noteahead {

class Kick808Device;

class Kick808DialogModel : public DeviceDialogModel
{
    Q_OBJECT

    Q_PROPERTY(int tune READ tune WRITE setTune NOTIFY tuneChanged)
    Q_PROPERTY(int tone READ tone WRITE setTone NOTIFY toneChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY decayChanged)
    Q_PROPERTY(int pitchDepth READ pitchDepth WRITE setPitchDepth NOTIFY pitchDepthChanged)
    Q_PROPERTY(int pitchDecay READ pitchDecay WRITE setPitchDecay NOTIFY pitchDecayChanged)
    Q_PROPERTY(int drive READ drive WRITE setDrive NOTIFY driveChanged)
    Q_PROPERTY(int glide READ glide WRITE setGlide NOTIFY glideChanged)
    Q_PROPERTY(bool keyTrack READ keyTrack WRITE setKeyTrack NOTIFY keyTrackChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    // The amp envelope's knobs change together on a preset or a reload, so they share one signal
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY ampChanged)
    Q_PROPERTY(int ampHold READ ampHold WRITE setAmpHold NOTIFY ampChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY ampChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY ampChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY ampChanged)
    Q_PROPERTY(int ampCurve READ ampCurve WRITE setAmpCurve NOTIFY ampChanged)
    //! One hit rendered for the waveform view, with the envelope and the playhead over it
    Q_PROPERTY(QObject * waveform READ waveform CONSTANT)

public:
    explicit Kick808DialogModel(std::shared_ptr<Kick808Device> device, QObject * parent = nullptr);
    ~Kick808DialogModel() override;

    DeviceS device() const override;
    bool setDevice(DeviceS device) override;

    int tune() const;
    void setTune(int value);
    int tone() const;
    void setTone(int value);
    int decay() const;
    void setDecay(int value);
    int pitchDepth() const;
    void setPitchDepth(int value);
    int pitchDecay() const;
    void setPitchDecay(int value);
    int drive() const;
    void setDrive(int value);
    int glide() const;
    void setGlide(int value);
    bool keyTrack() const;
    void setKeyTrack(bool value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    int ampAttack() const;
    void setAmpAttack(int value);
    int ampHold() const;
    void setAmpHold(int value);
    int ampDecay() const;
    void setAmpDecay(int value);
    int ampSustain() const;
    void setAmpSustain(int value);
    int ampRelease() const;
    void setAmpRelease(int value);
    int ampCurve() const;
    void setAmpCurve(int value);

    QObject * waveform() const;

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void tuneChanged();
    void toneChanged();
    void decayChanged();
    void pitchDepthChanged();
    void pitchDecayChanged();
    void driveChanged();
    void glideChanged();
    void keyTrackChanged();
    void lpfCutoffChanged();
    void hpfCutoffChanged();
    void ampChanged();

public:
    void setDevice(std::shared_ptr<Kick808Device> device);

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);

    std::shared_ptr<Kick808Device> m_device;
    DrumHitWaveformModel * m_waveform { nullptr };
};

} // namespace noteahead

#endif // KICK_808_DIALOG_MODEL_HPP
