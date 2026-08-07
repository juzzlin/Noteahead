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

#include "render_settings.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <QString>
#include <QVariant>

namespace noteahead {

int RenderSettings::format() const
{
    return m_format;
}

void RenderSettings::setFormat(int format)
{
    m_format = format;
}

int RenderSettings::sampleRate() const
{
    return m_sampleRate;
}

void RenderSettings::setSampleRate(int sampleRate)
{
    m_sampleRate = sampleRate;
}

int RenderSettings::bitDepth() const
{
    return m_bitDepth;
}

void RenderSettings::setBitDepth(int bitDepth)
{
    m_bitDepth = bitDepth;
}

int RenderSettings::oversampleFactor() const
{
    return m_oversampleFactor;
}

void RenderSettings::setOversampleFactor(int factor)
{
    m_oversampleFactor = factor;
}

bool RenderSettings::normalizeEnabled() const
{
    return m_normalizeEnabled;
}

void RenderSettings::setNormalizeEnabled(bool enabled)
{
    m_normalizeEnabled = enabled;
}

int RenderSettings::normalizeLevelTenthsDb() const
{
    return m_normalizeLevelTenthsDb;
}

void RenderSettings::setNormalizeLevelTenthsDb(int tenths)
{
    m_normalizeLevelTenthsDb = tenths;
}

bool RenderSettings::trimEnabled() const
{
    return m_trimEnabled;
}

void RenderSettings::setTrimEnabled(bool enabled)
{
    m_trimEnabled = enabled;
}

int RenderSettings::trimMinutes() const
{
    return m_trimMinutes;
}

void RenderSettings::setTrimMinutes(int minutes)
{
    m_trimMinutes = minutes;
}

int RenderSettings::trimSeconds() const
{
    return m_trimSeconds;
}

void RenderSettings::setTrimSeconds(int seconds)
{
    m_trimSeconds = seconds;
}

bool RenderSettings::fadeOutEnabled() const
{
    return m_fadeOutEnabled;
}

void RenderSettings::setFadeOutEnabled(bool enabled)
{
    m_fadeOutEnabled = enabled;
}

int RenderSettings::fadeOutSeconds() const
{
    return m_fadeOutSeconds;
}

void RenderSettings::setFadeOutSeconds(int seconds)
{
    m_fadeOutSeconds = seconds;
}

int RenderSettings::fadeOutTenths() const
{
    return m_fadeOutTenths;
}

void RenderSettings::setFadeOutTenths(int tenths)
{
    m_fadeOutTenths = tenths;
}

bool RenderSettings::silenceEnabled() const
{
    return m_silenceEnabled;
}

void RenderSettings::setSilenceEnabled(bool enabled)
{
    m_silenceEnabled = enabled;
}

int RenderSettings::silenceSeconds() const
{
    return m_silenceSeconds;
}

void RenderSettings::setSilenceSeconds(int seconds)
{
    m_silenceSeconds = seconds;
}

int RenderSettings::silenceTenths() const
{
    return m_silenceTenths;
}

void RenderSettings::setSilenceTenths(int tenths)
{
    m_silenceTenths = tenths;
}

bool RenderSettings::analyzeEnabled() const
{
    return m_analyzeEnabled;
}

void RenderSettings::setAnalyzeEnabled(bool enabled)
{
    m_analyzeEnabled = enabled;
}

void RenderSettings::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyRenderSettings());

    writer.writeAttribute(Constants::NahdXml::xmlKeyFormat(), QString::number(m_format));
    writer.writeAttribute(Constants::NahdXml::xmlKeySampleRate(), QString::number(m_sampleRate));
    writer.writeAttribute(Constants::NahdXml::xmlKeyBitDepth(), QString::number(m_bitDepth));
    writer.writeAttribute(Constants::NahdXml::xmlKeyOversampleFactor(), QString::number(m_oversampleFactor));
    writer.writeAttribute(Constants::NahdXml::xmlKeyNormalizeEnabled(), m_normalizeEnabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyNormalizeLevelTenthsDb(), QString::number(m_normalizeLevelTenthsDb));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTrimEnabled(), m_trimEnabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyTrimMinutes(), QString::number(m_trimMinutes));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTrimSeconds(), QString::number(m_trimSeconds));
    writer.writeAttribute(Constants::NahdXml::xmlKeyFadeOutEnabled(), m_fadeOutEnabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFadeOutSeconds(), QString::number(m_fadeOutSeconds));
    writer.writeAttribute(Constants::NahdXml::xmlKeyFadeOutTenths(), QString::number(m_fadeOutTenths));
    writer.writeAttribute(Constants::NahdXml::xmlKeySilenceEnabled(), m_silenceEnabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());
    writer.writeAttribute(Constants::NahdXml::xmlKeySilenceSeconds(), QString::number(m_silenceSeconds));
    writer.writeAttribute(Constants::NahdXml::xmlKeySilenceTenths(), QString::number(m_silenceTenths));
    writer.writeAttribute(Constants::NahdXml::xmlKeyAnalyzeEnabled(), m_analyzeEnabled ? Constants::NahdXml::xmlValueTrue() : Constants::NahdXml::xmlValueFalse());

    writer.writeEndElement(); // RenderSettings
}

void RenderSettings::deserializeFromXml(ProjectReader & reader)
{
    const auto integer = [&reader](const QString & key, int fallback) {
        const auto value = reader.attribute(key);
        return value.isValid() ? value.toInt() : fallback;
    };
    const auto boolean = [&reader](const QString & key, bool fallback) {
        const auto value = reader.attribute(key);
        return value.isValid() ? value.toString() == Constants::NahdXml::xmlValueTrue() : fallback;
    };

    m_format = integer(Constants::NahdXml::xmlKeyFormat(), m_format);
    m_sampleRate = integer(Constants::NahdXml::xmlKeySampleRate(), m_sampleRate);
    m_bitDepth = integer(Constants::NahdXml::xmlKeyBitDepth(), m_bitDepth);
    m_oversampleFactor = integer(Constants::NahdXml::xmlKeyOversampleFactor(), m_oversampleFactor);
    m_normalizeEnabled = boolean(Constants::NahdXml::xmlKeyNormalizeEnabled(), m_normalizeEnabled);
    m_normalizeLevelTenthsDb = integer(Constants::NahdXml::xmlKeyNormalizeLevelTenthsDb(), m_normalizeLevelTenthsDb);
    m_trimEnabled = boolean(Constants::NahdXml::xmlKeyTrimEnabled(), m_trimEnabled);
    m_trimMinutes = integer(Constants::NahdXml::xmlKeyTrimMinutes(), m_trimMinutes);
    m_trimSeconds = integer(Constants::NahdXml::xmlKeyTrimSeconds(), m_trimSeconds);
    m_fadeOutEnabled = boolean(Constants::NahdXml::xmlKeyFadeOutEnabled(), m_fadeOutEnabled);
    m_fadeOutSeconds = integer(Constants::NahdXml::xmlKeyFadeOutSeconds(), m_fadeOutSeconds);
    m_fadeOutTenths = integer(Constants::NahdXml::xmlKeyFadeOutTenths(), m_fadeOutTenths);
    m_silenceEnabled = boolean(Constants::NahdXml::xmlKeySilenceEnabled(), m_silenceEnabled);
    m_silenceSeconds = integer(Constants::NahdXml::xmlKeySilenceSeconds(), m_silenceSeconds);
    m_silenceTenths = integer(Constants::NahdXml::xmlKeySilenceTenths(), m_silenceTenths);
    m_analyzeEnabled = boolean(Constants::NahdXml::xmlKeyAnalyzeEnabled(), m_analyzeEnabled);
}

} // namespace noteahead
