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

#ifndef RENDER_SETTINGS_HPP
#define RENDER_SETTINGS_HPP

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! How a song is rendered to audio. Part of the song rather than the application, so exporting one
//! song cannot quietly inherit another song's trim length. There are no application-wide render
//! settings: a new song starts from the defaults below and diverges from there, like its tags.
//!
//! Every value here is an integer on purpose, including the normalize level, which is kept in tenths
//! of a decibel. The project format contains no floating-point values anywhere — continuous
//! parameters go through Parameter's integer xmlValue/xmlScale for the same reason — because
//! std::to_string, std::stod and printf("%f") all follow the C locale: under a locale that writes
//! "-1,0" the file stops round-tripping. Please keep it integer-only rather than "simplifying" this
//! to a double.
class RenderSettings
{
public:
    RenderSettings() = default;

    int format() const;
    void setFormat(int format);

    int sampleRate() const;
    void setSampleRate(int sampleRate);

    //! A BitDepth enum value (0 = 16-bit PCM, 1 = 24-bit, 2 = 32-bit, 3 = float), not a bit count.
    int bitDepth() const;
    void setBitDepth(int bitDepth);

    int oversampleFactor() const;
    void setOversampleFactor(int factor);

    //! Whether the render may spread the devices across the worker threads. Devices are summed per
    //! worker lane and which lane a device lands in is decided by whichever worker wins the task
    //! queue, so parallel rendering regroups the sum -- and float addition is not associative. The
    //! same song then renders to a file that differs in the last bits from one run to the next.
    //! Defaults to false, which is how rendering has always behaved.
    bool fastRender() const;
    void setFastRender(bool fast);

    bool normalizeEnabled() const;
    void setNormalizeEnabled(bool enabled);

    //! Target level in tenths of a decibel, e.g. -10 for -1.0 dB.
    int normalizeLevelTenthsDb() const;
    void setNormalizeLevelTenthsDb(int tenths);

    bool trimEnabled() const;
    void setTrimEnabled(bool enabled);

    int trimMinutes() const;
    void setTrimMinutes(int minutes);

    int trimSeconds() const;
    void setTrimSeconds(int seconds);

    //! The fade out ends where the tail silence begins, i.e. on the trimmed end minus the silence
    //! when trimming, and on the song end otherwise. Its length is split into whole seconds and
    //! tenths of a second the same way trimming splits into minutes and seconds.
    bool fadeOutEnabled() const;
    void setFadeOutEnabled(bool enabled);

    int fadeOutSeconds() const;
    void setFadeOutSeconds(int seconds);

    int fadeOutTenths() const;
    void setFadeOutTenths(int tenths);

    //! Silence ending the rendered file. It is carved out of the trimmed length when trimming, so
    //! that the trim stays the exact length of the file, and appended after the song end otherwise.
    bool silenceEnabled() const;
    void setSilenceEnabled(bool enabled);

    int silenceSeconds() const;
    void setSilenceSeconds(int seconds);

    int silenceTenths() const;
    void setSilenceTenths(int tenths);

    bool analyzeEnabled() const;
    void setAnalyzeEnabled(bool enabled);

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

private:
    // Defaults for a new song. A project saved without a RenderSettings element simply keeps these.
    int m_format = 1; // AudioFormat::Flac
    int m_sampleRate = 48000;
    int m_bitDepth = 1; // BitDepth::PCM_24
    int m_oversampleFactor = 4;
    bool m_fastRender = false;
    bool m_normalizeEnabled = false;
    int m_normalizeLevelTenthsDb = -10;
    bool m_trimEnabled = false;
    int m_trimMinutes = 0;
    int m_trimSeconds = 0;
    bool m_fadeOutEnabled = false;
    int m_fadeOutSeconds = 0;
    int m_fadeOutTenths = 0;
    bool m_silenceEnabled = false;
    int m_silenceSeconds = 0;
    int m_silenceTenths = 0;
    bool m_analyzeEnabled = true;
};

} // namespace noteahead

#endif // RENDER_SETTINGS_HPP
