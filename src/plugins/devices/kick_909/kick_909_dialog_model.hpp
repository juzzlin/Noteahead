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

#ifndef KICK_909_DIALOG_MODEL_HPP
#define KICK_909_DIALOG_MODEL_HPP

#include "../../../view/controllers/device_dialog_model.hpp"

#include <QTimer>
#include <QVariantList>

#include <memory>

namespace noteahead {

class Kick909Device;

//! The Kick 909 as its dialog sees it: one property per knob, scaled by
//! Constants::uiInternalScaling(), and one hit of the drum rendered for the waveform view.
class Kick909DialogModel : public DeviceDialogModel
{
    Q_OBJECT

    // Every knob changes with the others on a preset or a reload, so they share one signal
    Q_PROPERTY(int tune READ tune WRITE setTune NOTIFY knobsChanged)
    Q_PROPERTY(int attack READ attack WRITE setAttack NOTIFY knobsChanged)
    Q_PROPERTY(int decay READ decay WRITE setDecay NOTIFY knobsChanged)
    Q_PROPERTY(int pitch READ pitch WRITE setPitch NOTIFY knobsChanged)
    Q_PROPERTY(int pitchDepth READ pitchDepth WRITE setPitchDepth NOTIFY knobsChanged)
    Q_PROPERTY(int drive READ drive WRITE setDrive NOTIFY knobsChanged)
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY knobsChanged)
    Q_PROPERTY(int ampHold READ ampHold WRITE setAmpHold NOTIFY knobsChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY knobsChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY knobsChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY knobsChanged)
    Q_PROPERTY(int ampCurve READ ampCurve WRITE setAmpCurve NOTIFY knobsChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY knobsChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY knobsChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY knobsChanged)
    Q_PROPERTY(bool chromatic READ chromatic WRITE setChromatic NOTIFY knobsChanged)

    // One hit, rendered for the waveform view, with the envelope it was rendered with: the overlay
    // never runs ahead of the waveform under it
    Q_PROPERTY(QVariantList waveformData READ waveformData NOTIFY waveformChanged)
    Q_PROPERTY(double waveformDuration READ waveformDuration NOTIFY waveformChanged)
    Q_PROPERTY(double audibleLength READ audibleLength NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeAttack READ waveformEnvelopeAttack NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeHold READ waveformEnvelopeHold NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeDecay READ waveformEnvelopeDecay NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeSustain READ waveformEnvelopeSustain NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeRelease READ waveformEnvelopeRelease NOTIFY waveformChanged)
    Q_PROPERTY(double waveformEnvelopeCurve READ waveformEnvelopeCurve NOTIFY waveformChanged)

public:
    explicit Kick909DialogModel(std::shared_ptr<Kick909Device> device, QObject * parent = nullptr);
    ~Kick909DialogModel() override;

    DeviceDialogModel::DeviceS device() const override;
    bool setDevice(DeviceDialogModel::DeviceS device) override;
    void setDevice(std::shared_ptr<Kick909Device> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int tune() const;
    void setTune(int value);
    int attack() const;
    void setAttack(int value);
    int decay() const;
    void setDecay(int value);
    int pitch() const;
    void setPitch(int value);
    int pitchDepth() const;
    void setPitchDepth(int value);
    int drive() const;
    void setDrive(int value);
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
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);
    bool chromatic() const;
    void setChromatic(bool value);

    QVariantList waveformData() const;
    double waveformDuration() const;
    double audibleLength() const;
    double waveformEnvelopeAttack() const;
    double waveformEnvelopeHold() const;
    double waveformEnvelopeDecay() const;
    double waveformEnvelopeSustain() const;
    double waveformEnvelopeRelease() const;
    double waveformEnvelopeCurve() const;
    //! How many points the waveform view has room for, and whether it is on screen: the drum is only
    //! rendered while someone can see it
    Q_INVOKABLE void setWaveformRequest(int peakCount, bool visible);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void knobsChanged();
    void waveformChanged();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    //! Waits for the knobs to settle before rendering: a drag would otherwise render on every step
    void scheduleWaveform(int delayMs = WaveformDebounceMs);
    void renderWaveform();

    static constexpr int WaveformDebounceMs { 150 };

    QTimer m_waveformTimer;
    int m_waveformPeakCount { 0 };
    bool m_waveformVisible { false };
    QVariantList m_waveformData;
    double m_waveformDuration { 0.0 };
    double m_audibleLength { 0.0 };

    struct WaveformEnvelope
    {
        double attack {};
        double hold {};
        double decay {};
        double sustain { 1.0 };
        double release {};
        double curve {};
    };

    WaveformEnvelope m_waveformEnvelope;

    std::shared_ptr<Kick909Device> m_device;
};

} // namespace noteahead

#endif // KICK_909_DIALOG_MODEL_HPP
