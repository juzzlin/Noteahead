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

#ifndef SONG_OVERVIEW_CONTROLLER_HPP
#define SONG_OVERVIEW_CONTROLLER_HPP

#include <QObject>
#include <QVariantList>

#include <memory>

namespace noteahead {

class EffectRackController;
class SongOverviewService;

//! Hands the Song Overview's graph to QML.
//!
//! Thin on purpose: the graph and its layout are the service's, and everything here is translation.
//! The one thing it adds is resolving an effect's raw type to the name the gallery shows, which it
//! can do and the service cannot -- that list lives in a controller, and a service must not reach up
//! into the view.
class SongOverviewController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList nodes READ nodes NOTIFY graphChanged)
    Q_PROPERTY(QVariantList edges READ edges NOTIFY graphChanged)
    Q_PROPERTY(int columnCount READ columnCount NOTIFY graphChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY graphChanged)

public:
    using SongOverviewServiceS = std::shared_ptr<SongOverviewService>;
    using EffectRackControllerS = std::shared_ptr<EffectRackController>;

    SongOverviewController(SongOverviewServiceS songOverviewService, EffectRackControllerS effectRackController, QObject * parent = nullptr);

    //! Rebuilds from the current devices. Nothing is cached, so this is also how the view refreshes.
    Q_INVOKABLE void refresh();

    QVariantList nodes() const;
    QVariantList edges() const;
    int columnCount() const;
    int rowCount() const;

signals:
    void graphChanged();

    //! The view asking for a node's own editor. Routed through UiService in QML rather than opened
    //! from here, which is how every other dialog in the application is reached.
    void deviceOpenRequested(int slotIndex);
    void masterEffectsOpenRequested();
    void effectSendsOpenRequested(const QString & deviceName);

public slots:
    void openNode(int nodeIndex);

private:
    QString effectDisplayName(const QString & type, const QString & typeId) const;

    SongOverviewServiceS m_songOverviewService;
    EffectRackControllerS m_effectRackController;

    QVariantList m_nodes;
    QVariantList m_edges;
    int m_columnCount { 0 };
    int m_rowCount { 0 };
};

} // namespace noteahead

#endif // SONG_OVERVIEW_CONTROLLER_HPP
