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

#ifndef SYNTH_D_CONTROLLER_HPP
#define SYNTH_D_CONTROLLER_HPP

#include "device_controller.hpp"

#include <memory>

namespace noteahead {

class SynthDDevice;

//! Synth D as its dialog sees it: one property per control. Knobs are ints scaled by
//! Constants::uiInternalScaling(), selectors are the ordinal of their position, switches are bools.
class SynthDController : public DeviceController
{
    Q_OBJECT

    // Every control changes with the others on a preset or a reload, so they share one signal
    Q_PROPERTY(int tune READ tune WRITE setTune NOTIFY controlsChanged)
    Q_PROPERTY(int glide READ glide WRITE setGlide NOTIFY controlsChanged)
    Q_PROPERTY(int modMix READ modMix WRITE setModMix NOTIFY controlsChanged)
    Q_PROPERTY(int modDepth READ modDepth WRITE setModDepth NOTIFY controlsChanged)
    Q_PROPERTY(int modSource1 READ modSource1 WRITE setModSource1 NOTIFY controlsChanged)
    Q_PROPERTY(int modSource2 READ modSource2 WRITE setModSource2 NOTIFY controlsChanged)
    Q_PROPERTY(bool oscillatorModulation READ oscillatorModulation WRITE setOscillatorModulation NOTIFY controlsChanged)
    Q_PROPERTY(bool filterModulation READ filterModulation WRITE setFilterModulation NOTIFY controlsChanged)
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY controlsChanged)
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY controlsChanged)
    Q_PROPERTY(int osc1Range READ osc1Range WRITE setOsc1Range NOTIFY controlsChanged)
    Q_PROPERTY(int osc2Range READ osc2Range WRITE setOsc2Range NOTIFY controlsChanged)
    Q_PROPERTY(int osc3Range READ osc3Range WRITE setOsc3Range NOTIFY controlsChanged)
    Q_PROPERTY(int osc1Waveform READ osc1Waveform WRITE setOsc1Waveform NOTIFY controlsChanged)
    Q_PROPERTY(int osc2Waveform READ osc2Waveform WRITE setOsc2Waveform NOTIFY controlsChanged)
    Q_PROPERTY(int osc3Waveform READ osc3Waveform WRITE setOsc3Waveform NOTIFY controlsChanged)
    Q_PROPERTY(int osc2Frequency READ osc2Frequency WRITE setOsc2Frequency NOTIFY controlsChanged)
    Q_PROPERTY(int osc3Frequency READ osc3Frequency WRITE setOsc3Frequency NOTIFY controlsChanged)
    Q_PROPERTY(bool osc3KeyboardControl READ osc3KeyboardControl WRITE setOsc3KeyboardControl NOTIFY controlsChanged)
    Q_PROPERTY(int osc1Volume READ osc1Volume WRITE setOsc1Volume NOTIFY controlsChanged)
    Q_PROPERTY(int osc2Volume READ osc2Volume WRITE setOsc2Volume NOTIFY controlsChanged)
    Q_PROPERTY(int osc3Volume READ osc3Volume WRITE setOsc3Volume NOTIFY controlsChanged)
    Q_PROPERTY(bool osc1On READ osc1On WRITE setOsc1On NOTIFY controlsChanged)
    Q_PROPERTY(bool osc2On READ osc2On WRITE setOsc2On NOTIFY controlsChanged)
    Q_PROPERTY(bool osc3On READ osc3On WRITE setOsc3On NOTIFY controlsChanged)
    Q_PROPERTY(int extVolume READ extVolume WRITE setExtVolume NOTIFY controlsChanged)
    Q_PROPERTY(bool extOn READ extOn WRITE setExtOn NOTIFY controlsChanged)
    Q_PROPERTY(int noiseVolume READ noiseVolume WRITE setNoiseVolume NOTIFY controlsChanged)
    Q_PROPERTY(bool noiseOn READ noiseOn WRITE setNoiseOn NOTIFY controlsChanged)
    Q_PROPERTY(int noiseColour READ noiseColour WRITE setNoiseColour NOTIFY controlsChanged)
    Q_PROPERTY(int filterMode READ filterMode WRITE setFilterMode NOTIFY controlsChanged)
    Q_PROPERTY(int cutoff READ cutoff WRITE setCutoff NOTIFY controlsChanged)
    Q_PROPERTY(int emphasis READ emphasis WRITE setEmphasis NOTIFY controlsChanged)
    Q_PROPERTY(int contourAmount READ contourAmount WRITE setContourAmount NOTIFY controlsChanged)
    Q_PROPERTY(bool keyboardControl1 READ keyboardControl1 WRITE setKeyboardControl1 NOTIFY controlsChanged)
    Q_PROPERTY(bool keyboardControl2 READ keyboardControl2 WRITE setKeyboardControl2 NOTIFY controlsChanged)
    Q_PROPERTY(int filterAttack READ filterAttack WRITE setFilterAttack NOTIFY controlsChanged)
    Q_PROPERTY(int filterDecay READ filterDecay WRITE setFilterDecay NOTIFY controlsChanged)
    Q_PROPERTY(int filterSustain READ filterSustain WRITE setFilterSustain NOTIFY controlsChanged)
    Q_PROPERTY(bool filterDecaySwitch READ filterDecaySwitch WRITE setFilterDecaySwitch NOTIFY controlsChanged)
    Q_PROPERTY(int loudnessAttack READ loudnessAttack WRITE setLoudnessAttack NOTIFY controlsChanged)
    Q_PROPERTY(int loudnessDecay READ loudnessDecay WRITE setLoudnessDecay NOTIFY controlsChanged)
    Q_PROPERTY(int loudnessSustain READ loudnessSustain WRITE setLoudnessSustain NOTIFY controlsChanged)
    Q_PROPERTY(bool loudnessDecaySwitch READ loudnessDecaySwitch WRITE setLoudnessDecaySwitch NOTIFY controlsChanged)
    Q_PROPERTY(int notePriority READ notePriority WRITE setNotePriority NOTIFY controlsChanged)
    Q_PROPERTY(bool multiTrigger READ multiTrigger WRITE setMultiTrigger NOTIFY controlsChanged)
    Q_PROPERTY(int pitchBendRange READ pitchBendRange WRITE setPitchBendRange NOTIFY controlsChanged)
    Q_PROPERTY(int drift READ drift WRITE setDrift NOTIFY controlsChanged)
    Q_PROPERTY(int instability READ instability WRITE setInstability NOTIFY controlsChanged)
    Q_PROPERTY(int tolerance READ tolerance WRITE setTolerance NOTIFY controlsChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY controlsChanged)
    Q_PROPERTY(int velocitySensitivity READ velocitySensitivity WRITE setVelocitySensitivity NOTIFY controlsChanged)
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY controlsChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY controlsChanged)

public:
    explicit SynthDController(std::shared_ptr<SynthDDevice> device, QObject * parent = nullptr);
    ~SynthDController() override;

    DeviceController::DeviceS device() const override;
    bool setDevice(DeviceController::DeviceS device) override;
    void setDevice(std::shared_ptr<SynthDDevice> device);

    QStringList factoryPresetNames() const override;
    void loadFactoryPreset(int index) override;

    int tune() const;
    void setTune(int value);
    int glide() const;
    void setGlide(int value);
    int modMix() const;
    void setModMix(int value);
    int modDepth() const;
    void setModDepth(int value);
    int modSource1() const;
    void setModSource1(int value);
    int modSource2() const;
    void setModSource2(int value);
    bool oscillatorModulation() const;
    void setOscillatorModulation(bool value);
    bool filterModulation() const;
    void setFilterModulation(bool value);
    int lfoWaveform() const;
    void setLfoWaveform(int value);
    int lfoRate() const;
    void setLfoRate(int value);
    int osc1Range() const;
    void setOsc1Range(int value);
    int osc2Range() const;
    void setOsc2Range(int value);
    int osc3Range() const;
    void setOsc3Range(int value);
    int osc1Waveform() const;
    void setOsc1Waveform(int value);
    int osc2Waveform() const;
    void setOsc2Waveform(int value);
    int osc3Waveform() const;
    void setOsc3Waveform(int value);
    int osc2Frequency() const;
    void setOsc2Frequency(int value);
    int osc3Frequency() const;
    void setOsc3Frequency(int value);
    bool osc3KeyboardControl() const;
    void setOsc3KeyboardControl(bool value);
    int osc1Volume() const;
    void setOsc1Volume(int value);
    int osc2Volume() const;
    void setOsc2Volume(int value);
    int osc3Volume() const;
    void setOsc3Volume(int value);
    bool osc1On() const;
    void setOsc1On(bool value);
    bool osc2On() const;
    void setOsc2On(bool value);
    bool osc3On() const;
    void setOsc3On(bool value);
    int extVolume() const;
    void setExtVolume(int value);
    bool extOn() const;
    void setExtOn(bool value);
    int noiseVolume() const;
    void setNoiseVolume(int value);
    bool noiseOn() const;
    void setNoiseOn(bool value);
    int noiseColour() const;
    void setNoiseColour(int value);
    int filterMode() const;
    void setFilterMode(int value);
    int cutoff() const;
    void setCutoff(int value);
    int emphasis() const;
    void setEmphasis(int value);
    int contourAmount() const;
    void setContourAmount(int value);
    bool keyboardControl1() const;
    void setKeyboardControl1(bool value);
    bool keyboardControl2() const;
    void setKeyboardControl2(bool value);
    int filterAttack() const;
    void setFilterAttack(int value);
    int filterDecay() const;
    void setFilterDecay(int value);
    int filterSustain() const;
    void setFilterSustain(int value);
    bool filterDecaySwitch() const;
    void setFilterDecaySwitch(bool value);
    int loudnessAttack() const;
    void setLoudnessAttack(int value);
    int loudnessDecay() const;
    void setLoudnessDecay(int value);
    int loudnessSustain() const;
    void setLoudnessSustain(int value);
    bool loudnessDecaySwitch() const;
    void setLoudnessDecaySwitch(bool value);
    int notePriority() const;
    void setNotePriority(int value);
    bool multiTrigger() const;
    void setMultiTrigger(bool value);
    int pitchBendRange() const;
    void setPitchBendRange(int value);
    int drift() const;
    void setDrift(int value);
    int instability() const;
    void setInstability(int value);
    int tolerance() const;
    void setTolerance(int value);
    int panSpread() const;
    void setPanSpread(int value);
    int velocitySensitivity() const;
    void setVelocitySensitivity(int value);
    int lpfCutoff() const;
    void setLpfCutoff(int value);
    int hpfCutoff() const;
    void setHpfCutoff(int value);

    Q_INVOKABLE void requestSettings() override;

signals:
    void deviceChanged();
    void controlsChanged();

private:
    int knob(const QString & key) const;
    void setKnob(const QString & key, int value);
    int choice(const QString & key) const;
    void setChoice(const QString & key, int value);

    std::shared_ptr<SynthDDevice> m_device;
};

} // namespace noteahead

#endif // SYNTH_D_CONTROLLER_HPP
