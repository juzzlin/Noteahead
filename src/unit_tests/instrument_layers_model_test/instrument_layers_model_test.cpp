// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "instrument_layers_model_test.hpp"

#include "../../application/models/instrument_layers_model.hpp"
#include "../../domain/instrument_layer.hpp"

#include <QSignalSpy>

namespace noteahead {

void InstrumentLayersModelTest::test_addLayers_shouldAddLayers()
{
    const AutomationLocation location { 1, 2, 3 };

    InstrumentLayer::Parameters params;
    params.targetTrack = 5;
    params.note = 60;
    params.followSourceNote = true;
    params.velocity = 100;
    params.applyTargetVelocity = true;
    params.followSourceVelocity = false;

    InstrumentLayer instrumentLayer { 42, location, params, "Comment", true };

    InstrumentLayersModel model;
    model.setLayers({ instrumentLayer });

    QCOMPARE(model.rowCount(), 1);

    const auto index = model.index(0);

    // Base roles
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Column)), static_cast<quint64>(instrumentLayer.location().column()));
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Comment)), instrumentLayer.comment());
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Enabled)), instrumentLayer.enabled());
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Id)), static_cast<quint64>(instrumentLayer.id()));
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Track)), static_cast<quint64>(instrumentLayer.location().track()));

    // Parameter roles
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::TargetTrack)), static_cast<quint64>(params.targetTrack));
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Note)), static_cast<int>(params.note));
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::FollowSourceNote)), params.followSourceNote);
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::Velocity)), static_cast<int>(params.velocity));
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::ApplyTargetVelocity)), params.applyTargetVelocity);
    QCOMPARE(model.data(index, static_cast<int>(InstrumentLayersModel::DataRole::FollowSourceVelocity)), params.followSourceVelocity);
}

void InstrumentLayersModelTest::test_requestLayers_shouldFilterLayers()
{
    InstrumentLayer instrumentLayer { 42, { 1, 2, 3 }, {}, {} };

    InstrumentLayersModel model;
    QSignalSpy layersRequestedSpy { &model, &InstrumentLayersModel::layersRequested };

    model.requestLayersByColumn(2, 3);
    model.setLayers({ instrumentLayer });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(layersRequestedSpy.count(), 1);

    model.requestLayersByColumn(2, 4);
    model.setLayers({ instrumentLayer });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(layersRequestedSpy.count(), 2);

    model.requestLayersByTrack(2);
    model.setLayers({ instrumentLayer });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(layersRequestedSpy.count(), 3);

    model.requestLayersByTrack(3);
    model.setLayers({ instrumentLayer });
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(layersRequestedSpy.count(), 4);

    model.requestLayers();
    model.setLayers({ instrumentLayer });
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(layersRequestedSpy.count(), 5);
}

void InstrumentLayersModelTest::test_setData_shouldUpdateLayerData()
{
    using Role = InstrumentLayersModel::DataRole;

    const AutomationLocation location { 1, 2, 3 };

    InstrumentLayer::Parameters params;
    params.targetTrack = 5;
    params.note = 60;
    params.followSourceNote = true;
    params.velocity = 100;
    params.applyTargetVelocity = true;
    params.followSourceVelocity = false;

    InstrumentLayer layer { 42, location, params, "Old Comment", true };
    std::optional<InstrumentLayer> updatedLayer;

    InstrumentLayersModel model;
    model.setLayers({ layer });

    QSignalSpy layerChangedSpy { &model, &InstrumentLayersModel::layerChanged };
    connect(&model, &InstrumentLayersModel::layerChanged, this, [&updatedLayer](auto && layer) {
        updatedLayer = layer;
    });

    const auto index = model.index(0);

    // Editable top-level roles
    const QString newComment = "New Comment";
    QVERIFY(model.setData(index, newComment, static_cast<int>(Role::Comment)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Comment)).toString(), newComment);
    QVERIFY(!model.setData(index, newComment, static_cast<int>(Role::Comment)));

    QVERIFY(model.setData(index, false, static_cast<int>(Role::Enabled)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Enabled)).toBool(), false);
    QVERIFY(model.setData(index, true, static_cast<int>(Role::Enabled)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Enabled)).toBool(), true);
    QVERIFY(!model.setData(index, true, static_cast<int>(Role::Enabled)));

    // Non-editable roles
    QVERIFY(!model.setData(index, 5u, static_cast<int>(Role::Track)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Track)).toUInt(), layer.location().track());
    QVERIFY(!model.setData(index, 2u, static_cast<int>(Role::Column)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Column)).toUInt(), layer.location().column());
    QVERIFY(!model.setData(index, 99u, static_cast<int>(Role::Id)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Id)).toUInt(), layer.id());

    // Editable parameter roles
    QVERIFY(model.setData(index, 10u, static_cast<int>(Role::TargetTrack)));
    QCOMPARE(model.data(index, static_cast<int>(Role::TargetTrack)).toULongLong(), 10u);

    QVERIFY(model.setData(index, 64u, static_cast<int>(Role::Note)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Note)).toInt(), 64);

    QVERIFY(model.setData(index, false, static_cast<int>(Role::FollowSourceNote)));
    QCOMPARE(model.data(index, static_cast<int>(Role::FollowSourceNote)).toBool(), false);

    QVERIFY(model.setData(index, 120u, static_cast<int>(Role::Velocity)));
    QCOMPARE(model.data(index, static_cast<int>(Role::Velocity)).toInt(), 120);

    QVERIFY(model.setData(index, false, static_cast<int>(Role::ApplyTargetVelocity)));
    QCOMPARE(model.data(index, static_cast<int>(Role::ApplyTargetVelocity)).toBool(), false);

    QVERIFY(model.setData(index, true, static_cast<int>(Role::FollowSourceVelocity)));
    QCOMPARE(model.data(index, static_cast<int>(Role::FollowSourceVelocity)).toBool(), true);

    // Verify that layerChanged is emitted correctly on apply
    model.applyAll();
    QVERIFY(updatedLayer.has_value());
    QCOMPARE(layerChangedSpy.count(), 1);

    QCOMPARE(updatedLayer->comment(), model.data(index, static_cast<int>(Role::Comment)).toString());
    QCOMPARE(updatedLayer->enabled(), model.data(index, static_cast<int>(Role::Enabled)).toBool());
    QCOMPARE(updatedLayer->location().track(), model.data(index, static_cast<int>(Role::Track)).toUInt());
    QCOMPARE(updatedLayer->location().column(), model.data(index, static_cast<int>(Role::Column)).toUInt());

    // Verify parameters
    QCOMPARE(updatedLayer->parameters().targetTrack, model.data(index, static_cast<int>(Role::TargetTrack)).toULongLong());
    QCOMPARE(updatedLayer->parameters().note, static_cast<uint8_t>(model.data(index, static_cast<int>(Role::Note)).toUInt()));
    QCOMPARE(updatedLayer->parameters().followSourceNote, model.data(index, static_cast<int>(Role::FollowSourceNote)).toBool());
    QCOMPARE(updatedLayer->parameters().velocity, static_cast<uint8_t>(model.data(index, static_cast<int>(Role::Velocity)).toUInt()));
    QCOMPARE(updatedLayer->parameters().applyTargetVelocity, model.data(index, static_cast<int>(Role::ApplyTargetVelocity)).toBool());
    QCOMPARE(updatedLayer->parameters().followSourceVelocity, model.data(index, static_cast<int>(Role::FollowSourceVelocity)).toBool());
}

void InstrumentLayersModelTest::test_removeAt_shouldRemoveLayerData()
{
    InstrumentLayer layer { 42, { 1, 2, 3 }, {}, "Old Comment" };

    InstrumentLayersModel model;
    model.setLayers({ layer });
    QSignalSpy layerDeletedSpy { &model, &InstrumentLayersModel::layerDeleted };

    model.removeAt(0);
    QCOMPARE(model.rowCount(), 0);

    model.applyAll();
    QCOMPARE(layerDeletedSpy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::InstrumentLayersModelTest)
