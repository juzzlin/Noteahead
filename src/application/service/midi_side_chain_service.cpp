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

#include "midi_side_chain_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/instrument_settings.hpp"
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/note_data.hpp"

#include <algorithm>

namespace noteahead {

static const auto TAG = "MidiSideChainService";

Song::EventList MidiSideChainService::renderToEvents(const Song & song, const Song::EventList & events, size_t endPosition)
{
    Song::EventList processedEvents { events };
    Song::EventList sideChainEvents;

    const auto maxTick = song.positionToTick(endPosition);
    const double msPerTick = 60'000.0 / static_cast<double>(song.beatsPerMinute() * song.linesPerBeat() * song.ticksPerLine());

    for (auto trackIndex : song.trackIndices()) {
        if (const auto instrument = song.instrument(trackIndex); instrument) {
            if (const auto & sideChainSettings = instrument->settings().midiEffects.midiSideChain; sideChainSettings.enabled) {
                juzzlin::L(TAG).debug() << "Side-chain enabled on track " << trackIndex;
                for (const auto & event : processedEvents) {
                    if (const auto noteData = event->noteData(); noteData && noteData->type() == NoteData::Type::NoteOn) {
                        if (noteData->track() == sideChainSettings.sourceTrackIndex && noteData->column() == sideChainSettings.sourceColumnIndex) {

                            const auto addTargetEvents = [&](const InstrumentSettings::MidiEffects::MidiSideChain::Target & target) {
                                if (target.enabled) {
                                    const auto lookaheadTicks = static_cast<size_t>(sideChainSettings.lookahead.count() / msPerTick);
                                    const size_t targetTick = event->tick() > lookaheadTicks ? event->tick() - lookaheadTicks : 0;
                                    const auto targetEvent = std::make_shared<Event>(targetTick, MidiCcData(trackIndex, sideChainSettings.sourceColumnIndex, target.controller, target.targetValue));
                                    sideChainEvents.push_back(targetEvent);

                                    const auto releaseTicks = static_cast<size_t>(sideChainSettings.release.count() / msPerTick);
                                    size_t releaseTick = event->tick() + releaseTicks;
                                    if (releaseTick >= maxTick) {
                                        releaseTick = maxTick > 0 ? maxTick - 1 : 0;
                                    }

                                    const auto releaseEvent = std::make_shared<Event>(releaseTick, MidiCcData(trackIndex, sideChainSettings.sourceColumnIndex, target.controller, target.releaseValue));
                                    sideChainEvents.push_back(releaseEvent);
                                }
                            };

                            for (const auto & target : sideChainSettings.targets) {
                                addTargetEvents(target);
                            }
                        }
                    }
                }
            }
        }
    }

    processedEvents.insert(processedEvents.end(), sideChainEvents.begin(), sideChainEvents.end());
    std::sort(processedEvents.begin(), processedEvents.end(), [](const auto & a, const auto & b) {
        return a->tick() < b->tick();
    });
    return processedEvents;
}

} // namespace noteahead
