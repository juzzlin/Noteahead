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

#ifndef SAMPLER_MODEL_HPP
#define SAMPLER_MODEL_HPP

#include <QAbstractListModel>
#include <memory>

namespace noteahead {

class SamplerDevice;

enum class SamplerRoles {
    Note = Qt::UserRole + 1,
    FilePath,
    IsLoaded
};

class SamplerModel : public QAbstractListModel
{
    Q_OBJECT

public:
    using SamplerDeviceS = std::shared_ptr<SamplerDevice>;
    explicit SamplerModel(SamplerDeviceS sampler, QObject * parent = nullptr);
    ~SamplerModel() override;

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadSample(int note, const QString & filePath);
    Q_INVOKABLE void clearSample(int note);
    Q_INVOKABLE void playSample(int note, double velocity);

private:
    SamplerDeviceS m_sampler;
};

} // namespace noteahead

#endif // SAMPLER_MODEL_HPP
