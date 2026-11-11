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

#include "compressor_dialog_model.hpp"

#include "compressor.hpp"

namespace noteahead {

CompressorDialogModel::CompressorDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float CompressorDialogModel::reductionDb() const
{
    const auto compressor = std::dynamic_pointer_cast<Compressor>(this->effect());
    return compressor ? compressor->reductionDb() : 0.0f;
}

} // namespace noteahead
