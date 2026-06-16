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

#include "sampler_model.hpp"

#include "../../../domain/devices/sampler_device.hpp"

namespace noteahead {

SamplerModel::SamplerModel(SamplerDeviceS sampler, QObject * parent)
  : QAbstractListModel { parent }
  , m_sampler { std::move(sampler) }
{
}

SamplerModel::~SamplerModel() = default;

int SamplerModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return 128;
}

QVariant SamplerModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || !m_sampler) {
        return {};
    }

    const auto note = static_cast<uint8_t>(index.row());
    const auto sample = m_sampler->sample(note);

    switch (static_cast<SamplerRoles>(role)) {
    case SamplerRoles::Note:
        return note;
    case SamplerRoles::FilePath:
        return sample ? QString::fromStdString(sample->filePath) : QString {};
    case SamplerRoles::IsLoaded:
        return sample != nullptr;
    default:
        return {};
    }
}

QHash<int, QByteArray> SamplerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[static_cast<int>(SamplerRoles::Note)] = "note";
    roles[static_cast<int>(SamplerRoles::FilePath)] = "filePath";
    roles[static_cast<int>(SamplerRoles::IsLoaded)] = "isLoaded";
    return roles;
}

void SamplerModel::loadSample(int note, const QString & filePath)
{
    if (m_sampler) {
        m_sampler->loadSample(static_cast<uint8_t>(note), filePath.toStdString());
        const auto idx = index(note);
        emit dataChanged(idx, idx, { static_cast<int>(SamplerRoles::FilePath), static_cast<int>(SamplerRoles::IsLoaded) });
    }
}

void SamplerModel::clearSample(int note)
{
    if (m_sampler) {
        m_sampler->clearSample(static_cast<uint8_t>(note));
        const auto idx = index(note);
        emit dataChanged(idx, idx, { static_cast<int>(SamplerRoles::FilePath), static_cast<int>(SamplerRoles::IsLoaded) });
    }
}

void SamplerModel::playSample(int note, double velocity)
{
    if (m_sampler) {
        m_sampler->processMidiNoteOn(static_cast<uint8_t>(note), static_cast<uint8_t>(velocity * 127.0));
    }
}

} // namespace noteahead
