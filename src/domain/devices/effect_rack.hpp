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

#include "effect.hpp"
#include <vector>
#include <memory>
#include <mutex>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class EffectRack
{
public:
    using EffectS = std::shared_ptr<Effect>;

    EffectRack();
    ~EffectRack();

    void addEffect(EffectS effect);
    void removeEffect(size_t index);
    EffectS effect(size_t index) const;
    size_t effectCount() const;

    void process(float * output, const float * sendBus, size_t effectIndex, uint32_t frameCount, uint32_t sampleRate);

    void reset();

    void serializeToXml(QXmlStreamWriter & writer) const;
    void deserializeFromXml(QXmlStreamReader & reader);

private:
    std::vector<EffectS> m_effects;
    mutable std::mutex m_mutex;
};

} // namespace noteahead

#endif // EFFECT_RACK_HPP
