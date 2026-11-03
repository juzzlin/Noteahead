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

#include "drum_hit_waveform_model.hpp"

#include <algorithm>

namespace noteahead {

DrumHitWaveformModel::DrumHitWaveformModel(Source source, QObject * parent)
  : QObject { parent }
  , m_source { std::move(source) }
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(DebounceMs);
    connect(&m_timer, &QTimer::timeout, this, &DrumHitWaveformModel::render);
}

QVariantList DrumHitWaveformModel::waveformData() const
{
    return m_data;
}

double DrumHitWaveformModel::waveformDuration() const
{
    return m_duration;
}

double DrumHitWaveformModel::audibleLength() const
{
    return m_audibleLength;
}

double DrumHitWaveformModel::envelopeAttack() const
{
    return m_envelope.attack;
}

double DrumHitWaveformModel::envelopeHold() const
{
    return m_envelope.hold;
}

double DrumHitWaveformModel::envelopeDecay() const
{
    return m_envelope.decay;
}

double DrumHitWaveformModel::envelopeSustain() const
{
    return m_envelope.sustain;
}

double DrumHitWaveformModel::envelopeRelease() const
{
    return m_envelope.release;
}

double DrumHitWaveformModel::envelopeCurve() const
{
    return m_envelope.curve;
}

double DrumHitWaveformModel::playbackPosition() const
{
    if (m_duration <= 0.0 || !m_source.elapsedSeconds) {
        return 0.0;
    }
    const auto elapsed = m_source.elapsedSeconds();
    // Against the picture rather than the sound: an envelope pulled in ends the hit before the
    // drum does, and the playhead has to stop where it is heard to
    return elapsed ? std::clamp(*elapsed / m_duration, 0.0, 1.0) : 0.0;
}

bool DrumHitWaveformModel::sounding() const
{
    return m_source.elapsedSeconds && m_source.elapsedSeconds().has_value();
}

void DrumHitWaveformModel::setRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_peakCount || visible != m_visible;
    m_peakCount = peakCount;
    m_visible = visible;
    if (changed) {
        schedule();
    }
}

void DrumHitWaveformModel::updatePlaybackStatus()
{
    emit playbackStatusChanged();
}

void DrumHitWaveformModel::schedule()
{
    if (!m_visible || m_peakCount <= 0) {
        m_timer.stop();
        return;
    }
    m_timer.start();
}

void DrumHitWaveformModel::render()
{
    m_timer.stop();
    if (!m_visible || m_peakCount <= 0 || !m_source.render) {
        return;
    }
    const auto preview = m_source.render(m_peakCount);
    m_data = preview.peaks;
    m_duration = preview.durationSeconds;
    m_audibleLength = preview.audibleSeconds;
    m_envelope = m_source.envelope ? m_source.envelope() : Envelope {};
    emit waveformChanged();
}

} // namespace noteahead
