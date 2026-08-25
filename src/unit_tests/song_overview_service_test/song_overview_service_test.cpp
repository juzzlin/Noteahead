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

#include "song_overview_service_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/song_overview_service.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/effects/all_pass_filter.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/controllers/effect_rack_controller.hpp"
#include "../../view/controllers/song_overview_controller.hpp"

#include <QTest>

#include <algorithm>
#include <memory>

namespace noteahead {

namespace {

class MockDevice : public Device
{
public:
    explicit MockDevice(std::string name)
      : m_name { std::move(name) }
    {
    }

    std::string name() const override
    {
        return m_name;
    }

    std::string category() const override
    {
        return "Mock";
    }

    std::string typeName() const override
    {
        return "MockDevice";
    }

    std::string typeId() const override
    {
        return "mock-device-id";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processAudio(AudioContext &) override
    {
    }

    bool hasActiveAudio() const override
    {
        return true;
    }

private:
    std::string m_name;
};

struct Fixture
{
    Fixture()
      : engine { std::make_shared<AudioEngine>() }
      , deviceService { std::make_shared<DeviceService>(engine, std::make_shared<DataService>()) }
      , editorService { std::make_shared<EditorService>() }
      , service { std::make_shared<SongOverviewService>(deviceService, editorService) }
    {
        EffectFactory::init();
    }

    std::shared_ptr<MockDevice> addDevice(int slot, const std::string & name)
    {
        auto device = std::make_shared<MockDevice>(name);
        deviceService->setDevice(static_cast<size_t>(slot), device);
        return device;
    }

    //! A send rack slot has to hold something for a send edge to have anywhere to go.
    void addSendEffect(size_t index)
    {
        deviceService->sendEffectRack().setEffect(index, std::make_shared<Reverb>());
    }

    std::shared_ptr<AudioEngine> engine;
    std::shared_ptr<DeviceService> deviceService;
    std::shared_ptr<EditorService> editorService;
    std::shared_ptr<SongOverviewService> service;
};

using Kind = SongOverviewService::CellKind;

std::vector<Kind> kindsOf(const SongOverviewService::Node & node)
{
    std::vector<Kind> kinds;
    for (const auto & cell : node.chain) {
        kinds.push_back(cell.kind);
    }
    return kinds;
}

const SongOverviewService::Node & nodeForSlot(const SongOverviewService::Graph & graph, int slot)
{
    const auto it = std::ranges::find_if(graph.nodes, [slot](const auto & node) {
        return node.slot == slot && node.kind != SongOverviewService::NodeKind::SendEffect;
    });
    Q_ASSERT(it != graph.nodes.end());
    return *it;
}

int indexOfSlot(const SongOverviewService::Graph & graph, int slot)
{
    const auto it = std::ranges::find_if(graph.nodes, [slot](const auto & node) {
        return node.slot == slot && node.kind != SongOverviewService::NodeKind::SendEffect;
    });
    return it == graph.nodes.end() ? -1 : static_cast<int>(std::distance(graph.nodes.begin(), it));
}

} // namespace

void SongOverviewServiceTest::test_chain_preInserts_shouldPutTheFaderBeforeTheInserts()
{
    Fixture fixture;
    const auto device = fixture.addDevice(0, "Device 1");
    device->setFaderPosition(Device::FaderPosition::PreInserts);
    device->insertEffectRack().setEffect(0, std::make_shared<Gain>());

    const auto graph = fixture.service->build();
    const std::vector<Kind> expected { Kind::Source, Kind::Meter, Kind::Fader, Kind::Insert, Kind::Clip };
    QCOMPARE(kindsOf(nodeForSlot(graph, 0)), expected);
}

void SongOverviewServiceTest::test_chain_postInserts_shouldPutTheFaderAfterTheInserts()
{
    Fixture fixture;
    const auto device = fixture.addDevice(0, "Device 1");
    device->setFaderPosition(Device::FaderPosition::PostInserts);
    device->insertEffectRack().setEffect(0, std::make_shared<Gain>());

    const auto graph = fixture.service->build();
    const std::vector<Kind> expected { Kind::Source, Kind::Meter, Kind::Insert, Kind::Fader, Kind::Clip };
    QCOMPARE(kindsOf(nodeForSlot(graph, 0)), expected);
}

void SongOverviewServiceTest::test_sendTap_postFader_shouldLeaveFromTheEndOfTheChain()
{
    Fixture fixture;
    fixture.addSendEffect(0);
    const auto device = fixture.addDevice(0, "Device 1");
    device->setSendTap(Device::SendTap::PostFader);
    device->setReverbSend(0, 0.4f);

    const auto graph = fixture.service->build();
    const auto & node = nodeForSlot(graph, 0);
    const auto send = std::ranges::find_if(graph.edges, [](const auto & edge) {
        return edge.kind == SongOverviewService::EdgeKind::Send;
    });
    QVERIFY(send != graph.edges.end());
    QVERIFY(!send->preFader);
    QCOMPARE(send->tapCellIndex, static_cast<int>(node.chain.size()));
}

void SongOverviewServiceTest::test_sendTap_preFader_shouldLeaveFromJustBeforeTheFader()
{
    // The subtlety this view exists to show: the capture happens immediately before the fader
    // whichever side of the insert rack the fader sits, so the tap index is the fader's own -- but
    // the fader sits in a different place, so a "pre-fader" send has been through the inserts in
    // one arrangement and not in the other.
    const auto tapAndFaderFor = [](Device::FaderPosition position) {
        Fixture fixture;
        fixture.addSendEffect(0);
        const auto device = fixture.addDevice(0, "Device 1");
        device->setFaderPosition(position);
        device->setSendTap(Device::SendTap::PreFader);
        device->setReverbSend(0, 0.4f);
        device->insertEffectRack().setEffect(0, std::make_shared<Gain>());

        const auto graph = fixture.service->build();
        const auto & node = nodeForSlot(graph, 0);
        const auto send = std::ranges::find_if(graph.edges, [](const auto & edge) {
            return edge.kind == SongOverviewService::EdgeKind::Send;
        });
        Q_ASSERT(send != graph.edges.end());
        const auto fader = std::ranges::find_if(node.chain, [](const auto & cell) { return cell.kind == Kind::Fader; });
        return std::pair { send->tapCellIndex, static_cast<int>(std::distance(node.chain.begin(), fader)) };
    };

    const auto [preTap, preFader] = tapAndFaderFor(Device::FaderPosition::PreInserts);
    QCOMPARE(preTap, preFader);
    QCOMPARE(preFader, 2); // Source, Meter, Fader

    const auto [postTap, postFader] = tapAndFaderFor(Device::FaderPosition::PostInserts);
    QCOMPARE(postTap, postFader);
    QCOMPARE(postFader, 3); // Source, Meter, Insert, Fader

    // Same setting, different signal: with the fader after the inserts, the send has been through
    // them by the time it is taken.
    QVERIFY(postTap > preTap);
}

void SongOverviewServiceTest::test_subMixerMember_shouldRouteThroughTheGroupAndKeepItsSends()
{
    Fixture fixture;
    fixture.addSendEffect(0);
    const auto member = fixture.addDevice(0, "Device 1");
    member->setReverbSend(0, 0.3f);
    fixture.deviceService->setDevice(1, std::make_shared<SubMixerDevice>("Group"));
    QVERIFY(fixture.deviceService->addSubMixerMember(1, 0));

    const auto graph = fixture.service->build();
    const auto memberIndex = indexOfSlot(graph, 0);
    const auto groupIndex = indexOfSlot(graph, 1);

    // Its own path to the master is taken over by the group.
    const auto out = std::ranges::find_if(graph.edges, [&](const auto & edge) {
        return edge.fromNode == memberIndex && edge.kind != SongOverviewService::EdgeKind::Send;
    });
    QVERIFY(out != graph.edges.end());
    QCOMPARE(out->kind, SongOverviewService::EdgeKind::SubMixerMember);
    QCOMPARE(out->toNode, groupIndex);

    // But its sends survive: a send bus is a parallel tap, so nothing is heard twice.
    const auto send = std::ranges::find_if(graph.edges, [&](const auto & edge) {
        return edge.fromNode == memberIndex && edge.kind == SongOverviewService::EdgeKind::Send;
    });
    QVERIFY2(send != graph.edges.end(), "A claimed device lost its sends");
    QVERIFY(std::abs(send->amount - 0.3f) < 0.001f);
}

void SongOverviewServiceTest::test_ranking_shouldPlaceGroupsRightOfMembersAndMasterLast()
{
    Fixture fixture;
    fixture.addDevice(0, "Device 1");
    fixture.deviceService->setDevice(1, std::make_shared<SubMixerDevice>("Group"));
    QVERIFY(fixture.deviceService->addSubMixerMember(1, 0));

    const auto graph = fixture.service->build();
    const auto & member = nodeForSlot(graph, 0);
    const auto & group = nodeForSlot(graph, 1);
    QVERIFY2(group.column > member.column,
             qPrintable(QString { "Group at column %1, member at %2" }.arg(group.column).arg(member.column)));

    const auto master = std::ranges::find_if(graph.nodes, [](const auto & node) {
        return node.kind == SongOverviewService::NodeKind::Master;
    });
    QVERIFY(master != graph.nodes.end());
    for (const auto & node : graph.nodes) {
        if (node.kind != SongOverviewService::NodeKind::Master) {
            QVERIFY2(master->column > node.column,
                     qPrintable(QString { "Master at column %1 is not right of %2" }.arg(master->column).arg(node.name)));
        }
    }
}

void SongOverviewServiceTest::test_emptyProject_shouldStillHaveAMaster()
{
    Fixture fixture;
    const auto graph = fixture.service->build();

    QCOMPARE(graph.nodes.size(), size_t { 1 });
    QCOMPARE(graph.nodes.at(0).kind, SongOverviewService::NodeKind::Master);
    QVERIFY(graph.edges.empty());
    // And no empty columns held open for devices and sends that are not there.
    QCOMPARE(graph.columnCount, 1);
    QCOMPARE(graph.rowCount, 1);
}

void SongOverviewServiceTest::test_controller_shouldResolveEffectNamesRegisteredByEitherKey()
{
    Fixture fixture;
    const auto device = fixture.addDevice(0, "Device 1");
    // Gain is listed in the gallery by its type string, All-Pass Filter by its id. Matching on one
    // alone would leave half the effects on the map showing a raw type like "allPassFilter".
    device->insertEffectRack().setEffect(0, EffectFactory::createEffect(Gain::typeIdString()));
    device->insertEffectRack().setEffect(1, EffectFactory::createEffect(AllPassFilter::typeIdString()));

    const auto effectRackController = std::make_shared<EffectRackController>(fixture.deviceService, fixture.editorService);
    SongOverviewController controller { fixture.service, effectRackController };
    controller.refresh();

    QStringList labels;
    for (const auto & nodeVariant : controller.nodes()) {
        for (const auto & cellVariant : nodeVariant.toMap()["chain"].toList()) {
            if (const auto cell = cellVariant.toMap(); !cell["label"].toString().isEmpty()) {
                labels << cell["label"].toString();
            }
        }
    }

    QVERIFY2(labels.contains("Gain"), qPrintable("Labels: " + labels.join(", ")));
    QVERIFY2(labels.contains("All-Pass Filter"), qPrintable("Labels: " + labels.join(", ")));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SongOverviewServiceTest)
