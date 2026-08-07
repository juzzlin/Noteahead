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

#include "render_settings_test.hpp"

#include "../../common/audio_backend.hpp"
#include "../../common/constants.hpp"
#include "../../domain/tracker/metadata.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>

#include <clocale>

namespace noteahead {

namespace {

//! Fills every field with a value distinct from the defaults, so a lost one is visible.
void populate(RenderSettings & settings)
{
    settings.setFormat(static_cast<int>(AudioFormat::Wav));
    settings.setSampleRate(96000);
    settings.setBitDepth(static_cast<int>(BitDepth::Float_32));
    settings.setOversampleFactor(2);
    settings.setNormalizeEnabled(true);
    settings.setNormalizeLevelTenthsDb(-15);
    settings.setTrimEnabled(true);
    settings.setTrimMinutes(3);
    settings.setTrimSeconds(30);
    settings.setFadeOutEnabled(true);
    settings.setFadeOutSeconds(5);
    settings.setFadeOutTenths(5);
    settings.setSilenceEnabled(true);
    settings.setSilenceSeconds(2);
    settings.setSilenceTenths(3);
    settings.setAnalyzeEnabled(false);
}

void verify(const RenderSettings & settings)
{
    QCOMPARE(settings.format(), static_cast<int>(AudioFormat::Wav));
    QCOMPARE(settings.sampleRate(), 96000);
    QCOMPARE(settings.bitDepth(), static_cast<int>(BitDepth::Float_32));
    QCOMPARE(settings.oversampleFactor(), 2);
    QCOMPARE(settings.normalizeEnabled(), true);
    QCOMPARE(settings.normalizeLevelTenthsDb(), -15);
    QCOMPARE(settings.trimEnabled(), true);
    QCOMPARE(settings.trimMinutes(), 3);
    QCOMPARE(settings.trimSeconds(), 30);
    QCOMPARE(settings.fadeOutEnabled(), true);
    QCOMPARE(settings.fadeOutSeconds(), 5);
    QCOMPARE(settings.fadeOutTenths(), 5);
    QCOMPARE(settings.silenceEnabled(), true);
    QCOMPARE(settings.silenceSeconds(), 2);
    QCOMPARE(settings.silenceTenths(), 3);
    QCOMPARE(settings.analyzeEnabled(), false);
}

QByteArray serialize(const Metadata & metadata)
{
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    metadata.serializeToXml(writer);
    return data;
}

Metadata deserialize(QByteArray & data)
{
    QBuffer buffer { &data };
    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    Metadata metadata;
    if (reader.readNextStartElement()) {
        metadata.deserializeFromXml(reader);
    }
    return metadata;
}

} // namespace

void RenderSettingsTest::test_defaults_shouldBeTheOnesANewSongStartsFrom()
{
    // There are no application-wide render settings any more, so these are what every new song and
    // every project saved before render settings existed will use. Bit depth in particular is an
    // enum index rather than a bit count, and an out-of-range default fails the whole export.
    const RenderSettings settings;

    QCOMPARE(settings.format(), static_cast<int>(AudioFormat::Flac));
    QCOMPARE(settings.sampleRate(), 48000);
    QCOMPARE(settings.bitDepth(), static_cast<int>(BitDepth::PCM_24));
    QCOMPARE(settings.oversampleFactor(), 4);
    QCOMPARE(settings.analyzeEnabled(), true);
    QCOMPARE(settings.normalizeEnabled(), false);
    QCOMPARE(settings.trimEnabled(), false);
    QCOMPARE(settings.fadeOutEnabled(), false);
    QCOMPARE(settings.silenceEnabled(), false);
}

void RenderSettingsTest::test_serialization_shouldRoundTripThroughMetadata()
{
    Metadata metadata;
    metadata.setTag(Constants::NahdXml::xmlKeyTitle().toStdString(), "A song");
    populate(metadata.renderSettings());

    auto data = serialize(metadata);
    const auto restored = deserialize(data);

    verify(restored.renderSettings());
    QCOMPARE(restored.tags().at(Constants::NahdXml::xmlKeyTitle().toStdString()), std::string { "A song" });
}

void RenderSettingsTest::test_serialization_commaDecimalLocale_shouldRoundTrip()
{
    // The reason every value here is an integer. Under a locale that writes decimals with a comma,
    // anything that reached the file as a double would come back as "-1,5" and fail to parse.
    const auto * previous = std::setlocale(LC_NUMERIC, nullptr);
    const std::string saved = previous ? previous : "C";
    if (!std::setlocale(LC_NUMERIC, "fi_FI.UTF-8")) {
        QSKIP("A comma-decimal locale is not installed");
    }

    Metadata metadata;
    populate(metadata.renderSettings());
    metadata.renderSettings().setNormalizeLevelTenthsDb(-15); // -1.5 dB

    auto data = serialize(metadata);
    QVERIFY2(!data.contains(","), "A decimal comma reached the project file");

    const auto restored = deserialize(data);
    verify(restored.renderSettings());

    std::setlocale(LC_NUMERIC, saved.c_str());
}

void RenderSettingsTest::test_metadataWithoutRenderSettings_shouldKeepDefaults()
{
    // A project saved before render settings belonged to the song. It must come back on the
    // defaults rather than with zeroes left behind.
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    {
        NahdXmlWriter writer { buffer };
        Metadata metadata;
        metadata.setTag(Constants::NahdXml::xmlKeyTitle().toStdString(), "Old song");
        metadata.serializeToXml(writer);
    }
    buffer.close();

    // Strip the element the way an older version would never have written it.
    const auto start = data.indexOf("<" + Constants::NahdXml::xmlKeyRenderSettings().toUtf8());
    QVERIFY(start >= 0);
    const auto end = data.indexOf('>', start);
    data.remove(start, end - start + 1);
    QVERIFY(!data.contains(Constants::NahdXml::xmlKeyRenderSettings().toUtf8()));

    const auto restored = deserialize(data);

    QCOMPARE(restored.renderSettings().format(), static_cast<int>(AudioFormat::Flac));
    QCOMPARE(restored.renderSettings().sampleRate(), 48000);
    QCOMPARE(restored.renderSettings().bitDepth(), static_cast<int>(BitDepth::PCM_24));
    QCOMPARE(restored.renderSettings().oversampleFactor(), 4);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderSettingsTest)
