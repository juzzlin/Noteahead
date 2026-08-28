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

#include "text_to_phonemes.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace noteahead {

namespace {

//! One letter-to-sound rule: in the context of @c left and @c right, the letters @c focus are
//! spoken as @c phonemes.
//!
//! Contexts are written in reading order and may use these classes, which is the notation the
//! published rule sets use and the reason the table below is legible at all:
//!
//!   ' '  a word boundary
//!   '#'  one or more vowel letters
//!   '^'  one consonant letter
//!   ':'  zero or more consonant letters
//!   '+'  a front vowel: E, I or Y
//!   '.'  a voiced consonant
//!   '&'  a sibilant
//!   '@'  a consonant after which long U is spoken as in "rule" rather than as in "mule"
//!   '%'  a suffix: E, ER, ES, ED, ING or ELY
struct LetterRule
{
    std::string_view left;
    std::string_view focus;
    std::string_view right;
    std::string_view phonemes;
};

struct LexiconEntry
{
    std::string_view word;
    std::string_view phonemes;
};

//! A word as written, with the syllable the user marked as stressed if they marked one.
struct Spelling
{
    std::string word;
    std::optional<size_t> stress;
};

//! The words English says weakly.
//!
//! A function word is not merely a word the rules could already read -- it is a word that is *not*
//! pronounced the way its spelling suggests, because English reduces it. "The" is not /ðiː/ in
//! running speech, it is /ðə/; "of" is not /ɒv/ but /əv/; "to" is /tə/. Reading them at full value
//! gives every word in a sentence the same weight, and a stress-timed language read with even
//! stress is exactly what "reading a list of syllables" sounds like.
//!
//! They double as a small exception dictionary, which is why a few here are not reductions at all:
//! "one" and "two" have spellings no rule set is going to get right.
constexpr LexiconEntry FunctionWords[] {
    { "A", "AX" },
    { "AN", "AX N" },
    { "AND", "AX N D" },
    { "ARE", "ER" },
    { "AS", "AX Z" },
    { "AT", "AX T" },
    { "BE", "B IY" },
    { "BEEN", "B IH N" },
    { "BUT", "B AX T" },
    { "BY", "B AY" },
    { "CAN", "K AX N" },
    { "DO", "D UW" },
    { "FOR", "F ER" },
    { "FROM", "F R AX M" },
    { "HAD", "HH AX D" },
    { "HAS", "HH AX Z" },
    { "HAVE", "HH AX V" },
    { "HE", "HH IY" },
    { "HER", "HH ER" },
    { "HIS", "HH IH Z" },
    { "IN", "IH N" },
    { "IS", "IH Z" },
    { "IT", "IH T" },
    { "ITS", "IH T S" },
    { "OF", "AX V" },
    { "ON", "AA N" },
    { "OR", "AO R" },
    { "SHE", "SH IY" },
    { "THAN", "DH AX N" },
    { "THAT", "DH AX T" },
    { "THE", "DH AX" },
    { "THEM", "DH AX M" },
    { "THERE", "DH EH R" },
    { "THEY", "DH EY" },
    { "THIS", "DH IH S" },
    { "TO", "T AX" },
    { "WAS", "W AX Z" },
    { "WE", "W IY" },
    { "WERE", "W ER" },
    { "WILL", "W IH L" },
    { "WITH", "W IH DH" },
    { "WOULD", "W UH D" },
    { "YOU", "Y UW" },
    { "YOUR", "Y AO R" },
    { "ONE", "W AH N" },
    { "TWO", "T UW" },
};

//! How much shorter an unstressed function word runs than its natural length.
constexpr double FunctionWordLength = 0.75;

//! What stress does to a syllable's length. English is stress-timed: the strong syllable is long and
//! the weak ones are short, and that alternation is the rhythm. Giving every syllable the same
//! length is what makes a sentence sound like a list of syllables read out one by one.
constexpr double StressedLength = 1.3;
constexpr double UnstressedLength = 0.8;

//! How much of its length an unvoiced stop keeps when nothing is going to be aspirated after it.
constexpr double UnaspiratedStopLength = 0.55;

//! How much longer the last syllable before a pause runs. One of the strongest cues that a phrase
//! has finished: without it a sentence stops rather than ends.
constexpr double PhraseFinalLength = 1.45;

bool isVowelLetter(char c)
{
    return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'Y';
}

bool isLetter(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool isConsonantLetter(char c)
{
    return isLetter(c) && !isVowelLetter(c);
}

bool isFrontVowel(char c)
{
    return c == 'E' || c == 'I' || c == 'Y';
}

bool isVoicedConsonant(char c)
{
    return std::string_view { "BDVGJLMNRWZ" }.find(c) != std::string_view::npos;
}

bool isSibilant(char c)
{
    return std::string_view { "SCGZXJ" }.find(c) != std::string_view::npos;
}

bool isLongUConsonant(char c)
{
    return std::string_view { "TSRDLZNJ" }.find(c) != std::string_view::npos;
}

//! Length of the apostrophe at @p pos, or zero if there is not one.
//!
//! Typed text carries the curly one as often as the straight one, and left unhandled it is not
//! merely ignored: it ends the word, so "don't" is read as "don" followed by "t".
size_t apostropheLength(std::string_view text, size_t pos)
{
    if (text[pos] == '\'') {
        return 1;
    }
    // U+2019 RIGHT SINGLE QUOTATION MARK.
    if (text.substr(pos, 3) == "\xE2\x80\x99") {
        return 3;
    }
    return 0;
}

bool isApostrophe(std::string_view text, size_t pos)
{
    return apostropheLength(text, pos) > 0;
}

//! Whether the apostrophe at @p pos in @p word marks a contraction rather than stress.
//!
//! Contraction endings are a closed set and always finish the word, so what follows tells the two
//! uses apart: "don\'t" has a T and nothing more, while "A\'merica" has a whole syllable. The one
//! word this reads as stress rather than contraction is "o\'clock", which is stressed on "clock"
//! anyway.
bool isContraction(std::string_view word, size_t pos)
{
    const auto rest = word.substr(pos + 1);
    for (auto && ending : { "T", "S", "D", "M", "LL", "RE", "VE" }) {
        if (rest == ending) {
            return true;
        }
    }
    return rest.empty();
}

//! Whether a suffix ('%') starts at @p pos. Longest first, so that ELY is not read as E.
size_t suffixLength(std::string_view text, size_t pos)
{
    for (auto && suffix : { "ELY", "ING", "ED", "ER", "ES", "E" }) {
        const std::string_view candidate { suffix };
        if (text.substr(pos, candidate.size()) == candidate) {
            return candidate.size();
        }
    }
    return 0;
}

//! Matches @p pattern forwards from @p pos. The text is padded with spaces either side, so a word
//! boundary is just a space and needs no special case for the ends.
bool matchRight(std::string_view text, size_t pos, std::string_view pattern)
{
    size_t p = pos;
    for (auto && token : pattern) {
        switch (token) {
        case '#':
            if (p >= text.size() || !isVowelLetter(text[p])) {
                return false;
            }
            while (p < text.size() && isVowelLetter(text[p])) {
                p++;
            }
            break;
        case ':':
            while (p < text.size() && isConsonantLetter(text[p])) {
                p++;
            }
            break;
        case '^':
            if (p >= text.size() || !isConsonantLetter(text[p])) {
                return false;
            }
            p++;
            break;
        case '+':
            if (p >= text.size() || !isFrontVowel(text[p])) {
                return false;
            }
            p++;
            break;
        case '.':
            if (p >= text.size() || !isVoicedConsonant(text[p])) {
                return false;
            }
            p++;
            break;
        case '&':
            if (p >= text.size() || !isSibilant(text[p])) {
                return false;
            }
            p++;
            break;
        case '@':
            if (p >= text.size() || !isLongUConsonant(text[p])) {
                return false;
            }
            p++;
            break;
        case '%': {
            const size_t length = suffixLength(text, p);
            if (!length) {
                return false;
            }
            p += length;
            break;
        }
        default:
            if (p >= text.size() || text[p] != token) {
                return false;
            }
            p++;
            break;
        }
    }
    return true;
}

//! Matches @p pattern backwards from @p pos, which is one past the last character to consider.
//! Written in reading order like the right contexts, so it is walked from its own end.
bool matchLeft(std::string_view text, size_t pos, std::string_view pattern)
{
    size_t p = pos;
    for (size_t i = pattern.size(); i-- > 0;) {
        switch (const char token = pattern[i]; token) {
        case '#':
            if (!p || !isVowelLetter(text[p - 1])) {
                return false;
            }
            while (p && isVowelLetter(text[p - 1])) {
                p--;
            }
            break;
        case ':':
            while (p && isConsonantLetter(text[p - 1])) {
                p--;
            }
            break;
        case '^':
            if (!p || !isConsonantLetter(text[p - 1])) {
                return false;
            }
            p--;
            break;
        case '+':
            if (!p || !isFrontVowel(text[p - 1])) {
                return false;
            }
            p--;
            break;
        case '.':
            if (!p || !isVoicedConsonant(text[p - 1])) {
                return false;
            }
            p--;
            break;
        case '&':
            if (!p || !isSibilant(text[p - 1])) {
                return false;
            }
            p--;
            break;
        case '@':
            if (!p || !isLongUConsonant(text[p - 1])) {
                return false;
            }
            p--;
            break;
        default:
            if (!p || text[p - 1] != token) {
                return false;
            }
            p--;
            break;
        }
    }
    return true;
}

//! The rule set. Around 250 rules, in the tradition of the Naval Research Laboratory set: enough to
//! read ordinary English prose, not enough to read a phone book. Where it is wrong, the /.../ escape
//! is the fix, and it is wrong per word rather than per spelling.
constexpr LetterRule Rules[] {

    // Rules are tried in order and the first that matches wins, so within a letter they run from the
    // most specific context to the bare fallback. Every letter ends with a rule whose contexts are
    // empty, which is what guarantees the walk always advances.
    { " ", "A", " ", "AH" },
    { " ", "ARE", " ", "AA R" },
    { " ", "AR", "O", "AH R" },
    { "", "AR", "#", "EH R" },
    { " ^", "AS", "#", "EY S" },
    { "", "A", "WA", "AH" },
    { "", "AW", "", "AO" },
    { " :", "ANY", "", "EH N IY" },
    { "", "A", "^+#", "EY" },
    { "#:", "ALLY", "", "AH L IY" },
    { " ", "AL", "#", "AH L" },
    { "", "AGAIN", "", "AH G EH N" },
    { "#:", "AG", "E", "IH JH" },
    { "", "A", "^+:#", "AE" },
    { " :", "A", "^+ ", "EY" },
    { "", "A", "^%", "EY" },
    { " ", "ARR", "", "AH R" },
    { "", "ARR", "", "AE R" },
    { " ^", "AR", " ", "AA R" },
    { "", "AR", "", "AA R" },
    { "", "AIR", "", "EH R" },
    { "", "AI", "", "EY" },
    { "", "AY", "", "EY" },
    // "caught" and "daughter" are AO, but "laugh" and "laughter" are AE F -- and "slaughter" is AO
    // again, so the split is word-initial L rather than L.
    { " L", "AUGH", "", "AE F" },
    { "", "AUGH", "T", "AO" },
    { "", "AUGH", "", "AE F" },
    { "", "AU", "", "AO" },
    { "#:", "ALS", " ", "AH L Z" },
    { "#:", "AL", " ", "AH L" },
    { "", "ALK", "", "AO K" },
    { "", "AL", "^", "AO L" },
    { " :", "ABLE", "", "EY B AH L" },
    { "", "ABLE", "", "AH B AH L" },
    { "", "ANG", "+", "EY N JH" },
    { "#:^", "A", " ", "AH" },
    { "", "A", "", "AE" },

    { " ", "BE", "^#", "B IH" },
    { "", "BEING", "", "B IY IH NG" },
    { " ", "BOTH", " ", "B OW TH" },
    { " ", "BUS", "#", "B IH Z" },
    { "", "BUIL", "", "B IH L" },
    { "", "B", "", "B" },

    { " ", "CH", "^", "K" },
    { "^E", "CH", "", "K" },
    { "", "CH", "", "CH" },
    { " S", "CI", "#", "S AY" },
    { "", "CI", "A", "SH" },
    { "", "CI", "O", "SH" },
    { "", "CI", "EN", "SH" },
    { "", "C", "+", "S" },
    { "", "CK", "", "K" },
    { "", "COM", "%", "K AH M" },
    { "", "C", "", "K" },

    { "#:", "DED", " ", "D IH D" },
    { ".E", "D", " ", "D" },
    { "#:^E", "D", " ", "T" },
    { " ", "DE", "^#", "D IH" },
    { " ", "DO", " ", "D UW" },
    { " ", "DOES", "", "D AH Z" },
    { " ", "DOING", "", "D UW IH NG" },
    { " ", "DOW", "", "D AW" },
    { "", "DU", "A", "JH UW" },
    { "", "D", "", "D" },

    { "#:", "E", " ", "" },
    { "':^", "E", " ", "" },
    { " :", "E", " ", "IY" },
    { "#", "ED", " ", "D" },
    { "#:", "E", "D ", "" },
    { "", "EV", "ER", "EH V" },
    { "", "E", "^%", "IY" },
    { "", "ERI", "#", "IY R IY" },
    { "", "ERI", "", "EH R IH" },
    { "#:", "ER", "#", "ER" },
    { "", "ER", "#", "EH R" },
    { "", "ER", "", "ER" },
    { " ", "EVEN", "", "IY V EH N" },
    { "#:", "E", "W", "" },
    { "@", "EW", "", "UW" },
    { "", "EW", "", "Y UW" },
    { "", "E", "O", "IY" },
    { "#:&", "ES", " ", "IH Z" },
    { "#:", "E", "S ", "" },
    { "#:", "ELY", " ", "L IY" },
    { "#:", "EMENT", "", "M EH N T" },
    { "", "EFUL", "", "F UH L" },
    { "", "EE", "", "IY" },
    { "", "EARN", "", "ER N" },
    { " ", "EAR", "^", "ER" },
    { "", "EAD", "", "EH D" },
    { "#:", "EA", " ", "IY AH" },
    { "", "EAU", "", "Y UW" },
    { "", "EA", "SU", "EH" },
    { "", "EA", "", "IY" },
    { "", "EIGH", "", "EY" },
    { "", "EI", "", "IY" },
    { " ", "EYE", "", "AY" },
    { "", "EY", "", "IY" },
    { "", "EU", "", "Y UW" },
    { "", "E", "", "EH" },

    { "", "FUL", "", "F UH L" },
    { "", "F", "", "F" },

    { "", "GIV", "", "G IH V" },
    { " ", "G", "I^", "G" },
    { "", "GE", "T", "G EH" },
    { "SU", "GGES", "", "G JH EH S" },
    { "", "GG", "", "G" },
    { " B#", "G", "", "G" },
    { "", "G", "+", "JH" },
    { "", "GREAT", "", "G R EY T" },
    { "#", "GH", "", "" },
    { "", "G", "", "G" },

    { " ", "HAV", "", "HH AE V" },
    { " ", "HERE", "", "HH IY R" },
    { " ", "HOUR", "", "AW ER" },
    { "", "HOW", "", "HH AW" },
    { " R", "H", "", "" },
    { "", "H", "#", "HH" },
    { "", "H", "", "" },

    { " ", "IN", "", "IH N" },
    { " ", "I", " ", "AY" },
    { "", "IN", "D", "AY N" },
    { "", "IER", "", "IY ER" },
    { "#:R", "IED", " ", "IY D" },
    { "", "IED", " ", "AY D" },
    { "", "IEN", "", "IY EH N" },
    { "", "IE", "T", "AY EH" },
    { " :", "I", "%", "AY" },
    { "", "I", "%", "IY" },
    { "", "IE", "", "IY" },
    { "", "I", "^+:#", "IH" },
    { "", "IR", "#", "AY R" },
    { "", "IZ", "%", "AY Z" },
    { "", "IS", "%", "AY Z" },
    { "", "I", "D%", "AY" },
    { "+^", "I", "^+", "IH" },
    { "", "I", "T%", "AY" },
    { "#:^", "I", "^+", "IH" },
    { "", "I", "^Y ", "IH" },
    { "", "I", "^+", "AY" },
    { "", "IR", "", "ER" },
    { "", "IGH", "", "AY" },
    { "", "ILDR", "", "IH L D R" },
    { "", "ILD", "", "AY L D" },
    { "", "IGN", " ", "AY N" },
    { "", "IGN", "^", "AY N" },
    { "", "IQUE", "", "IY K" },
    { "", "I", "", "IH" },

    { "", "J", "", "JH" },

    { " ", "K", "N", "" },
    { "", "K", "", "K" },

    { "", "LO", "C#", "L OW" },
    { "L", "L", "", "" },
    { "#:^", "L", "%", "AH L" },
    { "", "LEAD", "", "L IY D" },
    { "", "L", "", "L" },

    { "", "MOV", "", "M UW V" },
    { "", "M", "", "M" },

    { "E", "NG", "+", "N JH" },
    { "", "NG", "R", "NG G" },
    { "", "NGL", "%", "NG G AH L" },
    { "", "NG", "", "NG" },
    { "", "NK", "", "NG K" },
    { " ", "NOW", " ", "N AW" },
    { "", "N", "", "N" },

    { "", "OF", " ", "AH V" },
    { "", "OROUGH", "", "ER OW" },
    { "#:", "OR", " ", "ER" },
    { "#:", "ORS", " ", "ER Z" },
    { "", "OR", "", "AO R" },
    { " ", "ONE", "", "W AH N" },
    { "", "OW", "", "OW" },
    { " ", "OVER", "", "OW V ER" },
    { "", "OV", "", "AH V" },
    { "", "O", "^%", "OW" },
    { "", "O", "^EN", "OW" },
    { "", "O", "^I#", "OW" },
    { "", "OL", "D", "OW L" },
    { "", "OUGHT", "", "AO T" },
    { "", "OUGH", "", "AH F" },
    { " ", "OU", "", "AW" },
    { "H", "OU", "S#", "AW" },
    { "", "OUS", "", "AH S" },
    { "", "OUR", "", "AO R" },
    { "", "OULD", "", "UH D" },
    { "^", "OU", "^L", "AH" },
    { "", "OUP", "", "UW P" },
    { "", "OU", "", "AW" },
    { "", "OY", "", "OY" },
    { "", "OING", "", "OW IH NG" },
    { "", "OI", "", "OY" },
    { "", "OOR", "", "AO R" },
    { "", "OOK", "", "UH K" },
    { "", "OOD", "", "UH D" },
    { "", "OO", "", "UW" },
    { "", "O", "E", "OW" },
    { "", "O", " ", "OW" },
    { "", "OA", "", "OW" },
    { " ", "ONLY", "", "OW N L IY" },
    { " ", "ONCE", "", "W AH N S" },
    { "", "ON'T", "", "OW N T" },
    { "C", "O", "N", "AA" },
    { "", "O", "NG", "AO" },
    { " :^", "O", "N", "AH" },
    { "I", "ON", "", "AH N" },
    { "#:", "ON", " ", "AH N" },
    { "#^", "ON", "", "AH N" },
    { "", "O", "ST ", "OW" },
    { "", "OF", "^", "AO F" },
    { "", "OTHER", "", "AH DH ER" },
    { "", "OSS", " ", "AO S" },
    { "#:^", "OM", "", "AH M" },
    { "", "O", "", "AA" },

    { "", "PH", "", "F" },
    { "", "PEOP", "", "P IY P" },
    { "", "POW", "", "P AW" },
    { "", "PUT", " ", "P UH T" },
    { "", "P", "", "P" },

    { "", "QUAR", "", "K W AO R" },
    { "", "QU", "", "K W" },
    { "", "Q", "", "K" },

    { " ", "RE", "^#", "R IY" },
    { "", "R", "", "R" },

    { "", "SH", "", "SH" },
    { "#", "SION", "", "ZH AH N" },
    { "", "SOME", "", "S AH M" },
    { "#", "SUR", "#", "ZH ER" },
    { "", "SUR", "#", "SH ER" },
    { "#", "SU", "#", "ZH UW" },
    { "#", "SSU", "#", "SH UW" },
    { "#", "SED", " ", "Z D" },
    { "#", "S", "#", "Z" },
    { "", "SAID", "", "S EH D" },
    { "^", "SION", "", "SH AH N" },
    { "", "S", "S", "" },
    { ".", "S", " ", "Z" },
    { "#:.E", "S", " ", "Z" },
    { "#:^##", "S", " ", "Z" },
    { "#:^#", "S", " ", "S" },
    { "U", "S", " ", "S" },
    { " :#", "S", " ", "Z" },
    { " ", "SCH", "", "S K" },
    { "", "S", "C+", "" },
    { "#", "SM", "", "Z M" },
    { "#", "SN", "'", "Z AH N" },
    { "", "S", "", "S" },

    { " ", "THE", " ", "DH AH" },
    { "", "TO", " ", "T UW" },
    { " ", "THAT", "", "DH AE T" },
    { " ", "THIS", " ", "DH IH S" },
    { " ", "THEY", "", "DH EY" },
    { " ", "THERE", "", "DH EH R" },
    { "", "THER", "", "DH ER" },
    { "", "THEIR", "", "DH EH R" },
    { " ", "THAN", " ", "DH AE N" },
    { " ", "THEM", " ", "DH EH M" },
    { "", "THESE", " ", "DH IY Z" },
    { " ", "THEN", "", "DH EH N" },
    { "", "THROUGH", "", "TH R UW" },
    { "", "THOSE", "", "DH OW Z" },
    { "", "THOUGH", " ", "DH OW" },
    { " ", "THUS", "", "DH AH S" },
    { "", "TH", "", "TH" },
    { "#:", "TED", " ", "T IH D" },
    { "S", "TI", "#N", "CH" },
    { "", "TI", "O", "SH" },
    { "", "TI", "A", "SH" },
    { "", "TIEN", "", "SH AH N" },
    { "", "TUR", "#", "CH ER" },
    { "", "TU", "A", "CH UW" },
    { " ", "TWO", "", "T UW" },
    { "", "T", "", "T" },

    { " ", "UN", "I", "Y UW N" },
    { " ", "UN", "", "AH N" },
    { " ", "UPON", "", "AH P AO N" },
    { "@", "UR", "#", "UH R" },
    { "", "UR", "#", "Y UH R" },
    { "", "UR", "", "ER" },
    { "", "U", "^ ", "AH" },
    { "", "U", "^^", "AH" },
    { "", "UY", "", "AY" },
    { " G", "U", "#", "" },
    { "G", "U", "%", "" },
    { "G", "U", "#", "W" },
    { "#N", "U", "", "Y UW" },
    { "@", "U", "", "UW" },
    { "", "U", "", "Y UW" },

    { "", "VIEW", "", "V Y UW" },
    { "", "V", "", "V" },

    { " ", "WERE", "", "W ER" },
    { "", "WA", "S", "W AA" },
    { "", "WA", "T", "W AA" },
    { "", "WHERE", "", "W EH R" },
    { "", "WHAT", "", "W AH T" },
    { "", "WHOL", "", "HH OW L" },
    { "", "WHO", "", "HH UW" },
    { "", "WH", "", "W" },
    { "", "WAR", "", "W AO R" },
    { "", "WOR", "^", "W ER" },
    { "", "WR", "", "R" },
    { "", "W", "", "W" },

    { " ", "X", "", "Z" },
    { "", "X", "", "K S" },

    { "", "YOUNG", "", "Y AH NG" },
    { " ", "YOU", "", "Y UW" },
    { " ", "YES", "", "Y EH S" },
    { " ", "Y", "", "Y" },
    { "#:^", "Y", " ", "IY" },
    { "#:^", "Y", "I", "IY" },
    { " :", "Y", " ", "AY" },
    { " :", "Y", "#", "AY" },
    { " :", "Y", "^+:#", "IH" },
    { " :", "Y", "^#", "AY" },
    { "", "Y", "", "IH" },

    { "", "Z", "", "Z" },
};

//! Splits a phoneme string like "HH EH L OW" into table entries, appending them to @p events.
//!
//! The affricates are written CH and JH in the rules because that is what they are called, but the
//! synthesizer has no affricate: an affricate *is* a stop released into a fricative, so it is spelled
//! here as the two phonemes it consists of and the voice renders it correctly without a special case.
void appendPhonemes(std::string_view names, PhonemeEventList & events, bool wordStart)
{
    size_t pos = 0;
    while (pos < names.size()) {
        const size_t end = std::min(names.find(' ', pos), names.size());
        const auto name = names.substr(pos, end - pos);
        pos = end + 1;
        if (name.empty()) {
            continue;
        }

        const auto append = [&](std::string_view resolved) {
            if (const auto * spec = speechPhoneme(resolved); spec) {
                events.push_back({ spec, false, wordStart && events.empty() });
            }
        };

        if (name == "CH") {
            append("T");
            append("SH");
        } else if (name == "JH") {
            append("D");
            append("ZH");
        } else {
            append(name);
        }
    }
}

//! Runs one word, already uppercased and padded with a space either side, through the rules.
//! Marks the stressed syllable of the word occupying events from @p firstEvent onwards, and gives
//! the syllables the lengths that stress implies.
void markWordStress(PhonemeEventList & events, size_t firstEvent, size_t lastEvent, std::string_view word, std::optional<size_t> explicitStress)
{
    size_t syllables = 0;
    for (size_t i = firstEvent; i < lastEvent; i++) {
        if (events[i].syllableStart) {
            syllables++;
        }
    }
    if (!syllables) {
        return;
    }

    // A mark in the phrase wins: the rules are right four times in five, and this is how the user
    // settles the fifth.
    const size_t stressed = explicitStress ? std::min(*explicitStress, syllables - 1) : speechStressedSyllable(word, syllables);

    size_t syllable = 0;
    bool started = false;
    for (size_t i = firstEvent; i < lastEvent; i++) {
        if (events[i].syllableStart) {
            if (started) {
                syllable++;
            }
            started = true;
        }
        const bool isStressed = started && syllable == stressed;
        events[i].stressed = isStressed;
        // A word of one syllable has nothing to alternate with, so it is left at its natural length.
        //
        // Only the vowel is stretched. Stress lengthening in speech falls almost entirely on the
        // nucleus, and stretching the consonants with it is not merely inaccurate: a stop's closure
        // is silence, so a /t/ stretched to 216 ms by stress and phrase-final lengthening became a
        // fifth of a second of nothing in the middle of a word.
        if (syllables > 1 && events[i].spec->type == PhonemeType::Vowel) {
            events[i].lengthScale *= isStressed ? StressedLength : UnstressedLength;
        }
    }
}

//! Returns whether the word was read as a reduced function word rather than through the rules.
bool appendWord(std::string_view padded, PhonemeEventList & events)
{
    const size_t firstEvent = events.size();

    const auto bare = padded.substr(1, padded.size() - 2);
    if (const auto entry = std::ranges::find(FunctionWords, bare, &LexiconEntry::word); entry != std::ranges::end(FunctionWords)) {
        appendPhonemes(entry->phonemes, events, false);
        for (size_t i = firstEvent; i < events.size(); i++) {
            events[i].lengthScale = FunctionWordLength;
        }
        if (events.size() > firstEvent) {
            events[firstEvent].wordStart = true;
        }
        return true;
    }

    size_t pos = 1;
    while (pos + 1 < padded.size()) {
        const auto matched = std::ranges::find_if(Rules, [&](const LetterRule & rule) {
            return padded.compare(pos, rule.focus.size(), rule.focus) == 0
              && matchLeft(padded, pos, rule.left)
              && matchRight(padded, pos + rule.focus.size(), rule.right);
        });

        if (matched == std::ranges::end(Rules)) {
            // Every letter's list ends in a fallback, so this can only be a character the rules do
            // not cover -- a digit, say. Skipping it is better than stalling.
            pos++;
            continue;
        }

        appendPhonemes(matched->phonemes, events, false);
        pos += matched->focus.size();
    }

    if (events.size() > firstEvent) {
        events[firstEvent].wordStart = true;
    }
    return false;
}

//! Reads the contents of a /.../ escape as literal phoneme names.
void appendLiteralPhonemes(std::string_view span, PhonemeEventList & events)
{
    const size_t firstEvent = events.size();
    size_t pos = 0;
    while (pos < span.size()) {
        while (pos < span.size() && std::isspace(static_cast<unsigned char>(span[pos]))) {
            pos++;
        }
        size_t end = pos;
        std::string name;
        while (end < span.size() && !std::isspace(static_cast<unsigned char>(span[end]))) {
            name += static_cast<char>(std::toupper(static_cast<unsigned char>(span[end])));
            end++;
        }
        pos = end;
        if (!name.empty()) {
            appendPhonemes(name, events, false);
        }
    }

    if (events.size() > firstEvent) {
        events[firstEvent].wordStart = true;
    }
}

//! Marks where syllables begin, by maximal onset.
//!
//! A syllable is built around a vowel, and the consonants before it belong to it rather than to the
//! syllable before -- "extra" divides EHKS-TRAH, not EHKST-RAH. English allows at most three
//! consonants in an onset and two covers all but a handful of words, so the run is capped there;
//! without a cap, a long consonant run between vowels would hand every one of its consonants to the
//! following syllable and leave the preceding one bare.
//!
//! At the start of a word there is no preceding syllable to leave anything to, so the whole run is
//! the onset, which is what makes "street" one syllable rather than two.
void markSyllables(PhonemeEventList & events)
{
    constexpr size_t maxOnset = 2;

    bool seenVowelInWord = false;
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].wordStart) {
            seenVowelInWord = false;
        }
        if (events[i].spec->type != PhonemeType::Vowel) {
            continue;
        }

        size_t onset = i;
        while (onset > 0
               && events[onset - 1].spec->type != PhonemeType::Vowel
               && events[onset - 1].spec->type != PhonemeType::Silence
               && !events[onset].wordStart) {
            onset--;
        }
        if (seenVowelInWord && i - onset > maxOnset) {
            onset = i - maxOnset;
        }

        events[onset].syllableStart = true;
        seenVowelInWord = true;
    }

    // A phrase of nothing but consonants still has to be divisible, or Step mode has nothing to
    // advance through.
    if (!events.empty() && std::ranges::none_of(events, &PhonemeEvent::syllableStart)) {
        events.front().syllableStart = true;
    }
}

//! Applies stress word by word, once the syllable marks are in place.
//!
//! It cannot be done as each word is read: which phonemes begin a syllable is decided by
//! markSyllables() over the finished phrase, and the stress rules are stated in syllables.
void applyWordStress(PhonemeEventList & events, const std::vector<Spelling> & spellings)
{
    size_t word = 0;
    size_t start = 0;
    for (size_t i = 0; i <= events.size(); i++) {
        const bool boundary = i == events.size() || events[i].wordStart;
        if (boundary && i > start) {
            if (word < spellings.size() && !spellings[word].word.empty()) {
                markWordStress(events, start, i, spellings[word].word, spellings[word].stress);
            }
            word++;
        }
        if (boundary) {
            start = i;
        }
    }
}

//! Shortens an unvoiced stop that will not be aspirated.
//!
//! One is long only because it has to hold a burst and 40 to 80 ms of aspiration after it. In a
//! cluster like /str/ it has neither -- the stop after /s/ is unaspirated -- and at the end of a
//! phrase it is not even released. All that length then becomes closure, which is silence: at full
//! length "destroy" came out as a hiss, then a tenth of a second of nothing, then a four millisecond
//! burst, and an isolated transient after a gap that long is a click however continuous the waveform
//! through it is. It made the /s/ before it stand out as a separate event too.
void shortenUnaspiratedStops(PhonemeEventList & events)
{
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type != PhonemeType::Plosive || events[i].spec->voicing > 0.0) {
            continue;
        }
        const bool afterFricative = i > 0
          && events[i - 1].spec->type == PhonemeType::Fricative
          && events[i - 1].spec->voicing <= 0.0;
        const bool endsPhrase = i + 1 == events.size() || events[i + 1].spec->type == PhonemeType::Silence;
        if (afterFricative || endsPhrase) {
            events[i].lengthScale *= UnaspiratedStopLength;
        }
    }
}

//! Lengthens the last syllable of every phrase.
//!
//! A phrase is what falls between the pauses, so this runs after the silences are in place. It is
//! the other half of the rhythm the function-word reductions give: those make the weak syllables
//! weak, and this makes the phrase end sound like an end.
void applyPhraseFinalLengthening(PhonemeEventList & events)
{
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type == PhonemeType::Silence) {
            continue;
        }
        if (i + 1 != events.size() && events[i + 1].spec->type != PhonemeType::Silence) {
            continue;
        }

        size_t syllable = i;
        while (syllable > 0 && !events[syllable].syllableStart) {
            syllable--;
        }
        for (size_t j = syllable; j <= i; j++) {
            // The nucleus again: a phrase ends on a long vowel, not on a long closure.
            if (events[j].spec->type == PhonemeType::Vowel) {
                events[j].lengthScale *= PhraseFinalLength;
            }
        }
    }
}

//! Suffixes that hang off the end without moving the stress: the root keeps it.
constexpr std::string_view NeutralSuffixes[] {
    "INESS", "INGLY", "EDLY", "NESS", "MENT", "LESS", "ING", "FUL", "EST", "ERS", "ED", "LY", "ER", "S"
};

//! Suffixes that take the stress themselves.
constexpr std::string_view FinalStressSuffixes[] {
    "ESQUE", "AIRE", "ETTE", "TEEN", "EER", "ESE", "OON", "EUR", "ADE", "EE"
};

//! Suffixes that put the stress on the syllable before them.
constexpr std::string_view PenultimateSuffixes[] {
    "ESCENCE", "ESCENT", "IENCE", "IENCY", "ICAL", "IOUS", "EOUS", "UOUS", "ITIVE", "CIENT",
    "TION", "SION", "CION", "IAN", "ION", "IAL", "IENT", "UAL", "UATE", "IC"
};

//! Suffixes that put the stress two syllables before them.
constexpr std::string_view AntepenultimateSuffixes[] {
    "ITUDE", "OLOGY", "GRAPHY", "CRACY", "ATIVE", "ATORY", "ULOUS", "METER", "PATHY",
    "ULAR", "INAL", "IMAL", "LOGY", "NOMY", "ITY", "ETY", "IFY", "EFY", "ATE", "IZE",
    "ISE", "ARY", "ORY", "ACY", "OMY"
};

//! Prefixes that carry no stress and so push it onto the root.
//!
//! Only ones long enough not to be the opening of an ordinary word by accident. They are the least
//! reliable part of this -- "reverence" and "substitute" are stressed on the very syllable this
//! rule moves away from -- but dropping the rule costs 8 points of accuracy, so they earn their
//! place even wrong as often as they are.
constexpr std::string_view UnstressedPrefixes[] {
    "TRANS", "INTER", "UNDER", "SUPER", "OVER", "ANTI", "AUTO", "DIS", "CON", "COM", "PRE",
    "PRO", "SUB", "MIS", "DE", "RE", "UN", "IN", "IM", "EX", "EN", "EM", "BE", "AD", "OB",
    "PER", "SUR"
};

bool endsWith(std::string_view word, std::string_view suffix)
{
    return word.size() >= suffix.size() && word.substr(word.size() - suffix.size()) == suffix;
}

//! Vowel groups in a spelling. Unlike spellingSyllables() this makes no allowance for a silent
//! final E, because it is used on a fragment of a word rather than a whole one: the "Ame" of
//! "Ame'rica" ends in an E that is not silent, it is the vowel of the syllable before the mark.
size_t vowelGroups(std::string_view word)
{
    size_t count = 0;
    bool previous = false;
    for (auto && c : word) {
        const bool vowel = std::string_view { "AEIOUY" }.find(c) != std::string_view::npos;
        if (vowel && !previous) {
            count++;
        }
        previous = vowel;
    }
    return count;
}

//! Syllables in a spelling, counted as vowel groups less a silent final E. Rough, and only used to
//! work out how many a neutral suffix added.
size_t spellingSyllables(std::string_view word)
{
    const auto isVowel = [](char c) {
        return std::string_view { "AEIOUY" }.find(c) != std::string_view::npos;
    };

    size_t count = 0;
    bool previous = false;
    for (auto && c : word) {
        if (isVowel(c) && !previous) {
            count++;
        }
        previous = isVowel(c);
    }
    if (count > 1 && endsWith(word, "E") && !endsWith(word, "LE") && !endsWith(word, "EE") && !endsWith(word, "YE")) {
        count--;
    }
    return std::max<size_t>(1, count);
}

} // namespace

size_t speechStressedSyllable(std::string_view word, size_t syllableCount)
{
    if (syllableCount < 2) {
        return 0;
    }

    // Checked before the neutral suffixes, or "mutineer" is read as "mutine" plus a neutral "-er".
    for (auto && suffix : FinalStressSuffixes) {
        if (endsWith(word, suffix)) {
            return syllableCount - 1;
        }
    }

    for (auto && suffix : NeutralSuffixes) {
        if (endsWith(word, suffix) && word.size() - suffix.size() >= 3) {
            const auto base = word.substr(0, word.size() - suffix.size());
            const auto added = static_cast<long>(syllableCount) - static_cast<long>(spellingSyllables(base));
            if (added >= 0) {
                // The root keeps the stress, so the answer counted from the *start* carries over
                // unchanged however many syllables the suffix added.
                return speechStressedSyllable(base, std::max<size_t>(1, syllableCount - static_cast<size_t>(added)));
            }
            break;
        }
    }

    for (auto && suffix : AntepenultimateSuffixes) {
        if (endsWith(word, suffix) && syllableCount >= 3) {
            return syllableCount - 3;
        }
    }

    for (auto && suffix : PenultimateSuffixes) {
        if (endsWith(word, suffix)) {
            return syllableCount - 2;
        }
    }

    for (auto && prefix : UnstressedPrefixes) {
        if (word.size() >= prefix.size() && word.substr(0, prefix.size()) == prefix
            && word.size() - prefix.size() >= 3) {
            return std::min<size_t>(1, syllableCount - 1);
        }
    }

    return 0;
}

namespace {

} // namespace

PhonemeEventList textToPhonemes(std::string_view text)
{
    PhonemeEventList events;
    // The spelling behind each word, in order, so the stress pass can consult it once the syllable
    // marks exist. Empty for anything the stress rules have no spelling to work from: an escape, a
    // pause, or a function word, which is unstressed by definition.
    std::vector<Spelling> spellings;

    size_t pos = 0;
    while (pos < text.size()) {
        const auto character = static_cast<unsigned char>(text[pos]);

        if (text[pos] == '/') {
            const size_t end = std::min(text.find('/', pos + 1), text.size());
            const size_t before = events.size();
            appendLiteralPhonemes(text.substr(pos + 1, end - pos - 1), events);
            if (events.size() > before) {
                spellings.emplace_back();
            }
            pos = std::min(end + 1, text.size());
        } else if (std::isalpha(character) || isApostrophe(text, pos)) {
            // Padded either side so the rules' word-boundary class needs no special case at the ends.
            std::string word { ' ' };
            std::vector<size_t> marks;
            while (pos < text.size()) {
                if (std::isalpha(static_cast<unsigned char>(text[pos]))) {
                    word += static_cast<char>(std::toupper(static_cast<unsigned char>(text[pos])));
                    pos++;
                } else if (const size_t length = apostropheLength(text, pos); length) {
                    marks.push_back(word.size());
                    word += '\'';
                    pos += length;
                } else {
                    break;
                }
            }
            word += ' ';

            // A mark that is not a contraction is a stress mark: it names the syllable it stands in
            // front of, which is the one after however many the spelling holds before it.
            std::optional<size_t> stress;
            for (auto && mark : marks) {
                if (!isContraction(word, mark)) {
                    // The mark stands in front of the syllable it names, so the index is however
                    // many syllables the spelling holds before it.
                    stress = vowelGroups(std::string_view { word }.substr(1, mark - 1));
                    word.erase(mark, 1);
                    break;
                }
            }

            const size_t before = events.size();
            const bool reduced = appendWord(word, events);
            if (events.size() > before) {
                spellings.push_back({ reduced ? std::string {} : word.substr(1, word.size() - 2), stress });
            }
        } else {
            // Punctuation is a pause; a plain space only separates words. Speech does not stop
            // between every word, and in Step mode a silence between them would eat a note.
            if (std::string_view { ".,;:!?-" }.find(text[pos]) != std::string_view::npos) {
                events.push_back({ &speechSilence(), true, true });
                spellings.emplace_back();
            }
            pos++;
        }
    }

    markSyllables(events);
    shortenUnaspiratedStops(events);
    applyWordStress(events, spellings);
    applyPhraseFinalLengthening(events);
    return events;
}

bool speechRulesAreWellFormed()
{
    return std::ranges::all_of(Rules, [](const LetterRule & rule) {
        if (rule.focus.empty()) {
            return false; // A rule consuming nothing would stall the walk.
        }
        PhonemeEventList produced;
        appendPhonemes(rule.phonemes, produced, false);
        const auto names = std::ranges::count(rule.phonemes, ' ') + (rule.phonemes.empty() ? 0 : 1);
        // Affricates expand to two phonemes, so the count only has to be at least the names given.
        return produced.size() >= static_cast<size_t>(names);
    });
}

std::string phonemeNames(const PhonemeEventList & events)
{
    std::string names;
    for (auto && event : events) {
        if (!names.empty()) {
            names += event.wordStart ? "  " : " ";
        }
        names += event.spec->name;
    }
    return names;
}

} // namespace noteahead
