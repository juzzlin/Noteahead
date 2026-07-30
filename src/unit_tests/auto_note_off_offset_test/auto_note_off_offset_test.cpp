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

#include "auto_note_off_offset_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/tracker/auto_note_off_offset.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QBuffer>
#include <QTest>

#include <clocale>

namespace noteahead {

using namespace std::chrono_literals;

namespace {

const auto testElement = "Timing";

//! The timing of a song at 120 BPM, 4 lines per beat and 24 ticks per line: one line is 24 ticks and
//! one beat is 96.
constexpr size_t beatsPerMinute = 120;
constexpr size_t linesPerBeat = 4;
constexpr size_t ticksPerLine = 24;

size_t ticks(const AutoNoteOffOffset & offset)
{
    return offset.ticks(beatsPerMinute, linesPerBeat, ticksPerLine);
}

QByteArray serialize(const AutoNoteOffOffset & offset)
{
    QByteArray data;
    QBuffer buffer { &data };
    buffer.open(QIODevice::WriteOnly);
    NahdXmlWriter writer { buffer };
    writer.writeStartElement(testElement);
    offset.serializeToXmlAttributes(writer);
    writer.writeEndElement();
    return data;
}

AutoNoteOffOffset deserialize(QByteArray & data)
{
    QBuffer buffer { &data };
    buffer.open(QIODevice::ReadOnly);
    NahdXmlReader reader { buffer };
    if (reader.readNextStartElement()) {
        return AutoNoteOffOffset::deserializeFromXmlAttributes(reader);
    }
    return {};
}

} // namespace

void AutoNoteOffOffsetTest::test_defaults_shouldBeMilliseconds()
{
    // Milliseconds is what MIDI hardware wants and what every song predating sync mode used.
    const AutoNoteOffOffset offset;

    QVERIFY(!offset.syncEnabled());
    QCOMPARE(offset.mode(), AutoNoteOffOffset::Mode::Milliseconds);
    QCOMPARE(offset.milliseconds(), Constants::defaultAutoNoteOffOffset());
    QCOMPARE(Constants::defaultAutoNoteOffOffset(), 25ms);
}

void AutoNoteOffOffsetTest::test_ticks_milliseconds_shouldScaleWithTempo()
{
    const AutoNoteOffOffset offset { 125ms };

    // 125 ms is an eighth of a minute-eighth: at 120 BPM one beat is 500 ms, so this is a quarter of
    // a beat, which is one line of 24 ticks.
    QCOMPARE(ticks(offset), 24);

    // Twice the tempo makes the same wall-clock offset cover twice as many ticks.
    QCOMPARE(offset.ticks(beatsPerMinute * 2, linesPerBeat, ticksPerLine), 48);
}

void AutoNoteOffOffsetTest::test_ticks_milliseconds_zero_shouldBeZeroTicks()
{
    // Legal, if unkind to hardware: cut the note exactly at the next note-on.
    QCOMPARE(ticks(AutoNoteOffOffset { 0ms }), 0);
}

void AutoNoteOffOffsetTest::test_ticks_sync_shouldBeANoteLength()
{
    // A whole note is four beats of 96 ticks, so 1/16 is one beat quarter: 24 ticks.
    QCOMPARE(ticks(AutoNoteOffOffset { 16 }), 24);
    QCOMPARE(ticks(AutoNoteOffOffset { 32 }), 12);
    QCOMPARE(ticks(AutoNoteOffOffset { 64 }), 6);
    QCOMPARE(ticks(AutoNoteOffOffset { 4 }), 96);
}

void AutoNoteOffOffsetTest::test_ticks_sync_shouldNotScaleWithTempo()
{
    const AutoNoteOffOffset offset { 16 };

    // The whole point of sync mode: the gap stays the same fraction of the groove at any tempo.
    QCOMPARE(offset.ticks(beatsPerMinute, linesPerBeat, ticksPerLine), offset.ticks(beatsPerMinute * 4, linesPerBeat, ticksPerLine));
}

void AutoNoteOffOffsetTest::test_ticks_sync_everyDenominator_shouldBeShorterThanThePrevious()
{
    // Every value the UI offers has to be usable, and they have to be in descending length order,
    // which is the order the combo box presents them in.
    std::optional<size_t> previous;
    for (auto && denominator : AutoNoteOffOffset::syncDenominators()) {
        const auto current = ticks(AutoNoteOffOffset { denominator });
        QVERIFY2(current > 0, qPrintable(QString { "1/%1 rounds down to zero ticks" }.arg(denominator)));
        if (previous.has_value()) {
            QVERIFY(current < *previous);
        }
        previous = current;
    }
}

void AutoNoteOffOffsetTest::test_ticks_sync_invalidDenominator_shouldBeZeroTicks()
{
    AutoNoteOffOffset offset { 16 };
    offset.setSyncDenominator(0);

    QCOMPARE(ticks(offset), 0);
}

void AutoNoteOffOffsetTest::test_serialization_shouldRoundTrip()
{
    const AutoNoteOffOffset offset { 250ms };

    auto data = serialize(offset);
    const auto restored = deserialize(data);

    QCOMPARE(restored, offset);
}

void AutoNoteOffOffsetTest::test_serialization_sync_shouldKeepMillisecondsValue()
{
    AutoNoteOffOffset offset { 64 };
    offset.setMilliseconds(42ms);

    auto data = serialize(offset);
    const auto restored = deserialize(data);

    // The inactive mode's value has to survive, or toggling sync off in the UI and saving would
    // silently reset the milliseconds the user had set.
    QCOMPARE(restored, offset);
    QVERIFY(restored.syncEnabled());
    QCOMPARE(restored.syncDenominator(), 64);
    QCOMPARE(restored.milliseconds(), 42ms);
}

void AutoNoteOffOffsetTest::test_serialization_commaDecimalLocale_shouldRoundTrip()
{
    // Sync mode is a denominator rather than a fraction of a whole note precisely so that no
    // decimal separator ever reaches the project file.
    const auto * previous = std::setlocale(LC_NUMERIC, nullptr);
    const std::string saved = previous ? previous : "C";
    if (!std::setlocale(LC_NUMERIC, "fi_FI.UTF-8")) {
        QSKIP("A comma-decimal locale is not installed");
    }

    const AutoNoteOffOffset offset { 64 };

    auto data = serialize(offset);
    QVERIFY2(!data.contains(","), "A decimal comma reached the project file");

    const auto restored = deserialize(data);
    QCOMPARE(restored, offset);

    std::setlocale(LC_NUMERIC, saved.c_str());
}

void AutoNoteOffOffsetTest::test_deserialization_legacyAttributesOnly_shouldBeMilliseconds()
{
    // What a project saved before sync mode existed looks like.
    QByteArray data = QString { "<%1 %2=\"666\"/>" }
                        .arg(testElement)
                        .arg(Constants::NahdXml::xmlKeyAutoNoteOffOffset())
                        .toUtf8();

    const auto restored = deserialize(data);

    QVERIFY(!restored.syncEnabled());
    QCOMPARE(restored.milliseconds(), 666ms);
}

void AutoNoteOffOffsetTest::test_deserialization_noAttributes_shouldKeepDefaults()
{
    QByteArray data = QString { "<%1/>" }.arg(testElement).toUtf8();

    const auto restored = deserialize(data);

    QCOMPARE(restored, AutoNoteOffOffset {});
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoNoteOffOffsetTest)
