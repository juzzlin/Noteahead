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

#ifndef MANUAL_SERVICE_HPP
#define MANUAL_SERVICE_HPP

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

namespace noteahead {

class ThemeService;

//! Prepares the bundled user manual for display: cuts it into one themed document per heading and
//! lists those headings as a table of contents.
//!
//! The manual on disk carries no colors of its own, so the look follows the theme rather than
//! being frozen into the markup.
class ManualService : public QObject
{
    Q_OBJECT

    //! One entry per heading: { title, level, anchor }. Levels are 1..3, in document order.
    Q_PROPERTY(QVariantList tableOfContents READ tableOfContents NOTIFY tableOfContentsChanged)
    //! The manual cut into one themed document per heading: { title, level, anchor, html }, in
    //! document order and covering the whole manual. The dialog renders one item per section, which
    //! is what lets it scroll to a section by that item's position instead of by hunting for the
    //! heading's text in one enormous document.
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)

public:
    struct Heading
    {
        QString title;
        int level {};
        QString anchor;
    };

    struct Section
    {
        Heading heading;
        //! The heading and everything under it, up to the next heading.
        QString markup;
    };

    using HeadingList = std::vector<Heading>;
    using SectionList = std::vector<Section>;
    using ThemeServiceS = std::shared_ptr<ThemeService>;

    explicit ManualService(ThemeServiceS themeService, QObject * parent = nullptr);
    ~ManualService() override;

    //! Reads the manual from the given URL or path. Safe to call again to reload.
    Q_INVOKABLE void load(const QString & source);

    QVariantList tableOfContents() const;
    QVariantList sections() const;

    //! Headings in document order. Pure, so it can be exercised without touching the filesystem.
    static HeadingList parseHeadings(const QString & manual);

    //! The manual split at every heading, in document order. Anything before the first heading goes
    //! to the first section, so the sections concatenate back into the manual exactly.
    static SectionList parseSections(const QString & manual);

    //! The anchor a heading title resolves to. Lowercased, with runs of non-alphanumerics folded
    //! into single dashes.
    static QString anchorFor(const QString & title);

    //! The stylesheet prepended to the manual, built from the current theme.
    QString styleSheet() const;

signals:
    void tableOfContentsChanged();
    void sectionsChanged();
    //! Reported instead of throwing, so a missing manual degrades to a message in the dialog.
    void loadFailed(QString reason);

private:
    void rebuild();

    ThemeServiceS m_themeService;
    QString m_manual; //!< As read, before theming
    HeadingList m_headings;
    SectionList m_sections;
};

} // namespace noteahead

#endif // MANUAL_SERVICE_HPP
