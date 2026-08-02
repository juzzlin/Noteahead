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

#include "manual_service.hpp"

#include "theme_service.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QTextDocumentFragment>
#include <QUrl>
#include <QVariantMap>

namespace noteahead {

namespace {

//! Matches a heading and captures its level and its text. The manual keeps headings free of
//! nested markup, so the text capture stays simple on purpose.
QRegularExpression headingExpression()
{
    return QRegularExpression { "<h([1-3])>(.*?)</h[1-3]>", QRegularExpression::DotMatchesEverythingOption };
}

//! Heading text as the reader will see it, with entities resolved.
//!
//! The dialog locates a section by searching the rendered document for its title, so a title
//! holding "&amp;" has to be compared as "&" or that section becomes unreachable.
QString renderedText(const QString & markup)
{
    return markup.contains('&') ? QTextDocumentFragment::fromHtml(markup).toPlainText().simplified() : markup.simplified();
}

QString toLocalPath(const QString & source)
{
    if (const QUrl url { source }; url.isValid() && !url.scheme().isEmpty()) {
        return url.scheme() == "qrc" ? ":" + url.path() : url.toLocalFile();
    }
    return source;
}

} // namespace

ManualService::ManualService(ThemeServiceS themeService, QObject * parent)
  : QObject { parent }
  , m_themeService { themeService }
{
    if (m_themeService) {
        // The manual carries no colors of its own, so a new accent has to be re-applied here
        connect(m_themeService.get(), &ThemeService::accentColorChanged, this, &ManualService::rebuild);
    }
}

ManualService::~ManualService() = default;

QString ManualService::anchorFor(const QString & title)
{
    static const QRegularExpression nonAlphanumeric { "[^a-z0-9]+" };
    return title.toLower().replace(nonAlphanumeric, "-").replace(QRegularExpression { "^-|-$" }, "");
}

ManualService::HeadingList ManualService::parseHeadings(const QString & manual)
{
    HeadingList headings;
    auto iterator = headingExpression().globalMatch(manual);
    while (iterator.hasNext()) {
        const auto match = iterator.next();
        if (const auto title = renderedText(match.captured(2)); !title.isEmpty()) {
            headings.push_back({ title, match.captured(1).toInt(), anchorFor(title) });
        }
    }
    return headings;
}

QString ManualService::injectAnchors(const QString & manual)
{
    QString anchored;
    anchored.reserve(manual.size());
    qsizetype copiedUpTo = 0;
    auto iterator = headingExpression().globalMatch(manual);
    while (iterator.hasNext()) {
        const auto match = iterator.next();
        const auto title = renderedText(match.captured(2));
        if (title.isEmpty()) {
            continue;
        }
        const auto level = match.captured(1);
        anchored += manual.mid(copiedUpTo, match.capturedStart() - copiedUpTo);
        anchored += QString { "<h%1><a name=\"%2\"></a>%3</h%1>" }.arg(level, anchorFor(title), match.captured(2));
        copiedUpTo = match.capturedEnd();
    }
    anchored += manual.mid(copiedUpTo);
    return anchored;
}

QString ManualService::styleSheet() const
{
    if (!m_themeService) {
        return {};
    }

    const auto accent = m_themeService->accentColor().name();
    const auto rule = m_themeService->manualRuleColor().name();
    const auto text = m_themeService->noteColumnTextColor().name();
    const auto codeBackground = m_themeService->manualCodeBackgroundColor().name();
    const auto footer = m_themeService->manualFooterTextColor().name();

    // Only the title, the section headings and the links take the accent. The manual runs to
    // thirty-odd sub-headings, and coloring those too turns the page into a wall of accent.
    return QString {
        "<style>"
        "  body { font-size: 1.1em; line-height: 1.4; color: %1; }"
        "  h1 { color: %2; border-bottom: 2px solid %2; }"
        "  h2 { color: %2; border-bottom: 1px solid %3; margin-top: 20px; }"
        "  h3 { color: %1; }"
        // TextEdit has no linkColor of its own, so cross references are colored here
        "  a { color: %2; }"
        "  li { margin-bottom: 8px; }"
        "  th { color: %1; }"
        "  td { border-bottom: 1px solid %3; }"
        "  code { font-family: monospace; }"
        "  .example { font-family: monospace; background-color: %4; padding: 10px; }"
        "  .footer { margin-top: 30px; font-size: 0.8em; color: %5; }"
        "</style>"
    }
      .arg(text, accent, rule, codeBackground, footer);
}

void ManualService::load(const QString & source)
{
    QFile file { toLocalPath(source) };
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_manual.clear();
        rebuild();
        emit loadFailed(file.errorString());
        return;
    }

    m_manual = QString::fromUtf8(file.readAll());
    rebuild();
}

void ManualService::rebuild()
{
    m_headings = parseHeadings(m_manual);
    m_html = m_manual.isEmpty() ? QString {} : styleSheet() + injectAnchors(m_manual);
    emit htmlChanged();
    emit tableOfContentsChanged();
}

QString ManualService::html() const
{
    return m_html;
}

QVariantList ManualService::tableOfContents() const
{
    QVariantList contents;
    for (auto && heading : m_headings) {
        contents.append(QVariantMap {
          { "title", heading.title },
          { "level", heading.level },
          { "anchor", heading.anchor } });
    }
    return contents;
}

} // namespace noteahead
