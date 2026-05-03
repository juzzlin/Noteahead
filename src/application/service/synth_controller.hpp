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

#ifndef SYNTH_CONTROLLER_HPP
#define SYNTH_CONTROLLER_HPP

#include <QObject>
#include <memory>

namespace noteahead {

class SynthDevice;

class SynthController : public QObject
{
    Q_OBJECT

    // VCO1
    Q_PROPERTY(int vco1Waveform READ vco1Waveform WRITE setVco1Waveform NOTIFY vco1WaveformChanged)
    Q_PROPERTY(int vco1Octave READ vco1Octave WRITE setVco1Octave NOTIFY vco1OctaveChanged)
    Q_PROPERTY(int vco1Pitch READ vco1Pitch WRITE setVco1Pitch NOTIFY vco1PitchChanged)
    Q_PROPERTY(int vco1Shape READ vco1Shape WRITE setVco1Shape NOTIFY vco1ShapeChanged)
    Q_PROPERTY(bool vco1Sync READ vco1Sync WRITE setVco1Sync NOTIFY vco1SyncChanged)

    // VCO2
    Q_PROPERTY(int vco2Waveform READ vco2Waveform WRITE setVco2Waveform NOTIFY vco2WaveformChanged)
    Q_PROPERTY(int vco2Octave READ vco2Octave WRITE setVco2Octave NOTIFY vco2OctaveChanged)
    Q_PROPERTY(int vco2Pitch READ vco2Pitch WRITE setVco2Pitch NOTIFY vco2PitchChanged)
    Q_PROPERTY(int vco2Shape READ vco2Shape WRITE setVco2Shape NOTIFY vco2ShapeChanged)
    Q_PROPERTY(bool vco2Sync READ vco2Sync WRITE setVco2Sync NOTIFY vco2SyncChanged)

    // Multi Engine
    Q_PROPERTY(int multiType READ multiType WRITE setMultiType NOTIFY multiTypeChanged)
    Q_PROPERTY(int multiShape READ multiShape WRITE setMultiShape NOTIFY multiShapeChanged)
    Q_PROPERTY(int multiLevel READ multiLevel WRITE setMultiLevel NOTIFY multiLevelChanged)
    Q_PROPERTY(int multiKeyTrack READ multiKeyTrack WRITE setMultiKeyTrack NOTIFY multiKeyTrackChanged)

    // Mixer
    Q_PROPERTY(int mixVco1 READ mixVco1 WRITE setMixVco1 NOTIFY mixVco1Changed)
    Q_PROPERTY(int mixVco2 READ mixVco2 WRITE setMixVco2 NOTIFY mixVco2Changed)

    // Filter
    Q_PROPERTY(int lpfCutoff READ lpfCutoff WRITE setLpfCutoff NOTIFY lpfCutoffChanged)
    Q_PROPERTY(int lpfResonance READ lpfResonance WRITE setLpfResonance NOTIFY lpfResonanceChanged)
    Q_PROPERTY(int hpfCutoff READ hpfCutoff WRITE setHpfCutoff NOTIFY hpfCutoffChanged)
    Q_PROPERTY(int filterKeyTrack READ filterKeyTrack WRITE setFilterKeyTrack NOTIFY filterKeyTrackChanged)

    // Amp EG
    Q_PROPERTY(int ampAttack READ ampAttack WRITE setAmpAttack NOTIFY ampAttackChanged)
    Q_PROPERTY(int ampDecay READ ampDecay WRITE setAmpDecay NOTIFY ampDecayChanged)
    Q_PROPERTY(int ampSustain READ ampSustain WRITE setAmpSustain NOTIFY ampSustainChanged)
    Q_PROPERTY(int ampRelease READ ampRelease WRITE setAmpRelease NOTIFY ampReleaseChanged)

    // Mod EG
    Q_PROPERTY(int modAttack READ modAttack WRITE setModAttack NOTIFY modAttackChanged)
    Q_PROPERTY(int modDecay READ modDecay WRITE setModDecay NOTIFY modDecayChanged)
    Q_PROPERTY(int modInt READ modInt WRITE setModInt NOTIFY modIntChanged)
    Q_PROPERTY(int modTarget READ modTarget WRITE setModTarget NOTIFY modTargetChanged)

    // LFO
    Q_PROPERTY(int lfoWaveform READ lfoWaveform WRITE setLfoWaveform NOTIFY lfoWaveformChanged)
    Q_PROPERTY(int lfoMode READ lfoMode WRITE setLfoMode NOTIFY lfoModeChanged)
    Q_PROPERTY(int lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(int lfoInt READ lfoInt WRITE setLfoInt NOTIFY lfoIntChanged)
    Q_PROPERTY(int lfoTarget READ lfoTarget WRITE setLfoTarget NOTIFY lfoTargetChanged)

    // Global
    Q_PROPERTY(int voiceMode READ voiceMode WRITE setVoiceMode NOTIFY voiceModeChanged)
    Q_PROPERTY(int voiceDepth READ voiceDepth WRITE setVoiceDepth NOTIFY voiceDepthChanged)
    Q_PROPERTY(int portamento READ portamento WRITE setPortamento NOTIFY portamentoChanged)
    Q_PROPERTY(int panSpread READ panSpread WRITE setPanSpread NOTIFY panSpreadChanged)
    Q_PROPERTY(int masterVolume READ masterVolume WRITE setMasterVolume NOTIFY masterVolumeChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames CONSTANT)

    // Delay
    Q_PROPERTY(int delayType READ delayType WRITE setDelayType NOTIFY delayTypeChanged)
    Q_PROPERTY(int delayTime READ delayTime WRITE setDelayTime NOTIFY delayTimeChanged)
    Q_PROPERTY(int delayFeedback READ delayFeedback WRITE setDelayFeedback NOTIFY delayFeedbackChanged)
    Q_PROPERTY(int delayDepth READ delayDepth WRITE setDelayDepth NOTIFY delayDepthChanged)
    Q_PROPERTY(int delayMix READ delayMix WRITE setDelayMix NOTIFY delayMixChanged)
    Q_PROPERTY(bool delaySync READ delaySync WRITE setDelaySync NOTIFY delaySyncChanged)
    Q_PROPERTY(int delaySyncDivision READ delaySyncDivision WRITE setDelaySyncDivision NOTIFY delaySyncDivisionChanged)

public:
    explicit SynthController(std::shared_ptr<SynthDevice> synth, QObject * parent = nullptr);
    ~SynthController() override;

    std::shared_ptr<SynthDevice> synth() const;

    // Accessors
    int vco1Waveform() const; void setVco1Waveform(int wave);
    int vco1Octave() const; void setVco1Octave(int oct);
    int vco1Pitch() const; void setVco1Pitch(int p);
    int vco1Shape() const; void setVco1Shape(int s);
    bool vco1Sync() const; void setVco1Sync(bool s);

    int vco2Waveform() const; void setVco2Waveform(int wave);
    int vco2Octave() const; void setVco2Octave(int oct);
    int vco2Pitch() const; void setVco2Pitch(int p);
    int vco2Shape() const; void setVco2Shape(int s);
    bool vco2Sync() const; void setVco2Sync(bool s);

    int multiType() const; void setMultiType(int type);
    int multiShape() const; void setMultiShape(int s);
    int multiLevel() const; void setMultiLevel(int lvl);
    int multiKeyTrack() const; void setMultiKeyTrack(int t);

    int mixVco1() const; void setMixVco1(int lvl);
    int mixVco2() const; void setMixVco2(int lvl);

    int lpfCutoff() const; void setLpfCutoff(int c);
    int lpfResonance() const; void setLpfResonance(int r);
    int hpfCutoff() const; void setHpfCutoff(int c);
    int filterKeyTrack() const; void setFilterKeyTrack(int t);

    int ampAttack() const; void setAmpAttack(int a);
    int ampDecay() const; void setAmpDecay(int d);
    int ampSustain() const; void setAmpSustain(int s);
    int ampRelease() const; void setAmpRelease(int r);

    int modAttack() const; void setModAttack(int a);
    int modDecay() const; void setModDecay(int d);
    int modInt() const; void setModInt(int i);
    int modTarget() const; void setModTarget(int t);

    int lfoWaveform() const; void setLfoWaveform(int wave);
    int lfoMode() const; void setLfoMode(int mode);
    int lfoRate() const; void setLfoRate(int rate);
    int lfoInt() const; void setLfoInt(int intensity);
    int lfoTarget() const; void setLfoTarget(int target);

    int voiceMode() const; void setVoiceMode(int m);
    int voiceDepth() const; void setVoiceDepth(int d);
    int portamento() const; void setPortamento(int p);
    int panSpread() const; void setPanSpread(int s);
    int masterVolume() const; void setMasterVolume(int v);

    QStringList presetNames() const;

    int delayType() const; void setDelayType(int type);
    int delayTime() const; void setDelayTime(int time);
    int delayFeedback() const; void setDelayFeedback(int fb);
    int delayDepth() const; void setDelayDepth(int d);
    int delayMix() const; void setDelayMix(int mix);
    bool delaySync() const; void setDelaySync(bool sync);
    int delaySyncDivision() const; void setDelaySyncDivision(int div);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void requestSettings();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();
    Q_INVOKABLE void loadPreset(int index);

    Q_INVOKABLE void playNote(int note, double velocity = 1.0);
    Q_INVOKABLE void stopNote(int note);

signals:
    void vco1WaveformChanged(); void vco1OctaveChanged(); void vco1PitchChanged(); void vco1ShapeChanged(); void vco1SyncChanged();
    void vco2WaveformChanged(); void vco2OctaveChanged(); void vco2PitchChanged(); void vco2ShapeChanged(); void vco2SyncChanged();
    void multiTypeChanged(); void multiShapeChanged(); void multiLevelChanged(); void multiKeyTrackChanged();
    void mixVco1Changed(); void mixVco2Changed();
    void lpfCutoffChanged(); void lpfResonanceChanged(); void hpfCutoffChanged(); void filterKeyTrackChanged();
    void ampAttackChanged(); void ampDecayChanged(); void ampSustainChanged(); void ampReleaseChanged();
    void modAttackChanged(); void modDecayChanged(); void modIntChanged(); void modTargetChanged();
    void lfoWaveformChanged(); void lfoModeChanged(); void lfoRateChanged(); void lfoIntChanged(); void lfoTargetChanged();
    void voiceModeChanged(); void voiceDepthChanged(); void portamentoChanged(); void panSpreadChanged(); void masterVolumeChanged();
    void delayTypeChanged(); void delayTimeChanged(); void delayFeedbackChanged(); void delayDepthChanged(); void delayMixChanged(); void delaySyncChanged(); void delaySyncDivisionChanged();

private:
    std::shared_ptr<SynthDevice> m_synth;
};

} // namespace noteahead

#endif // SYNTH_CONTROLLER_HPP
