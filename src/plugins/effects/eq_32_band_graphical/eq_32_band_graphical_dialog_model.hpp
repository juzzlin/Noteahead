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

#ifndef EQ_32_BAND_GRAPHICAL_DIALOG_MODEL_HPP
#define EQ_32_BAND_GRAPHICAL_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class Eq32BandGraphicalDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit Eq32BandGraphicalDialogModel(QObject * parent = nullptr);

    // EQ 32-Band Graphical
    Q_INVOKABLE QVariantList response(int points) const;
    Q_INVOKABLE int bandCount() const;
    //! The nominal frequency a slider is labelled with, "31.5 Hz" or "1.25 kHz".
    Q_INVOKABLE QString bandLabel(quint32 bandIndex) const;
    //! How late Linear Phase makes the output at the effect's sample rate, in milliseconds.
    Q_INVOKABLE double linearPhaseLatencyMs() const;
    //! Every slider back to 0 dB, leaving the stereo mode and the output gain where they are, which
    //! is how this differs from the Flat preset.
    Q_INVOKABLE void flatten();
};

} // namespace noteahead

#endif // EQ_32_BAND_GRAPHICAL_DIALOG_MODEL_HPP
