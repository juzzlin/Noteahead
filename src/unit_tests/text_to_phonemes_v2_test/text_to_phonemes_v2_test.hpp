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

#ifndef TEXT_TO_PHONEMES_V2_TEST_HPP
#define TEXT_TO_PHONEMES_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class TextToPhonemesV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_lexicon_shouldBeWellFormed();
    void test_preFortisClipping_shouldShortenVowelsBeforeVoicelessCodas();
    void test_allophone_flap_shouldTriggerBeforeUnstressedVowelOnly();
    void test_allophone_the_shouldAdaptToFollowingVowel();
    void test_numbers_shouldExpandNaturally();
    void test_contractions_shouldPronounceNaturally();
    void test_exceptionLexicon_shouldFixIrregularWords();
};

} // namespace noteahead

#endif // TEXT_TO_PHONEMES_V2_TEST_HPP
