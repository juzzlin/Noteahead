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

#include "language_service_test.hpp"

#include "../../application/service/language_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../common/constants.hpp"

#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace noteahead {

namespace {

// Keeps the settings written by this test process out of the user scope shared by all processes.
// Without it, concurrent runs of this binary (parallel CI workspaces) race on the same file.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

std::shared_ptr<SettingsService> createSettingsService()
{
    return std::make_shared<SettingsService>();
}

} // namespace

void LanguageServiceTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("LanguageServiceTest");

    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void LanguageServiceTest::cleanup()
{
    // Each case starts from "the user has never picked a language".
    QSettings settings {};
    settings.clear();
    settings.sync();
}

void LanguageServiceTest::cleanupTestCase()
{
    QSettings settings {};
    settings.clear();
}

void LanguageServiceTest::test_availableLanguages_default_shouldStartWithSourceLanguage()
{
    LanguageService languageService { createSettingsService() };

    QVERIFY(!languageService.availableLanguages().isEmpty());
    QCOMPARE(languageService.availableLanguages().first(), Constants::Language::sourceLanguage());
}

void LanguageServiceTest::test_availableLanguages_default_shouldContainFinnish()
{
    LanguageService languageService { createSettingsService() };

    QVERIFY(languageService.availableLanguages().contains("fi"));
}

void LanguageServiceTest::test_availableLanguages_supported_shouldAllHaveAnEmbeddedCatalogue()
{
    // Guards the one mistake this wiring invites: adding a language to supportedLanguages() without
    // adding its .ts to the TS list in src/CMakeLists.txt. The selector would offer a language that
    // silently does nothing, which is invisible until someone picks it.
    for (auto && language : Constants::Language::supportedLanguages()) {
        const auto catalogue = Constants::Language::translationsResourceBase() + language + ".qm";
        QVERIFY2(QFile::exists(catalogue), qPrintable("Missing embedded catalogue: " + catalogue));
    }
}

void LanguageServiceTest::test_nativeLanguageName_supported_shouldBeSpelledInThatLanguage()
{
    LanguageService languageService { createSettingsService() };

    QCOMPARE(languageService.nativeLanguageName("fi"), QString::fromUtf8("Suomi"));
    QCOMPARE(languageService.nativeLanguageName("de"), QString::fromUtf8("Deutsch"));
    QCOMPARE(languageService.nativeLanguageName("pt_BR"), QString::fromUtf8("Português (Brasil)"));
}

void LanguageServiceTest::test_nativeLanguageName_unknown_shouldFallBackToTheCode()
{
    LanguageService languageService { createSettingsService() };

    QCOMPARE(languageService.nativeLanguageName("xx"), QString { "xx" });
}

void LanguageServiceTest::test_activeLanguage_beforeInitialization_shouldBeSourceLanguage()
{
    LanguageService languageService { createSettingsService() };

    QCOMPARE(languageService.activeLanguage(), Constants::Language::sourceLanguage());
}

void LanguageServiceTest::test_setActiveLanguage_changed_shouldEmitSignalAndPersist()
{
    const auto settingsService = createSettingsService();
    LanguageService languageService { settingsService };
    QSignalSpy spy { &languageService, &LanguageService::activeLanguageChanged };

    languageService.setActiveLanguage("fi");

    QCOMPARE(languageService.activeLanguage(), QString { "fi" });
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString { "fi" });
    QCOMPARE(settingsService->userLanguage(), QString { "fi" });
}

void LanguageServiceTest::test_setActiveLanguage_sameValue_shouldNotEmitSignal()
{
    LanguageService languageService { createSettingsService() };
    QSignalSpy spy { &languageService, &LanguageService::activeLanguageChanged };

    languageService.setActiveLanguage(Constants::Language::sourceLanguage());

    QCOMPARE(spy.count(), 0);
}

void LanguageServiceTest::test_initializeTranslations_commandLineLanguage_shouldWinOverSavedLanguage()
{
    const auto settingsService = createSettingsService();
    settingsService->setUserLanguage("de");
    LanguageService languageService { settingsService };
    languageService.setCommandLineLanguage("fi");

    languageService.initializeTranslations(*QCoreApplication::instance());

    QCOMPARE(languageService.activeLanguage(), QString { "fi" });
}

void LanguageServiceTest::test_initializeTranslations_savedLanguage_shouldBeUsedWhenNoCommandLineLanguage()
{
    const auto settingsService = createSettingsService();
    settingsService->setUserLanguage("de");
    LanguageService languageService { settingsService };

    languageService.initializeTranslations(*QCoreApplication::instance());

    QCOMPARE(languageService.activeLanguage(), QString { "de" });
}

void LanguageServiceTest::test_initializeTranslations_unknownLanguage_shouldFallBackToSourceLanguage()
{
    const auto settingsService = createSettingsService();
    LanguageService languageService { settingsService };
    languageService.setCommandLineLanguage("xx");

    languageService.initializeTranslations(*QCoreApplication::instance());

    QCOMPARE(languageService.activeLanguage(), Constants::Language::sourceLanguage());
}

void LanguageServiceTest::test_setActiveLanguage_finnish_shouldMakeTranslationsResolve()
{
    // Proves the whole runtime path in one go: the embedded catalogue is found, the translator is
    // installed on the application, and lookups actually resolve afterwards. A source string that
    // is translated in noteahead_fi.ts and short enough to stay stable is used as the probe.
    LanguageService languageService { createSettingsService() };
    const auto sourceText = "Cancel";
    QCOMPARE(QCoreApplication::translate("ConfirmationDialog", sourceText), QString { sourceText });

    languageService.setActiveLanguage("fi");

    QCOMPARE(QCoreApplication::translate("ConfirmationDialog", sourceText), QString::fromUtf8("Peruuta"));
}

void LanguageServiceTest::test_setActiveLanguage_backToSource_shouldRestoreSourceStrings()
{
    // Switching back has to uninstall the translator rather than leave the previous language in
    // place: English has no catalogue of its own, so nothing would otherwise undo the load.
    LanguageService languageService { createSettingsService() };
    const auto sourceText = "Cancel";
    languageService.setActiveLanguage("fi");
    QCOMPARE(QCoreApplication::translate("ConfirmationDialog", sourceText), QString::fromUtf8("Peruuta"));

    languageService.setActiveLanguage(Constants::Language::sourceLanguage());

    QCOMPARE(QCoreApplication::translate("ConfirmationDialog", sourceText), QString { sourceText });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::LanguageServiceTest)
