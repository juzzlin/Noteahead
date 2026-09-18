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

#ifndef FFT_HPP
#define FFT_HPP

namespace noteahead::Fft {

//! Iterative Cooley-Tukey DIT radix-2 FFT (in-place, N must be a power of 2).
void forward(double * re, double * im, int N);

//! Spectrum of @p N real samples, written to @p re and @p im as the bins 0 to N/2 inclusive.
//!
//! Half the work of packing real input into forward() and throwing the mirrored half away: the
//! even and odd samples are packed as the real and imaginary parts of one N/2-point transform, and
//! the spectrum of the original is recovered from its conjugate symmetry. @p re and @p im must
//! hold N/2+1 values and may not alias @p input. Bins above N/2 are left untouched; a real signal
//! has nothing there that the bins below do not already carry.
void forwardReal(const double * input, double * re, double * im, int N);

//! Inverse of forward(), scaled by 1/N (in-place, N must be a power of 2).
void inverse(double * re, double * im, int N);

} // namespace noteahead::Fft

#endif // FFT_HPP
