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

#include "domain/dsp/audio_context.hpp"
#include "domain/effects/effect.hpp"
#include <memory>
#include <mutex>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class EffectRack
{
public:
    using EffectS = std::shared_ptr<Effect>;

    EffectRack();
    ~EffectRack();

    void setEffect(size_t index, EffectS effect);
    void removeEffect(size_t index);
    EffectS effect(size_t index) const;
    std::vector<EffectS> effects() const;
    size_t effectCount() const;

    void process(AudioContext & outputContext, const double * sendBus, size_t effectIndex);
    void processInPlace(AudioContext & context);
    std::vector<size_t> sidechainDependencies() const;
    void reset();
    void setBpm(float bpm);
    void clear();

    void serializeEffectsToXml(QXmlStreamWriter & writer) const;
    void deserializeEffectsFromXml(QXmlStreamReader & reader);
    void deserializeEffect(QXmlStreamReader & reader);

    bool exportEffectSettings(size_t index, QXmlStreamWriter & writer) const;
    bool importEffectSettings(size_t index, QXmlStreamReader & reader);

private:
    std::vector<EffectS> m_effects;
    mutable std::recursive_mutex m_mutex;
};

} // namespace noteahead

#endif // EFFECT_RACK_HPP
