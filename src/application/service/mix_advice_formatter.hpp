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

#ifndef MIX_ADVICE_FORMATTER_HPP
#define MIX_ADVICE_FORMATTER_HPP

#include "../../domain/utility/mix_advisor.hpp"

#include <QString>

namespace noteahead::MixAdviceFormatter {

//! One finding as a sentence: what was measured, and what it tends to sound like.
//!
//! The wording lives here rather than beside the rules that produce it, because this is the layer
//! that can translate. MixAdvisor deals in codes and decibels and so stays testable without anyone
//! matching strings against it.
//!
//! Never empty for a finding the advisor can produce, which mix_advisor_test asserts across every
//! topic and severity: a blank line in the report would look like the analysis failed.
//!
//! Every sentence names its translation context as a literal, tedious as that is: lupdate reads the
//! source rather than running it, and a context behind a constant extracts nothing at all while
//! compiling perfectly.
QString sentence(const MixAdvisor::Finding & finding);

} // namespace noteahead::MixAdviceFormatter

#endif // MIX_ADVICE_FORMATTER_HPP
