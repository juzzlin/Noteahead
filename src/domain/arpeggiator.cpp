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

#include "arpeggiator.hpp"

#include "../application/service/random_service.hpp"

#include <algorithm>

namespace noteahead {

Arpeggiator::NoteInfoList Arpeggiator::generate(Pattern pattern, const NoteInfoList & inputNotes)
{
    if (inputNotes.empty()) {
        return {};
    }

    NoteInfoList uniqueNotes = inputNotes;
    std::sort(uniqueNotes.begin(), uniqueNotes.end(), [](auto && a, auto && b) {
        return a.note < b.note;
    });
    uniqueNotes.erase(std::unique(uniqueNotes.begin(), uniqueNotes.end(), [](auto && a, auto && b) {
        return a.note == b.note;
    }), uniqueNotes.end());

    NoteInfoList arpeggioSequence;
    switch (pattern) {
    case Pattern::Up:
        arpeggioSequence = uniqueNotes;
        break;
    case Pattern::Down:
        arpeggioSequence = uniqueNotes;
        std::reverse(arpeggioSequence.begin(), arpeggioSequence.end());
        break;
    case Pattern::UpDown:
        arpeggioSequence = uniqueNotes;
        for (int i = static_cast<int>(uniqueNotes.size()) - 2; i > 0; i--) {
            arpeggioSequence.push_back(uniqueNotes[static_cast<size_t>(i)]);
        }
        break;
    case Pattern::DownUp:
        arpeggioSequence = uniqueNotes;
        std::reverse(arpeggioSequence.begin(), arpeggioSequence.end());
        for (size_t i = 1; i < uniqueNotes.size() - 1; i++) {
            arpeggioSequence.push_back(uniqueNotes[i]);
        }
        break;
    case Pattern::Random:
        arpeggioSequence = uniqueNotes;
        std::shuffle(arpeggioSequence.begin(), arpeggioSequence.end(), RandomService::generator());
        break;
    }

    return arpeggioSequence;
}

} // namespace noteahead
