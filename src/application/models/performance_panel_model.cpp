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

#include "performance_panel_model.hpp"

#include "../../common/constants.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"

#include <algorithm>

namespace noteahead {

PerformancePanelModel::PerformancePanelModel(QObject * parent)
  : QAbstractListModel { parent }
{
}

int PerformancePanelModel::rowCount(const QModelIndex & parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_elements.size());
}

QVariant PerformancePanelModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const auto & element = m_elements.at(static_cast<size_t>(index.row()));
    switch (static_cast<Roles>(role)) {
    case Roles::TypeRole:
        return element.type;
    case Roles::SourceIdRole:
        return element.sourceId;
    }
    return {};
}

QHash<int, QByteArray> PerformancePanelModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::TypeRole), "type" },
        { static_cast<int>(Roles::SourceIdRole), "sourceId" }
    };
}

bool PerformancePanelModel::addElement(const QString & type, const QString & sourceId)
{
    if (type.isEmpty() || contains(type, sourceId)) {
        return false;
    }
    const auto row = rowCount();
    beginInsertRows({}, row, row);
    m_elements.push_back({ type, sourceId });
    endInsertRows();
    emit countChanged();
    emit modified();
    return true;
}

void PerformancePanelModel::removeElement(int row)
{
    if (row < 0 || row >= rowCount()) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_elements.erase(m_elements.begin() + row);
    endRemoveRows();
    emit countChanged();
    emit modified();
}

bool PerformancePanelModel::moveElement(int from, int to)
{
    if (from < 0 || from >= rowCount() || to < 0 || to >= rowCount() || from == to) {
        return false;
    }
    // A move, not a remove and an insert: the view keeps the element it has, and with it a pad's
    // controller and a meter's running analysis. Qt names the row the element goes in front of.
    beginMoveRows({}, from, from, {}, to > from ? to + 1 : to);
    const auto element = m_elements.at(static_cast<size_t>(from));
    m_elements.erase(m_elements.begin() + from);
    m_elements.insert(m_elements.begin() + to, element);
    endMoveRows();
    emit modified();
    return true;
}

bool PerformancePanelModel::contains(const QString & type, const QString & sourceId) const
{
    return std::ranges::find(m_elements, Element { type, sourceId }) != m_elements.end();
}

const PerformancePanelModel::ElementList & PerformancePanelModel::elements() const
{
    return m_elements;
}

void PerformancePanelModel::clear()
{
    if (m_elements.empty()) {
        return;
    }
    beginResetModel();
    m_elements.clear();
    endResetModel();
    emit countChanged();
}

void PerformancePanelModel::serializeToXml(ProjectWriter & writer) const
{
    if (m_elements.empty()) {
        return;
    }
    writer.writeStartElement(Constants::NahdXml::xmlKeyPerformancePanel());
    for (auto && element : m_elements) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyPanelElement());
        writer.writeAttribute(Constants::NahdXml::xmlKeyType(), element.type);
        writer.writeAttribute(Constants::NahdXml::xmlKeySourceId(), element.sourceId);
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

void PerformancePanelModel::deserializeFromXml(ProjectReader & reader)
{
    ElementList elements;
    while (!(reader.isEndElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPerformancePanel())) && !reader.atEnd()) {
        if (reader.isStartElement() && !reader.name().compare(Constants::NahdXml::xmlKeyPanelElement())) {
            Element element { reader.attribute(Constants::NahdXml::xmlKeyType()).toString(), reader.attribute(Constants::NahdXml::xmlKeySourceId()).toString() };
            // A type this version does not know is kept, so that saving here does not drop it
            if (!element.type.isEmpty() && std::ranges::find(elements, element) == elements.end()) {
                elements.push_back(std::move(element));
            }
        }
        reader.readNext();
    }
    beginResetModel();
    m_elements = std::move(elements);
    endResetModel();
    emit countChanged();
}

} // namespace noteahead
