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

#include "magic_master_dialog_model.hpp"

#include "magic_master.hpp"

namespace noteahead {

MagicMasterDialogModel::MagicMasterDialogModel(QObject * parent)
  : EffectDialogModel { parent }
{
}

float MagicMasterDialogModel::shortTermLufs() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->shortTermLufs();
    }
    return -70.0f;
}

float MagicMasterDialogModel::integratedLufs() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->integratedLufs();
    }
    return -70.0f;
}

float MagicMasterDialogModel::truePeakDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->truePeakDb();
    }
    return -70.0f;
}

float MagicMasterDialogModel::truePeakHoldDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->truePeakHoldDb();
    }
    return -70.0f;
}

float MagicMasterDialogModel::appliedDriveDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->appliedDriveDb();
    }
    return 0.0f;
}

bool MagicMasterDialogModel::isCalibrated() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->isCalibrated();
    }
    return false;
}

QString MagicMasterDialogModel::songContextSummary() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return QString::fromStdString(mm->songContextSummary());
    }
    return {};
}

float MagicMasterDialogModel::subHpfHz() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->subHpfHz();
    }
    return 28.0f;
}

float MagicMasterDialogModel::monoBassHz() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->monoBassHz();
    }
    return 125.0f;
}

float MagicMasterDialogModel::autoEqMaxMoveDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->autoEqMaxMoveDb();
    }
    return 0.0f;
}

float MagicMasterDialogModel::resonanceCutDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->resonanceCutDb();
    }
    return 0.0f;
}

float MagicMasterDialogModel::multibandReductionDb(int band) const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->multibandReductionDb(band);
    }
    return 0.0f;
}

float MagicMasterDialogModel::stereoCorrelation() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->stereoCorrelation();
    }
    return 1.0f;
}

float MagicMasterDialogModel::stereoWidthPercent() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->stereoWidthPercent();
    }
    return 100.0f;
}

float MagicMasterDialogModel::clipperReductionDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->clipperReductionDb();
    }
    return 0.0f;
}

float MagicMasterDialogModel::limiterReductionDb() const
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        return mm->limiterReductionDb();
    }
    return 0.0f;
}

void MagicMasterDialogModel::resetMeters()
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        mm->requestResetMeters();
    }
}

void MagicMasterDialogModel::clearCalibration()
{
    if (const auto mm { std::dynamic_pointer_cast<MagicMaster>(effect()) }; mm) {
        mm->clearCalibration();
    }
}

} // namespace noteahead
