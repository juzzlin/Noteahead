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

#include "instrument_layer_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>
#include <ranges>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "InstrumentLayerService";

InstrumentLayerService::InstrumentLayerService() = default;

quint64 InstrumentLayerService::addLayer(quint64 track, quint64 column, quint64 targetTrack, quint8 note, bool followSourceNote, quint8 velocity, bool applyTargetVelocity, bool followSourceVelocity, QString comment, bool enabled)
{
    InstrumentLayer::Parameters params;
    params.targetTrack = targetTrack;
    params.note = note;
    params.followSourceNote = followSourceNote;
    params.velocity = velocity;
    params.applyTargetVelocity = applyTargetVelocity;
    params.followSourceVelocity = followSourceVelocity;

    const auto maxIdItem = std::max_element(m_layers.begin(), m_layers.end(), [](auto && lhs, auto && rhs) { return lhs.id() < rhs.id(); });
    const auto id = maxIdItem != m_layers.end() ? (*maxIdItem).id() + 1 : 1;

    const AutomationLocation location = { 0, track, column }; // Don't care about pattern index

    const auto layer = InstrumentLayer { id, location, params, comment, enabled };

    m_layers.push_back(layer);

    emit modified();

    juzzlin::L(TAG).info() << "Instrument layer added: " << layer.toString().toStdString();

    return layer.id();
}

quint64 InstrumentLayerService::addLayer(quint64 track, quint64 column, quint64 targetTrack, quint8 note, bool followSourceNote, quint8 velocity, bool applyTargetVelocity, bool followSourceVelocity, QString comment)
{
    return addLayer(track, column, targetTrack, note, followSourceNote, velocity, applyTargetVelocity, followSourceVelocity, comment, true);
}

void InstrumentLayerService::deleteLayer(const InstrumentLayer & layerToDelete)
{
    if (const auto iter = std::ranges::find_if(m_layers, [&](auto && existingLayer) {
            return layerToDelete.id() == existingLayer.id();
        });
        iter != m_layers.end()) {
        m_layers.erase(iter);
        emit modified();
        juzzlin::L(TAG).info() << "Instrument layer deleted: " << layerToDelete.toString().toStdString();
    } else {
        juzzlin::L(TAG).error() << "No such instrument layer id to delete: " << layerToDelete.id();
    }
}

void InstrumentLayerService::updateLayer(const InstrumentLayer & updatedLayer)
{
    if (const auto iter = std::ranges::find_if(m_layers, [&](auto && existingLayer) {
            return updatedLayer.id() == existingLayer.id();
        });
        iter != m_layers.end()) {
        if (const auto oldLayer = *iter; oldLayer != updatedLayer) {
            *iter = updatedLayer;
            emit modified();
            juzzlin::L(TAG).info() << "Instrument layer updated: " << updatedLayer.toString().toStdString();
        } else {
            juzzlin::L(TAG).info() << "No changes for instrument layer: " << updatedLayer.toString().toStdString();
        }
    } else {
        juzzlin::L(TAG).error() << "No such instrument layer id: " << updatedLayer.id();
    }
}

bool InstrumentLayerService::hasLayers(quint64 track, quint64 column) const
{
    const auto match = std::ranges::find_if(m_layers, [&](auto && layer) {
        auto && location = layer.location();
        return location.track() == track && location.column() == column;
    });
    return match != m_layers.end();
}

InstrumentLayerService::InstrumentLayerList InstrumentLayerService::layersByColumn(quint64 track, quint64 column) const
{
    InstrumentLayerList layers;
    std::ranges::copy(m_layers
                        | std::views::filter([&](auto && layer) {
                              auto && location = layer.location();
                              return location.track() == track && location.column() == column;
                          }),
                      std::back_inserter(layers));
    return layers;
}

InstrumentLayerService::InstrumentLayerList InstrumentLayerService::layersByTrack(quint64 track) const
{
    InstrumentLayerList layers;
    std::ranges::copy(m_layers
                        | std::views::filter([&](auto && layer) {
                              auto && location = layer.location();
                              return location.track() == track;
                          }),
                      std::back_inserter(layers));
    return layers;
}

InstrumentLayerService::InstrumentLayerList InstrumentLayerService::layers() const
{
    return m_layers;
}

void InstrumentLayerService::clear()
{
    juzzlin::L(TAG).info() << "Clearing";

    m_layers = {};
}

void InstrumentLayerService::deserializeFromXml(QXmlStreamReader & reader)
{
    juzzlin::L(TAG).info() << "Deserializing";
    m_layers = {};
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrumentLayers()))) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyInstrumentLayer())) {
            if (const auto layer = InstrumentLayer::deserializeFromXml(reader); layer) {
                layer->setId(m_layers.size() + 1); // Assign id on-the-fly
                m_layers.push_back(*layer);
            }
        }
        reader.readNext();
    }
}

void InstrumentLayerService::serializeToXml(QXmlStreamWriter & writer) const
{
    juzzlin::L(TAG).info() << "Serializing";

    writer.writeStartElement(Constants::NahdXml::xmlKeyInstrumentLayers());

    for (const auto & layer : m_layers) {
        layer.serializeToXml(writer);
    }

    writer.writeEndElement(); // InstrumentLayerService
}

} // namespace noteahead
