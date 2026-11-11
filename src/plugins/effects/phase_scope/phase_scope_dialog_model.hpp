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

#ifndef PHASE_SCOPE_DIALOG_MODEL_HPP
#define PHASE_SCOPE_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QVariantMap>

namespace noteahead {

class PhaseScopeDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit PhaseScopeDialogModel(QObject * parent = nullptr);

    // PhaseScope
    //! The delay as it is applied, in whole samples at the effect's rate.
    Q_INVOKABLE int delaySamples() const;
    //! Sets the delay to exactly @p samples at the effect's rate, for nudging a sample at a time.
    Q_INVOKABLE void setDelaySamples(int samples);
    //! The Phase Scope runs only while its dialog is open.
    Q_INVOKABLE void setActive(bool active);
    //! hasReference, correlation, referenceDb, signalDb and sumGainDb. Empty for no such effect.
    Q_INVOKABLE QVariantMap reading() const;
    //! The latest hit: reference and signal, each decimated to at most @p maxPoints.
    Q_INVOKABLE QVariantMap capture(int maxPoints) const;
};

} // namespace noteahead

#endif // PHASE_SCOPE_DIALOG_MODEL_HPP
