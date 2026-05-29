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

#ifndef DENORMAL_PROTECTION_HPP
#define DENORMAL_PROTECTION_HPP

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <pmmintrin.h>
#include <xmmintrin.h>
#endif

namespace noteahead {

/**
 * @brief Enables hardware denormal protection on the current thread.
 *
 * This function sets the Flush-To-Zero (FTZ) and Denormals-Are-Zero (DAZ)
 * flags in the MXCSR register for x86/x64 processors. This prevents
 * significant CPU performance penalties when processing extremely small
 * floating-point numbers in audio DSP code.
 */
inline void enableHardwareDenormalProtection()
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif
}

} // namespace noteahead

#endif // DENORMAL_PROTECTION_HPP
