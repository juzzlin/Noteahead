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

#ifndef EFFECT_RACK_HPP
#define EFFECT_RACK_HPP

#include "../dsp/audio_context.hpp"
#include "effect.hpp"
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class EffectRack
{
public:
    using EffectS = std::shared_ptr<Effect>;

    EffectRack();
    ~EffectRack();

    void setEffect(size_t index, EffectS effect);
    void swapEffects(size_t indexA, size_t indexB);
    //! Move the effect in sourceIndex to targetIndex, shifting the slots in between by one so that
    //! the relative order of the other effects is preserved. Repeated swaps in one go.
    void moveEffect(size_t sourceIndex, size_t targetIndex);
    void removeEffect(size_t index);
    EffectS effect(size_t index) const;
    std::vector<EffectS> effects() const;
    //! Allocation-free variant: fills out (cleared first) with the non-null effects. Used on the
    //! audio thread where allocating every callback must be avoided.
    void effects(std::vector<EffectS> & out) const;
    size_t effectCount() const;
    bool hasEffects() const;

    //! Monotonically increasing counter bumped whenever the effect list changes (effects added,
    //! removed, swapped, cleared or deserialized). Lets callers cache a snapshot of effects() and
    //! only refresh it when this changes, avoiding a per-audio-callback copy. Cheap atomic read.
    uint64_t version() const;

    //! Whether the whole rack is active. When disabled the rack is bypassed (audio passes through
    //! unchanged). Enabled by default. Used e.g. to switch off effects when rendering individual tracks.
    bool enabled() const;
    void setEnabled(bool enabled);

    void process(AudioContext & outputContext, const double * sendBus, size_t effectIndex);
    void processInPlace(AudioContext & context);
    //! Whether every enabled effect in the rack is back at rest. The engine uses this to decide
    //! that a silent device really can stop being processed.
    bool isSettled() const;

    std::vector<size_t> sidechainDependencies() const;
    //! Allocation-free variant: fills out (cleared first) with the sidechain source device indices.
    //! Used on the audio thread where allocating every callback must be avoided.
    void sidechainDependencies(std::vector<size_t> & out) const;
    void reset();
    void setBpm(float bpm);
    void clear();

    void serializeEffectsToXml(ProjectWriter & writer) const;
    void deserializeEffectsFromXml(ProjectReader & reader);
    void deserializeEffect(ProjectReader & reader);

    bool exportEffectSettings(size_t index, ProjectWriter & writer) const;
    bool importEffectSettings(size_t index, ProjectReader & reader);

    //! Duplicate the effect in sourceIndex into targetIndex (in-memory clone). Returns false if the
    //! source slot is empty or source and target are the same slot.
    bool copyEffect(size_t sourceIndex, size_t targetIndex);

    //! Replace this rack's contents with in-memory clones of another rack's effects. The two racks
    //! stay independent afterwards: nothing is shared, not even the effect state.
    void copyFrom(const EffectRack & other);

private:
    void markChanged();

    //! Builds an independent copy of the given effect, parameters and enabled state included.
    EffectS cloneEffect(const EffectS & source) const;

    std::vector<EffectS> m_effects;
    std::atomic<uint64_t> m_version { 0 };
    std::atomic<bool> m_enabled { true };
    mutable std::recursive_mutex m_mutex;
};

} // namespace noteahead

#endif // EFFECT_RACK_HPP
