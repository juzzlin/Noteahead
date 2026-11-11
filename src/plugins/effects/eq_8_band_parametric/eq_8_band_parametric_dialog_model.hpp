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

#ifndef EQ_8_BAND_PARAMETRIC_DIALOG_MODEL_HPP
#define EQ_8_BAND_PARAMETRIC_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

namespace noteahead {

class Eq8BandParametricDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit Eq8BandParametricDialogModel(QObject * parent = nullptr);

    //! The response of the path the equalizer shapes, in dB at @p points frequencies.
    Q_INVOKABLE QVariantList response(int points) const;
    //! The flat path when the stereo mode leaves one unshaped, or empty when both are shaped.
    Q_INVOKABLE QVariantList passThroughResponse(int points) const;
    //! Sets a band's type and, when the type has an opinion about it, opens its Q at the width
    //! that type is reached for at. Picking the type a band already has leaves its Q alone.
    Q_INVOKABLE void setBandType(quint32 bandIndex, int bandType);
};

} // namespace noteahead

#endif // EQ_8_BAND_PARAMETRIC_DIALOG_MODEL_HPP
