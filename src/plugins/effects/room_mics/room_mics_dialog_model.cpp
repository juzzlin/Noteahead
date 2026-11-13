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

#include "room_mics_dialog_model.hpp"

#include "room_mics.hpp"

#include "../../../common/constants.hpp"

#include <algorithm>

namespace noteahead {

using namespace RoomAcoustics;

RoomMicsDialogModel::RoomMicsDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

QVariantMap RoomMicsDialogModel::layout() const
{
    const auto roomMics = std::dynamic_pointer_cast<RoomMics>(effect());
    if (!roomMics) {
        return {};
    }
    const auto settings = roomMics->settings();
    const auto layout = RoomLayout::from(settings);
    const auto point = [](double x, double y) {
        return QVariantMap { { "x", x }, { "y", y } };
    };

    QVariantMap map;
    map["width"] = settings.widthM;
    map["depth"] = settings.depthM;
    map["height"] = settings.heightM;
    map["sourceCentre"] = point(0.5 * (layout.sources[0].x + layout.sources[1].x), 0.5 * (layout.sources[0].y + layout.sources[1].y));
    QVariantList sources;
    for (auto && source : layout.sources) {
        sources.append(point(source.x, source.y));
    }
    map["sources"] = sources;

    QVariantList groups;
    for (size_t g = 0; g < MicGroupCount; g++) {
        const auto & pair = layout.mics[g];
        QVariantList mics;
        for (auto && mic : pair) {
            auto entry = point(mic.position.x, mic.position.y);
            entry["aimX"] = mic.aim.x;
            entry["aimY"] = mic.aim.y;
            mics.append(entry);
        }
        QVariantMap group;
        group["centre"] = point(0.5 * (pair[0].position.x + pair[1].position.x), 0.5 * (pair[0].position.y + pair[1].position.y));
        group["pattern"] = static_cast<int>(settings.groups[g].pattern);
        group["level"] = settings.groups[g].level;
        group["mics"] = mics;
        groups.append(group);
    }
    map["groups"] = groups;

    const auto times = reverbTimes(settings);
    QVariantList timeList;
    for (const double time : times) {
        timeList.append(time);
    }
    map["reverbTimes"] = timeList;
    map["reverbTime"] = midReverbTime(times);
    return map;
}

void RoomMicsDialogModel::moveSource(double x, double y)
{
    const auto roomMics = std::dynamic_pointer_cast<RoomMics>(effect());
    if (!roomMics) {
        return;
    }
    namespace C = Constants::NahdXml;
    const auto settings = roomMics->settings();
    setParameterValues({
      { C::xmlKeySourceX(), static_cast<float>(std::clamp(x / settings.widthM, 0.0, 1.0)) },
      { C::xmlKeySourceY(), static_cast<float>(std::clamp(y / settings.depthM, 0.0, 1.0)) },
    });
}

void RoomMicsDialogModel::moveMics(int group, double x, double y)
{
    const auto roomMics = std::dynamic_pointer_cast<RoomMics>(effect());
    if (!roomMics || group < 0 || group >= static_cast<int>(MicGroupCount)) {
        return;
    }
    namespace C = Constants::NahdXml;
    const auto settings = roomMics->settings();
    const auto index = static_cast<size_t>(group);
    if (static_cast<MicGroup>(group) == MicGroup::Close) {
        // By the offset from the middle of the source, as placed rather than as kept inside the room
        const double centreX = settings.sourceX * settings.widthM;
        const double centreY = settings.sourceY * settings.depthM;
        setParameterValues({
          { C::xmlKeyMicX(index), RoomMics::closeOffsetPosition(x - centreX) },
          { C::xmlKeyMicY(index), RoomMics::closeOffsetPosition(y - centreY) },
        });
        return;
    }
    setParameterValues({
      { C::xmlKeyMicX(index), static_cast<float>(std::clamp(x / settings.widthM, 0.0, 1.0)) },
      { C::xmlKeyMicY(index), static_cast<float>(std::clamp(y / settings.depthM, 0.0, 1.0)) },
    });
}

} // namespace noteahead
