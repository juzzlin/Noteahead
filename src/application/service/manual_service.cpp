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

//! Heading text as the reader will see it, with entities resolved, which is what the table of
//! contents lists and what the anchor is derived from.
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

//! The rules the dialog and the web page share, in the given colors.
QString styleRules(const QString & text, const QString & accent, const QString & rule, const QString & codeBackground, const QString & footer)
{
    // Only the title, the section headings and the links take the accent. The manual runs to
    // thirty-odd sub-headings, and coloring those too turns the page into a wall of accent.
    return QString {
        "  body { font-size: 1.1em; line-height: 1.4; color: %1; }"
        "  h1 { color: %2; border-bottom: 2px solid %2; }"
        "  h2 { color: %2; border-bottom: 1px solid %3; margin-top: 20px; }"
        "  h3 { color: %1; }"
        // Text has no linkColor of its own, so cross references are colored here
        "  a { color: %2; }"
        "  li { margin-bottom: 8px; }"
        "  th { color: %1; }"
        "  td { border-bottom: 1px solid %3; }"
        "  code { font-family: monospace; }"
        "  .example { font-family: monospace; background-color: %4; padding: 10px; }"
        "  .footer { margin-top: 30px; font-size: 0.8em; color: %5; }"
    }
      .arg(text, accent, rule, codeBackground, footer);
}

std::optional<QString> readFile(const QString & source)
{
    QFile file { toLocalPath(source) };
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
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

ManualService::SectionList ManualService::parseSections(const QString & manual)
{
    SectionList sections;
    auto iterator = headingExpression().globalMatch(manual);
    qsizetype sectionStart = -1;
    Heading heading;
    while (iterator.hasNext()) {
        const auto match = iterator.next();
        const auto title = renderedText(match.captured(2));
        if (title.isEmpty()) {
            continue;
        }
        if (sectionStart >= 0) {
            sections.push_back({ heading, manual.mid(sectionStart, match.capturedStart() - sectionStart) });
        }
        // Whatever precedes the first heading belongs to the first section rather than to nothing,
        // so that the sections put back together are the manual again.
        sectionStart = sections.empty() ? 0 : match.capturedStart();
        heading = { title, match.captured(1).toInt(), anchorFor(title) };
    }
    if (sectionStart >= 0) {
        sections.push_back({ heading, manual.mid(sectionStart) });
    }
    return sections;
}

QString ManualService::styleSheet() const
{
    if (!m_themeService) {
        return {};
    }

    return "<style>"
      + styleRules(m_themeService->noteColumnTextColor().name(), m_themeService->accentColor().name(), m_themeService->manualRuleColor().name(),
                   m_themeService->manualCodeBackgroundColor().name(), m_themeService->manualFooterTextColor().name())
      + "</style>";
}

QString ManualService::webPage(const QString & manual, const QString & version)
{
    // The default theme, not whatever accent the one exporting has picked
    const ThemeService theme;
    const auto text = theme.noteColumnTextColor().name();
    const auto rule = theme.manualRuleColor().name();
    // As the dialog the manual is read in
    const QString background { "#1e1e1e" };

    QString body;
    QString contents;
    qsizetype position = 0;
    auto iterator = headingExpression().globalMatch(manual);
    while (iterator.hasNext()) {
        const auto match = iterator.next();
        const auto title = renderedText(match.captured(2));
        if (title.isEmpty()) {
            continue;
        }
        const auto level = match.captured(1);
        const auto anchor = anchorFor(title);
        body += manual.mid(position, match.capturedStart() - position);
        body += QString { "<h%1 id=\"%2\">%3</h%1>" }.arg(level, anchor, match.captured(2));
        position = match.capturedEnd();
        // The title is the page's own heading, so the contents start below it
        if (level != "1") {
            contents += QString { "<li class=\"level-%1\"><a href=\"#%2\">%3</a></li>\n" }.arg(level, anchor, title.toHtmlEscaped());
        }
    }
    body += manual.mid(position);

    const auto title = QString { "Noteahead %1 User Manual" }.arg(version).toHtmlEscaped();
    return QString {
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "<title>%1</title>\n"
        "<style>\n"
        "%2\n"
        "  html { background-color: %3; }"
        "  body { font-family: sans-serif; margin: 0; }"
        // The contents run to over a hundred entries, so they sit beside the manual and scroll on
        // their own rather than pushing it off the first screen
        "  .page { display: grid; grid-template-columns: 18em minmax(0, 1fr); gap: 2em; max-width: 80em; margin: 0 auto; padding: 0 2em; }"
        "  nav { position: sticky; top: 0; height: 100vh; overflow-y: auto; padding: 1em 0; font-size: 0.9em; border-right: 1px solid %4; }"
        "  nav a { text-decoration: none; }"
        "  nav ul { list-style: none; padding-left: 0; margin: 0; }"
        "  nav li { margin-bottom: 2px; }"
        "  nav .level-2 { margin-top: 0.6em; font-weight: bold; }"
        "  nav .level-3 { padding-left: 1em; }"
        "  main { padding: 1em 0 3em; }"
        "  code { background-color: %5; padding: 0 0.2em; }"
        "  @media (max-width: 50em) {"
        "    .page { display: block; padding: 0 1em; }"
        "    nav { position: static; height: auto; border-right: none; border-bottom: 1px solid %4; }"
        "  }"
        "\n</style>\n"
        "</head>\n"
        "<body>\n"
        "<div class=\"page\">\n"
        "<nav>\n<p class=\"footer\">Noteahead %6</p>\n<ul>\n%7</ul>\n</nav>\n"
        "<main>\n%8</main>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n"
    }
      .arg(title,
           styleRules(text, ThemeService::defaultAccentColor().name(), rule, theme.manualCodeBackgroundColor().name(), theme.manualFooterTextColor().name()),
           background, rule, theme.manualCodeBackgroundColor().name(), version.toHtmlEscaped(), contents, body);
}

std::optional<QString> ManualService::exportWebPage(const QString & source, const QString & target, const QString & version)
{
    const auto manual = readFile(source);
    if (!manual) {
        return QString { "Cannot read the manual from '%1'" }.arg(source);
    }

    QFile file { target };
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return QString { "Cannot write '%1': %2" }.arg(target, file.errorString());
    }
    if (file.write(webPage(*manual, version).toUtf8()) < 0) {
        return QString { "Cannot write '%1': %2" }.arg(target, file.errorString());
    }
    return {};
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
    m_sections = parseSections(m_manual);
    emit tableOfContentsChanged();
    emit sectionsChanged();
}

QVariantList ManualService::sections() const
{
    // Each section is a document of its own, so each carries the stylesheet.
    const auto style = styleSheet();
    QVariantList sections;
    for (auto && section : m_sections) {
        sections.append(QVariantMap {
          { "title", section.heading.title },
          { "level", section.heading.level },
          { "anchor", section.heading.anchor },
          { "html", style + section.markup } });
    }
    return sections;
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
