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

#ifndef DRUM_HIT_WAVEFORM_MODEL_HPP
#define DRUM_HIT_WAVEFORM_MODEL_HPP

#include "../../application/service/drum_hit_preview.hpp"

#include <QObject>
#include <QTimer>
#include <QVariantList>

#include <functional>
#include <optional>

namespace noteahead {

//! The waveform view of a single drum such as Kick 808 or Kick 909: one hit rendered, the amp
//! envelope it was rendered with, and the playhead running over it while the drum sounds.
//!
//! Owned by the drum's dialog model, which hands it the functions that reach the device, and
//! exposed to the dialog as that model's `waveform`.
class DrumHitWaveformModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList waveformData READ waveformData NOTIFY waveformChanged)
    Q_PROPERTY(double waveformDuration READ waveformDuration NOTIFY waveformChanged)
    Q_PROPERTY(double audibleLength READ audibleLength NOTIFY waveformChanged)
    // The envelope the picture was rendered with, so the overlay never runs ahead of the waveform
    Q_PROPERTY(double envelopeAttack READ envelopeAttack NOTIFY waveformChanged)
    Q_PROPERTY(double envelopeHold READ envelopeHold NOTIFY waveformChanged)
    Q_PROPERTY(double envelopeDecay READ envelopeDecay NOTIFY waveformChanged)
    Q_PROPERTY(double envelopeSustain READ envelopeSustain NOTIFY waveformChanged)
    Q_PROPERTY(double envelopeRelease READ envelopeRelease NOTIFY waveformChanged)
    Q_PROPERTY(double envelopeCurve READ envelopeCurve NOTIFY waveformChanged)
    //! Where the playhead sits, 0..1 of the picture, and whether there is anything to draw it for
    Q_PROPERTY(double playbackPosition READ playbackPosition NOTIFY playbackStatusChanged)
    Q_PROPERTY(bool sounding READ sounding NOTIFY playbackStatusChanged)

public:
    //! In seconds, sustain and curve 0..1, as the waveform view draws them
    struct Envelope
    {
        double attack {};
        double hold {};
        double decay {};
        double sustain { 1.0 };
        double release {};
        double curve {};
    };

    //! How the model reaches the drum. Each may be called with no device to reach, and answers so.
    struct Source
    {
        std::function<DrumHitPreview::Preview(int peakCount)> render;
        std::function<Envelope()> envelope;
        //! How far into the hit sounding now, or nothing when the drum is silent
        std::function<std::optional<double>()> elapsedSeconds;
    };

    explicit DrumHitWaveformModel(Source source, QObject * parent = nullptr);

    QVariantList waveformData() const;
    double waveformDuration() const;
    double audibleLength() const;
    double envelopeAttack() const;
    double envelopeHold() const;
    double envelopeDecay() const;
    double envelopeSustain() const;
    double envelopeRelease() const;
    double envelopeCurve() const;

    double playbackPosition() const;
    bool sounding() const;

    //! How many points the view has room for, and whether it is on screen: the drum is only
    //! rendered while someone can see it
    Q_INVOKABLE void setRequest(int peakCount, bool visible);
    //! Polled by the view while the dialog is open: the playhead moves with the audio, which
    //! nothing in the device has reason to signal about
    Q_INVOKABLE void updatePlaybackStatus();

    //! Renders again once the knobs have settled: a drag would otherwise render on every step
    void schedule();

signals:
    void waveformChanged();
    void playbackStatusChanged();

private:
    void render();

    static constexpr int DebounceMs { 150 };

    Source m_source;
    QTimer m_timer;
    int m_peakCount { 0 };
    bool m_visible { false };
    QVariantList m_data;
    double m_duration { 0.0 };
    double m_audibleLength { 0.0 };
    Envelope m_envelope;
};

} // namespace noteahead

#endif // DRUM_HIT_WAVEFORM_MODEL_HPP
