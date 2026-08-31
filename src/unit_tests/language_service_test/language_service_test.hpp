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

#ifndef LANGUAGE_SERVICE_TEST_HPP
#define LANGUAGE_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class LanguageServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void test_availableLanguages_default_shouldStartWithSourceLanguage();
    void test_availableLanguages_default_shouldContainFinnish();
    void test_availableLanguages_supported_shouldAllHaveAnEmbeddedCatalogue();

    void test_nativeLanguageName_supported_shouldBeSpelledInThatLanguage();
    void test_nativeLanguageName_unknown_shouldFallBackToTheCode();

    void test_activeLanguage_beforeInitialization_shouldBeSourceLanguage();

    void test_setActiveLanguage_changed_shouldEmitSignalAndPersist();
    void test_setActiveLanguage_sameValue_shouldNotEmitSignal();

    void test_initializeTranslations_commandLineLanguage_shouldWinOverSavedLanguage();
    void test_initializeTranslations_savedLanguage_shouldBeUsedWhenNoCommandLineLanguage();
    void test_initializeTranslations_unknownLanguage_shouldFallBackToSourceLanguage();

    void test_setActiveLanguage_finnish_shouldMakeTranslationsResolve();
    void test_setActiveLanguage_backToSource_shouldRestoreSourceStrings();
};

} // namespace noteahead

#endif // LANGUAGE_SERVICE_TEST_HPP
