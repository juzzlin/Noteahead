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

#ifndef RANDOM_SERVICE_HPP
#define RANDOM_SERVICE_HPP

#include <cstdint>
#include <random>

namespace noteahead::RandomService {
// Deterministic PRNG (shared across events)
using Generator = std::mt19937;
using GeneratorR = Generator &;
GeneratorR generator();

//! Seed the shared generator carries by default, and what reseed() restores it to.
inline constexpr uint32_t DefaultSeed { 0 };

//! Returns the shared generator to a known state. The generator lives for the whole process, so
//! without this a render inherits whatever position earlier playback and renders left it in, and
//! two renders of the same project differ. Render and playback start both call it.
void reseed(uint32_t seed = DefaultSeed);
} // namespace noteahead::RandomService

#endif // RANDOM_SERVICE_HPP
