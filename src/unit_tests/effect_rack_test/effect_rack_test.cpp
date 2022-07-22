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

#include "effect_rack_test.hpp"
#include "../../domain/devices/effect_rack.hpp"
#include "../../domain/dsp/reverb_effect.hpp"

#include <QTest>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

namespace noteahead {

void EffectRackTest::test_addRemove()
{
    EffectRack rack;
    QCOMPARE(rack.effectCount(), 0u);

    auto reverb = std::make_shared<ReverbEffect>();
    rack.addEffect(reverb);
    QCOMPARE(rack.effectCount(), 1u);
    QCOMPARE(rack.effect(0), reverb);

    rack.removeEffect(0);
    QCOMPARE(rack.effectCount(), 0u);
}

void EffectRackTest::test_process()
{
    EffectRack rack;
    auto reverb = std::make_shared<ReverbEffect>();
    reverb->setMix(1.0f); // Full wet
    reverb->setSize(0.5f);
    reverb->setDecay(0.5f);
    rack.addEffect(reverb);

    std::vector<float> output(2, 0.0f);
    std::vector<float> sendBus(2, 1.0f); // DC input

    // Reverb needs some samples to build up output
    for (int i = 0; i < 2000; ++i) {
        rack.process(output.data(), sendBus.data(), 0, 1, 44100);
    }

    QVERIFY(output[0] != 0.0f || output[1] != 0.0f);
}

void EffectRackTest::test_serialization()
{
    EffectRack rack;
    auto reverb = std::make_shared<ReverbEffect>();
    reverb->setSize(0.75f);
    rack.addEffect(reverb);

    QString xml;
    QXmlStreamWriter writer(&xml);
    rack.serializeToXml(writer);

    EffectRack rack2;
    QXmlStreamReader reader(xml);
    while (!reader.atEnd() && !reader.isStartElement()) {
        reader.readNext();
    }
    rack2.deserializeFromXml(reader);

    QCOMPARE(rack2.effectCount(), 1u);
    auto reverb2 = std::dynamic_pointer_cast<ReverbEffect>(rack2.effect(0));
    QVERIFY(reverb2 != nullptr);
    QCOMPARE(reverb2->size(), 0.75f);
}

void EffectRackTest::test_reverb_parameters()
{
    auto reverb = std::make_shared<ReverbEffect>();
    
    // Decay: 0.5 internal should be 5000ms
    reverb->setDecay(0.5f);
    QCOMPARE(reverb->decay(), 0.5f);
    
    // Pre-delay: 0.2 internal should be 100ms
    reverb->setPreDelay(0.2f);
    QCOMPARE(reverb->preDelay(), 0.2f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::EffectRackTest)
