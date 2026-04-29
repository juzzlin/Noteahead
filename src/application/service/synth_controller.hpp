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
    Q_PROPERTY(int detune READ detune WRITE setDetune NOTIFY detuneChanged)
    Q_PROPERTY(int freqModAmount READ freqModAmount WRITE setFreqModAmount NOTIFY freqModAmountChanged)
    Q_PROPERTY(int freqModSource READ freqModSource WRITE setFreqModSource NOTIFY freqModSourceChanged)
    Q_PROPERTY(int keyAssignMode READ keyAssignMode WRITE setKeyAssignMode NOTIFY keyAssignModeChanged)
    Q_PROPERTY(int pulseWidth READ pulseWidth WRITE setPulseWidth NOTIFY pulseWidthChanged)
    Q_PROPERTY(int filterCutoff READ filterCutoff WRITE setFilterCutoff NOTIFY filterCutoffChanged)
    Q_PROPERTY(int filterResonance READ filterResonance WRITE setFilterResonance NOTIFY filterResonanceChanged)
    Q_PROPERTY(int filterEnvAmount READ filterEnvAmount WRITE setFilterEnvAmount NOTIFY filterEnvAmountChanged)
    Q_PROPERTY(int filterKeyTrack READ filterKeyTrack WRITE setFilterKeyTrack NOTIFY filterKeyTrackChanged)
    Q_PROPERTY(int filterModAmount READ filterModAmount WRITE setFilterModAmount NOTIFY filterModAmountChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int mg1Frequency READ mg1Frequency WRITE setMg1Frequency NOTIFY mg1FrequencyChanged)
    Q_PROPERTY(int mg2Frequency READ mg2Frequency WRITE setMg2Frequency NOTIFY mg2FrequencyChanged)

    // Oscillator 1
    Q_PROPERTY(int osc1Waveform READ osc1Waveform WRITE setOsc1Waveform NOTIFY osc1WaveformChanged)
    Q_PROPERTY(int osc1Level READ osc1Level WRITE setOsc1Level NOTIFY osc1LevelChanged)
    Q_PROPERTY(int osc1Tune READ osc1Tune WRITE setOsc1Tune NOTIFY osc1TuneChanged)
    Q_PROPERTY(int osc1Octave READ osc1Octave WRITE setOsc1Octave NOTIFY osc1OctaveChanged)

    // Oscillator 2
    Q_PROPERTY(int osc2Waveform READ osc2Waveform WRITE setOsc2Waveform NOTIFY osc2WaveformChanged)
    Q_PROPERTY(int osc2Level READ osc2Level WRITE setOsc2Level NOTIFY osc2LevelChanged)
    Q_PROPERTY(int osc2Tune READ osc2Tune WRITE setOsc2Tune NOTIFY osc2TuneChanged)
    Q_PROPERTY(int osc2Octave READ osc2Octave WRITE setOsc2Octave NOTIFY osc2OctaveChanged)

    // Oscillator 3
    Q_PROPERTY(int osc3Waveform READ osc3Waveform WRITE setOsc3Waveform NOTIFY osc3WaveformChanged)
    Q_PROPERTY(int osc3Level READ osc3Level WRITE setOsc3Level NOTIFY osc3LevelChanged)
    Q_PROPERTY(int osc3Tune READ osc3Tune WRITE setOsc3Tune NOTIFY osc3TuneChanged)
    Q_PROPERTY(int osc3Octave READ osc3Octave WRITE setOsc3Octave NOTIFY osc3OctaveChanged)

    // Oscillator 4
    Q_PROPERTY(int osc4Waveform READ osc4Waveform WRITE setOsc4Waveform NOTIFY osc4WaveformChanged)
    Q_PROPERTY(int osc4Level READ osc4Level WRITE setOsc4Level NOTIFY osc4LevelChanged)
    Q_PROPERTY(int osc4Tune READ osc4Tune WRITE setOsc4Tune NOTIFY osc4TuneChanged)
    Q_PROPERTY(int osc4Octave READ osc4Octave WRITE setOsc4Octave NOTIFY osc4OctaveChanged)

    // ADSR VCA
    Q_PROPERTY(int vcaAttack READ vcaAttack WRITE setVcaAttack NOTIFY vcaAttackChanged)
    Q_PROPERTY(int vcaDecay READ vcaDecay WRITE setVcaDecay NOTIFY vcaDecayChanged)
    Q_PROPERTY(int vcaSustain READ vcaSustain WRITE setVcaSustain NOTIFY vcaSustainChanged)
    Q_PROPERTY(int vcaRelease READ vcaRelease WRITE setVcaRelease NOTIFY vcaReleaseChanged)

    // ADSR VCF
    Q_PROPERTY(int vcfAttack READ vcfAttack WRITE setVcfAttack NOTIFY vcfAttackChanged)
    Q_PROPERTY(int vcfDecay READ vcfDecay WRITE setVcfDecay NOTIFY vcfDecayChanged)
    Q_PROPERTY(int vcfSustain READ vcfSustain WRITE setVcfSustain NOTIFY vcfSustainChanged)
    Q_PROPERTY(int vcfRelease READ vcfRelease WRITE setVcfRelease NOTIFY vcfReleaseChanged)

public:
    explicit SynthController(std::shared_ptr<SynthDevice> synth, QObject * parent = nullptr);
    ~SynthController() override;

    std::shared_ptr<SynthDevice> synth() const;

    int detune() const;
    void setDetune(int detune);

    int freqModAmount() const;
    void setFreqModAmount(int amount);

    int freqModSource() const;
    void setFreqModSource(int source);

    int keyAssignMode() const;
    void setKeyAssignMode(int mode);

    int pulseWidth() const;
    void setPulseWidth(int pw);

    int filterCutoff() const;
    void setFilterCutoff(int cutoff);

    int filterResonance() const;
    void setFilterResonance(int resonance);

    int filterEnvAmount() const;
    void setFilterEnvAmount(int amount);

    int filterKeyTrack() const;
    void setFilterKeyTrack(int track);

    int filterModAmount() const;
    void setFilterModAmount(int amount);

    int volume() const;
    void setVolume(int volume);

    int mg1Frequency() const;
    void setMg1Frequency(int freq);

    int mg2Frequency() const;
    void setMg2Frequency(int freq);

    int osc1Waveform() const;
    void setOsc1Waveform(int waveform);
    int osc1Level() const;
    void setOsc1Level(int level);
    int osc1Tune() const;
    void setOsc1Tune(int tune);
    int osc1Octave() const;
    void setOsc1Octave(int octave);

    int osc2Waveform() const;
    void setOsc2Waveform(int waveform);
    int osc2Level() const;
    void setOsc2Level(int level);
    int osc2Tune() const;
    void setOsc2Tune(int tune);
    int osc2Octave() const;
    void setOsc2Octave(int octave);

    int osc3Waveform() const;
    void setOsc3Waveform(int waveform);
    int osc3Level() const;
    void setOsc3Level(int level);
    int osc3Tune() const;
    void setOsc3Tune(int tune);
    int osc3Octave() const;
    void setOsc3Octave(int octave);

    int osc4Waveform() const;
    void setOsc4Waveform(int waveform);
    int osc4Level() const;
    void setOsc4Level(int level);
    int osc4Tune() const;
    void setOsc4Tune(int tune);
    int osc4Octave() const;
    void setOsc4Octave(int octave);

    int vcaAttack() const;
    void setVcaAttack(int a);
    int vcaDecay() const;
    void setVcaDecay(int d);
    int vcaSustain() const;
    void setVcaSustain(int s);
    int vcaRelease() const;
    void setVcaRelease(int r);

    int vcfAttack() const;
    void setVcfAttack(int a);
    int vcfDecay() const;
    void setVcfDecay(int d);
    int vcfSustain() const;
    void setVcfSustain(int s);
    int vcfRelease() const;
    void setVcfRelease(int r);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void reset();
    Q_INVOKABLE void requestSettings();
    Q_INVOKABLE void accept();
    Q_INVOKABLE void reject();

    Q_INVOKABLE void playNote(int note, double velocity = 1.0);
    Q_INVOKABLE void stopNote(int note);

signals:
    void detuneChanged();
    void freqModAmountChanged();
    void freqModSourceChanged();
    void keyAssignModeChanged();
    void pulseWidthChanged();
    void filterCutoffChanged();
    void filterResonanceChanged();
    void filterEnvAmountChanged();
    void filterKeyTrackChanged();
    void filterModAmountChanged();
    void volumeChanged();
    void mg1FrequencyChanged();
    void mg2FrequencyChanged();
    void osc1WaveformChanged();
    void osc1LevelChanged();
    void osc1TuneChanged();
    void osc1OctaveChanged();
    void osc2WaveformChanged();
    void osc2LevelChanged();
    void osc2TuneChanged();
    void osc2OctaveChanged();
    void osc3WaveformChanged();
    void osc3LevelChanged();
    void osc3TuneChanged();
    void osc3OctaveChanged();
    void osc4WaveformChanged();
    void osc4LevelChanged();
    void osc4TuneChanged();
    void osc4OctaveChanged();
    void vcaAttackChanged();
    void vcaDecayChanged();
    void vcaSustainChanged();
    void vcaReleaseChanged();
    void vcfAttackChanged();
    void vcfDecayChanged();
    void vcfSustainChanged();
    void vcfReleaseChanged();

private:
    std::shared_ptr<SynthDevice> m_synth;
};

} // namespace noteahead

#endif // SYNTH_CONTROLLER_HPP
