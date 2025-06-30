// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef INSTRUMENT_LAYER_SERVICE_HPP
#define INSTRUMENT_LAYER_SERVICE_HPP

#include <QObject>

#include <vector>

#include "../../domain/instrument_layer.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

struct Position;

class InstrumentLayerService : public QObject
{
    Q_OBJECT

public:
    InstrumentLayerService();

    void clear();

    // <-- API for QML/UI -->

    Q_INVOKABLE quint64 addLayer(quint64 track, quint64 column, quint64 targetTrack, quint8 note, bool followSourceNote, quint8 velocity, bool applyTargetVelocity, bool followSourceVelocity, QString comment, bool enabled);
    Q_INVOKABLE quint64 addLayer(quint64 track, quint64 column, quint64 targetTrack, quint8 note, bool followSourceNote, quint8 velocity, bool applyTargetVelocity, bool followSourceVelocity, QString comment);

    using InstrumentLayerList = std::vector<InstrumentLayer>;
    InstrumentLayerList layersByColumn(quint64 track, quint64 column) const;
    InstrumentLayerList layersByTrack(quint64 track) const;
    InstrumentLayerList layers() const;

    Q_INVOKABLE bool hasLayers(quint64 track, quint64 column) const;

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

signals:
    void modified();

public slots:
    void deleteLayer(const InstrumentLayer & layerToDelete);
    void updateLayer(const InstrumentLayer & updatedLayer);

private:
    InstrumentLayerList m_layers;
};

} // namespace noteahead

#endif // INSTRUMENT_LAYER_SERVICE_HPP
