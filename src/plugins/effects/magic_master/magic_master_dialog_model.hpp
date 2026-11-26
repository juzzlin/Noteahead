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

#ifndef MAGIC_MASTER_DIALOG_MODEL_HPP
#define MAGIC_MASTER_DIALOG_MODEL_HPP

#include "../../../view/controllers/effect_dialog_model.hpp"

#include <QString>

namespace noteahead {

class MagicMasterDialogModel : public EffectDialogModel
{
    Q_OBJECT

public:
    explicit MagicMasterDialogModel(QObject * parent = nullptr);

    Q_INVOKABLE float shortTermLufs() const;
    Q_INVOKABLE float integratedLufs() const;
    Q_INVOKABLE float truePeakDb() const;
    Q_INVOKABLE float truePeakHoldDb() const;
    Q_INVOKABLE float appliedDriveDb() const;
    Q_INVOKABLE bool isCalibrated() const;
    Q_INVOKABLE QString songContextSummary() const;

    Q_INVOKABLE float subHpfHz() const;
    Q_INVOKABLE float monoBassHz() const;
    Q_INVOKABLE float autoEqMaxMoveDb() const;
    Q_INVOKABLE float resonanceCutDb() const;
    Q_INVOKABLE float multibandReductionDb(int band) const;
    Q_INVOKABLE float stereoCorrelation() const;
    Q_INVOKABLE float stereoWidthPercent() const;
    Q_INVOKABLE float clipperReductionDb() const;
    Q_INVOKABLE float limiterReductionDb() const;

    Q_INVOKABLE void resetMeters();
    Q_INVOKABLE void clearCalibration();
};

} // namespace noteahead

#endif // MAGIC_MASTER_DIALOG_MODEL_HPP
