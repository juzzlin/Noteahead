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

#include "sampler_pad_model.hpp"

#include "../../../domain/devices/sampler_device.hpp"
#include "../../note_converter.hpp"

namespace noteahead {

SamplerPadModel::SamplerPadModel(SamplerDevice::SamplerDeviceS sampler, QObject * parent)
  : QAbstractListModel { parent }
  , m_sampler { std::move(sampler) }
{
    if (m_sampler) {
        connect(m_sampler.get(), &SamplerDevice::dataChanged, this, [this]() {
            // Toggling chromatic mode remaps every pad's note and range, so refresh all roles.
            emit dataChanged(index(0), index(SamplerDevice::padCount - 1), { Note, NoteName, RangeLabel, FilePath, IsLoaded });
        });
    }
}

void SamplerPadModel::setSampler(SamplerDevice::SamplerDeviceS sampler)
{
    if (m_sampler == sampler) {
        return;
    }

    beginResetModel();
    if (m_sampler) {
        m_sampler->disconnect(this);
    }
    m_sampler = std::move(sampler);
    if (m_sampler) {
        connect(m_sampler.get(), &SamplerDevice::dataChanged, this, [this]() {
            // Toggling chromatic mode remaps every pad's note and range, so refresh all roles.
            emit dataChanged(index(0), index(SamplerDevice::padCount - 1), { Note, NoteName, RangeLabel, FilePath, IsLoaded });
        });
    }
    endResetModel();
}

int SamplerPadModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return SamplerDevice::padCount;
}

int SamplerPadModel::noteForPad(int padIndex) const
{
    // Without a device there is nothing to be in chromatic mode, so the drum layout is the sane default.
    return m_sampler ? m_sampler->noteForPad(padIndex) : SamplerDevice::padStartNote + padIndex;
}

QString SamplerPadModel::chromaticRangeLabel(int padIndex) const
{
    if (!m_sampler) {
        return {};
    }

    const int root = noteForPad(padIndex);
    if (root >= static_cast<int>(SamplerDevice::maxSamples) || !m_sampler->sample(static_cast<uint8_t>(root))) {
        return {};
    }

    // A set pad covers from its own note up to the next set pad below the one above it, with the
    // lowest extending down to note 0 and the highest up to the top of the keyboard. Every note is
    // searched rather than every twelfth, because pads are placed wherever the instrument was sampled.
    bool isLowestSet = true;
    for (int r = root - 1; r >= 0; r--) {
        if (m_sampler->sample(static_cast<uint8_t>(r))) {
            isLowestSet = false;
            break;
        }
    }

    int nextRoot = -1;
    for (int r = root + 1; r < static_cast<int>(SamplerDevice::maxSamples); r++) {
        if (m_sampler->sample(static_cast<uint8_t>(r))) {
            nextRoot = r;
            break;
        }
    }

    const int startNote = isLowestSet ? 0 : root;
    // Up to, but not including, wherever the next pad starts.
    const int endNote = nextRoot >= 0 ? nextRoot - 1 : static_cast<int>(SamplerDevice::maxSamples) - 1;

    return QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(startNote)))
      + " - " + QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(endNote)));
}

QVariant SamplerPadModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || !m_sampler) {
        return {};
    }

    const int noteValue = noteForPad(index.row());
    const bool inRange = noteValue < static_cast<int>(SamplerDevice::maxSamples);
    const auto sample = inRange ? m_sampler->sample(static_cast<uint8_t>(noteValue)) : nullptr;

    switch (role) {
    case Note:
        return noteValue;
    case NoteName:
        return inRange ? QString::fromStdString(NoteConverter::midiToString(static_cast<uint8_t>(noteValue))) : QString {};
    case RangeLabel:
        return m_sampler->chromaticMode() ? chromaticRangeLabel(index.row()) : QString {};
    case FilePath:
        return sample ? QString::fromStdString(sample->filePath) : QString {};
    case IsLoaded:
        return sample != nullptr;
    default:
        return {};
    }
}

QHash<int, QByteArray> SamplerPadModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Note] = "note";
    roles[NoteName] = "noteName";
    roles[RangeLabel] = "rangeLabel";
    roles[FilePath] = "filePath";
    roles[IsLoaded] = "isLoaded";
    return roles;
}

void SamplerPadModel::updatePad(int padIndex)
{
    if (padIndex < 0 || padIndex >= SamplerDevice::padCount) {
        return;
    }
    const auto idx = index(padIndex);
    emit dataChanged(idx, idx, { FilePath, IsLoaded });
}

void SamplerPadModel::updateAllPads()
{
    if (const auto last = rowCount() - 1; last >= 0) {
        emit dataChanged(index(0), index(last), { Note, NoteName, RangeLabel, FilePath, IsLoaded });
    }
}

} // namespace noteahead
