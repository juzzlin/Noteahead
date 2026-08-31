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

#ifndef LANGUAGE_SERVICE_HPP
#define LANGUAGE_SERVICE_HPP

#include <memory>

#include <QObject>
#include <QStringList>
#include <QTranslator>

class QCoreApplication;

namespace noteahead {

class SettingsService;

//! Owns the application's two translators and the choice of language.
//!
//! The source strings are English, so no en.qm exists and none is needed: uninstalling the
//! translators is what "English" means here.
class LanguageService : public QObject
{
    Q_OBJECT

    //! Qt locale name of the language in effect, e.g. "fi" or "pt_BR".
    Q_PROPERTY(QString activeLanguage READ activeLanguage WRITE setActiveLanguage NOTIFY activeLanguageChanged)
    //! Locale names of everything selectable, source language first.
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;

    explicit LanguageService(SettingsServiceS settingsService, QObject * parent = nullptr);

    ~LanguageService() override;

    QString activeLanguage() const;

    //! Installs the translators for the given language and remembers it as the user's choice.
    //! A language with no translations of its own still applies: it just resolves to the source
    //! strings, which is how the user gets back to English.
    Q_INVOKABLE void setActiveLanguage(QString language);

    QStringList availableLanguages() const;

    //! The language's own name for itself, for the selector.
    Q_INVOKABLE QString nativeLanguageName(const QString & language) const;

    //! Language requested via --lang, empty if the option was not given.
    QString commandLineLanguage() const;

    void setCommandLineLanguage(QString language);

    //! Resolves the language and installs the translators. Call once, before the QML engine loads,
    //! so that the first strings evaluated are already in the right language.
    void initializeTranslations(QCoreApplication & application);

signals:
    void activeLanguageChanged(QString language);

private:
    //! --lang beats the saved choice, which beats what the system asks for.
    QStringList languageCandidates() const;

    //! Qt's own translations, for the strings Qt itself supplies. Missing catalogues are not an
    //! error: the distribution's qt6-translations package may simply not be installed.
    void installQtTranslator(QCoreApplication & application, const QString & language);

    //! Returns false when the language has no embedded catalogue, which for the source language is
    //! the expected outcome rather than a failure.
    bool installApplicationTranslator(QCoreApplication & application, const QString & language);

    SettingsServiceS m_settingsService;

    QString m_activeLanguage;

    QString m_commandLineLanguage;

    QTranslator m_applicationTranslator;

    QTranslator m_qtTranslator;
};

} // namespace noteahead

#endif // LANGUAGE_SERVICE_HPP
