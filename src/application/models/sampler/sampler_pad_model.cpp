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

#include "../../note_converter.hpp"
#include "domain/devices/sampler_device.hpp"

namespace noteahead {

SamplerPadModel::SamplerPadModel(SamplerDevice::SamplerDeviceS sampler, QObject * parent)
  : QAbstractListModel { parent }
  , m_sampler { std::move(sampler) }
{
    if (m_sampler) {
        connect(m_sampler.get(), &SamplerDevice::dataChanged, this, [this]() {
            emit dataChanged(index(0), index(PadCount - 1), { NoteName, FilePath, IsLoaded });
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
            emit dataChanged(index(0), index(PadCount - 1), { NoteName, FilePath, IsLoaded });
        });
    }
    endResetModel();
}

int SamplerPadModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return PadCount;
}

QVariant SamplerPadModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || !m_sampler) {
        return {};
    }

    const auto note = static_cast<uint8_t>(StartNote + index.row());
    const auto sample = m_sampler->sample(note);

    switch (role) {
    case Note:
        return note;
    case NoteName:
        return QString::fromStdString(NoteConverter::midiToString(note));
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
    roles[FilePath] = "filePath";
    roles[IsLoaded] = "isLoaded";
    return roles;
}

void SamplerPadModel::updatePad(int padIndex)
{
    if (padIndex < 0 || padIndex >= PadCount) {
        return;
    }
    const auto idx = index(padIndex);
    emit dataChanged(idx, idx, { FilePath, IsLoaded });
}

} // namespace noteahead
