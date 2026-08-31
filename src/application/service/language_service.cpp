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

#include "language_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "settings_service.hpp"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>

namespace noteahead {

static const auto TAG = "LanguageService";

LanguageService::LanguageService(SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_settingsService { settingsService }
  , m_activeLanguage { Constants::Language::sourceLanguage() }
{
}

QString LanguageService::activeLanguage() const
{
    return m_activeLanguage;
}

QStringList LanguageService::availableLanguages() const
{
    QStringList languages { Constants::Language::sourceLanguage() };
    languages.append(Constants::Language::supportedLanguages());
    return languages;
}

QString LanguageService::nativeLanguageName(const QString & language) const
{
    return Constants::Language::nativeLanguageName(language);
}

QString LanguageService::commandLineLanguage() const
{
    return m_commandLineLanguage;
}

void LanguageService::setCommandLineLanguage(QString language)
{
    m_commandLineLanguage = language;
}

void LanguageService::setActiveLanguage(QString language)
{
    if (m_activeLanguage == language) {
        return;
    }

    juzzlin::L(TAG).info() << "Changing active language to '" << language.toStdString() << "'";

    auto & application = *QCoreApplication::instance();
    installQtTranslator(application, language);
    installApplicationTranslator(application, language);

    m_activeLanguage = language;
    m_settingsService->setUserLanguage(language);

    emit activeLanguageChanged(m_activeLanguage);
}

QStringList LanguageService::languageCandidates() const
{
    if (!m_commandLineLanguage.isEmpty()) {
        juzzlin::L(TAG).info() << "Using language given on the command line: '" << m_commandLineLanguage.toStdString() << "'";
        return { m_commandLineLanguage };
    }

    if (const auto savedLanguage = m_settingsService->userLanguage(); !savedLanguage.isEmpty()) {
        juzzlin::L(TAG).info() << "Using previously selected language: '" << savedLanguage.toStdString() << "'";
        return { savedLanguage };
    }

    // Qt hands these out as "fi-FI" while the catalogues are named "fi" and "pt_BR", so both the
    // separator and the bare language have to be tried before giving up on a system preference.
    QStringList candidates;
    for (auto && uiLanguage : QLocale {}.uiLanguages()) {
        const auto normalized = QString { uiLanguage }.replace('-', '_');
        candidates << normalized;
        if (const auto bare = normalized.section('_', 0, 0); bare != normalized) {
            candidates << bare;
        }
    }
    candidates.removeDuplicates();
    return candidates;
}

void LanguageService::initializeTranslations(QCoreApplication & application)
{
    for (auto && language : languageCandidates()) {
        installQtTranslator(application, language);
        if (installApplicationTranslator(application, language)) {
            m_activeLanguage = language;
            juzzlin::L(TAG).info() << "Active language is '" << language.toStdString() << "'";
            return;
        }
    }

    // Nothing matched, so the source strings stand. Not a failure: it is exactly what running in
    // English looks like.
    m_activeLanguage = Constants::Language::sourceLanguage();
    juzzlin::L(TAG).info() << "No translations loaded, falling back to '" << m_activeLanguage.toStdString() << "'";
}

void LanguageService::installQtTranslator(QCoreApplication & application, const QString & language)
{
    application.removeTranslator(&m_qtTranslator);

    const auto translationsPath = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (m_qtTranslator.load("qtbase_" + language, translationsPath)) {
        application.installTranslator(&m_qtTranslator);
        juzzlin::L(TAG).info() << "Loaded Qt translations for '" << language.toStdString() << "'";
    } else {
        juzzlin::L(TAG).debug() << "No Qt translations for '" << language.toStdString() << "' under " << translationsPath.toStdString();
    }
}

bool LanguageService::installApplicationTranslator(QCoreApplication & application, const QString & language)
{
    application.removeTranslator(&m_applicationTranslator);

    if (language == Constants::Language::sourceLanguage()) {
        return true;
    }

    if (m_applicationTranslator.load(Constants::Language::translationsResourceBase() + language)) {
        application.installTranslator(&m_applicationTranslator);
        juzzlin::L(TAG).info() << "Loaded application translations for '" << language.toStdString() << "'";
        return true;
    }

    juzzlin::L(TAG).debug() << "No application translations for '" << language.toStdString() << "'";
    return false;
}

LanguageService::~LanguageService() = default;

} // namespace noteahead
