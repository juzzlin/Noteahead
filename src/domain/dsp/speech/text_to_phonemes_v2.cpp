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

#include "text_to_phonemes_v2.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace noteahead {

namespace {

constexpr std::string_view PausePunctuation = ".,;:!?-";
constexpr std::string_view LineEndPunctuation = ".!?";

constexpr double FunctionWordLength = 0.75;
constexpr double StressedLength = 1.3;
constexpr double UnstressedLength = 0.8;
constexpr double UnaspiratedStopLength = 0.55;
constexpr double PhraseFinalLength = 1.45;
constexpr double PreFortisClipScale = 0.70;
constexpr double PreFortisClusterClipScale = 0.80;

constexpr PhonemeSpec FlapPhoneme {
    "DX",
    PhonemeType::Liquid,
    { { { 300.0, 140.0, 1.0 }, { 1750.0, 140.0, 0.5 }, { 2650.0, 180.0, 0.3 } } },
    std::nullopt,
    0.7,
    0.35,
    32
};

struct LexiconEntry
{
    std::string_view word;
    std::string_view phonemes;
    int stressedSyllable;
};

constexpr LexiconEntry Lexicon[] {
    // Contractions & casual forms
    { "i'm", "AY M", 0 },
    { "i've", "AY V", 0 },
    { "i'll", "AY L", 0 },
    { "i'd", "AY D", 0 },
    { "we'll", "W IY L", 0 },
    { "we're", "W IH R", 0 },
    { "we've", "W IY V", 0 },
    { "we'd", "W IY D", 0 },
    { "you're", "Y UW R", 0 },
    { "you'll", "Y UW L", 0 },
    { "you've", "Y UW V", 0 },
    { "you'd", "Y UW D", 0 },
    { "they're", "DH EH R", 0 },
    { "they'll", "DH EY L", 0 },
    { "they've", "DH EY V", 0 },
    { "they'd", "DH EY D", 0 },
    { "he's", "HH IY Z", 0 },
    { "he'll", "HH IY L", 0 },
    { "he'd", "HH IY D", 0 },
    { "she's", "SH IY Z", 0 },
    { "she'll", "SH IY L", 0 },
    { "she'd", "SH IY D", 0 },
    { "it's", "IH T S", 0 },
    { "it'll", "IH T AX L", 0 },
    { "that's", "DH AE T S", 0 },
    { "what's", "W AH T S", 0 },
    { "there's", "DH EH R Z", 0 },
    { "here's", "HH IH R Z", 0 },
    { "let's", "L EH T S", 0 },
    { "who's", "HH UW Z", 0 },
    { "won't", "W OW N T", 0 },
    { "can't", "K AE N T", 0 },
    { "don't", "D OW N T", 0 },
    { "doesn't", "D AH Z AX N T", 0 },
    { "didn't", "D IH D AX N T", 0 },
    { "isn't", "IH Z AX N T", 0 },
    { "aren't", "AA R N T", 0 },
    { "wasn't", "W AA Z AX N T", 0 },
    { "weren't", "W ER N T", 0 },
    { "haven't", "HH AE V AX N T", 0 },
    { "hasn't", "HH AE Z AX N T", 0 },
    { "hadn't", "HH AE D AX N T", 0 },
    { "wouldn't", "W UH D AX N T", 0 },
    { "couldn't", "K UH D AX N T", 0 },
    { "shouldn't", "SH UH D AX N T", 0 },
    { "ain't", "EY N T", 0 },
    { "gonna", "G AO N AX", 0 },
    { "wanna", "W AA N AX", 0 },
    { "gotta", "G AA DX AX", 0 },

    // Determiner & high-frequency irregular English words + music/synth vocabulary
    { "the", "DH AX", -1 },
    { "love", "L AH V", 0 },
    { "heart", "HH AA R T", 0 },
    { "work", "W ER K", 0 },
    { "world", "W ER L D", 0 },
    { "word", "W ER D", 0 },
    { "one", "W AH N", 0 },
    { "once", "W AH N S", 0 },
    { "two", "T UW", 0 },
    { "four", "F AO R", 0 },
    { "eight", "EY T", 0 },
    { "done", "D AH N", 0 },
    { "come", "K AH M", 0 },
    { "some", "S AH M", 0 },
    { "none", "N AH N", 0 },
    { "move", "M UW V", 0 },
    { "prove", "P R UW V", 0 },
    { "lose", "L UW Z", 0 },
    { "choose", "T SH UW Z", 0 },
    { "who", "HH UW", 0 },
    { "whose", "HH UW Z", 0 },
    { "where", "W EH R", 0 },
    { "there", "DH EH R", 0 },
    { "their", "DH EH R", 0 },
    { "they", "DH EY", 0 },
    { "said", "S EH D", 0 },
    { "says", "S EH Z", 0 },
    { "have", "HH AE V", 0 },
    { "give", "G IH V", 0 },
    { "live", "L IH V", 0 },
    { "are", "AA R", 0 },
    { "were", "W ER", 0 },
    { "was", "W AA Z", 0 },
    { "what", "W AH T", 0 },
    { "want", "W AA N T", 0 },
    { "water", "W AO T ER", 0 },
    { "watch", "W AA T SH", 0 },
    { "walk", "W AO K", 0 },
    { "talk", "T AO K", 0 },
    { "half", "HH AE F", 0 },
    { "calm", "K AA M", 0 },
    { "could", "K UH D", 0 },
    { "would", "W UH D", 0 },
    { "should", "SH UH D", 0 },
    { "through", "TH R UW", 0 },
    { "though", "DH OW", 0 },
    { "thought", "TH AO T", 0 },
    { "enough", "IH N AH F", 1 },
    { "tough", "T AH F", 0 },
    { "rough", "R AH F", 0 },
    { "laugh", "L AE F", 0 },
    { "cough", "K AO F", 0 },
    { "daughter", "D AO T ER", 0 },
    { "night", "N AY T", 0 },
    { "light", "L AY T", 0 },
    { "right", "R AY T", 0 },
    { "bright", "B R AY T", 0 },
    { "fight", "F AY T", 0 },
    { "might", "M AY T", 0 },
    { "sight", "S AY T", 0 },
    { "high", "HH AY", 0 },
    { "sign", "S AY N", 0 },
    { "island", "AY L AX N D", 0 },
    { "listen", "L IH S AX N", 0 },
    { "often", "AO F AX N", 0 },
    { "castle", "K AE S AX L", 0 },
    { "whistle", "W IH S AX L", 0 },
    { "answer", "AE N S ER", 0 },
    { "sword", "S AO R D", 0 },
    { "iron", "AY ER N", 0 },
    { "hour", "AW ER", 0 },
    { "honest", "AA N AX S T", 0 },
    { "honor", "AA N ER", 0 },
    { "ghost", "G OW S T", 0 },
    { "know", "N OW", 0 },
    { "knew", "N UW", 0 },
    { "knife", "N AY F", 0 },
    { "knee", "N IY", 0 },
    { "knock", "N AA K", 0 },
    { "write", "R AY T", 0 },
    { "wrote", "R OW T", 0 },
    { "wrong", "R AO NG", 0 },
    { "climb", "K L AY M", 0 },
    { "comb", "K OW M", 0 },
    { "thumb", "TH AH M", 0 },
    { "bomb", "B AA M", 0 },
    { "lamb", "L AE M", 0 },
    { "debt", "D EH T", 0 },
    { "doubt", "D AW T", 0 },
    { "subtle", "S AH T AX L", 0 },
    { "people", "P IY P AX L", 0 },
    { "purple", "P ER P AX L", 0 },
    { "circle", "S ER K AX L", 0 },
    { "eye", "AY", 0 },
    { "eyes", "AY Z", 0 },
    { "buy", "B AY", 0 },
    { "guy", "G AY", 0 },
    { "build", "B IH L D", 0 },
    { "built", "B IH L T", 0 },
    { "guitar", "G IH T AA R", 1 },
    { "friend", "F R EH N D", 0 },
    { "blood", "B L AH D", 0 },
    { "flood", "F L AH D", 0 },
    { "door", "D AO R", 0 },
    { "floor", "F L AO R", 0 },
    { "poor", "P UH R", 0 },
    { "soul", "S OW L", 0 },
    { "busy", "B IH Z IY", 0 },
    { "business", "B IH Z N AX S", 0 },
    { "minute", "M IH N AX T", 0 },
    { "women", "W IH M AX N", 0 },
    { "woman", "W UH M AX N", 0 },
    { "sugar", "SH UH G ER", 0 },
    { "sure", "SH UH R", 0 },
    { "machine", "M AX SH IY N", 1 },
    { "rhythm", "R IH DH AX M", 0 },
    { "bass", "B EY S", 0 },
    { "synth", "S IH N TH", 0 },
    { "chorus", "K AO R AX S", 0 },
    { "echo", "EH K OW", 0 },
    { "chaos", "K EY AA S", 0 },
    { "choir", "K W AY ER", 0 },
    { "chord", "K AO R D", 0 },
    { "ocean", "OW SH AX N", 0 },
    { "motion", "M OW SH AX N", 0 },
    { "emotion", "IH M OW SH AX N", 1 },
    { "station", "S T EY SH AX N", 0 },
    { "nation", "N EY SH AX N", 0 },
    { "future", "F Y UW T SH ER", 0 },
    { "nature", "N EY T SH ER", 0 },
    { "music", "M Y UW Z IH K", 0 },
    { "human", "HH Y UW M AX N", 0 },
    { "voice", "V OY S", 0 },
    { "noise", "N OY Z", 0 },
    { "silence", "S AY L AX N S", 0 },
    { "heaven", "HH EH V AX N", 0 },
    { "seven", "S EH V AX N", 0 },
    { "eleven", "IH L EH V AX N", 1 },
    { "earth", "ER TH", 0 },
    { "learn", "L ER N", 0 },
    { "heard", "HH ER D", 0 },
    { "early", "ER L IY", 0 },
    { "search", "S ER T SH", 0 },
    { "touch", "T AH T SH", 0 },
    { "young", "Y AH NG", 0 },
    { "country", "K AH N T R IY", 0 },
    { "trouble", "T R AH B AX L", 0 },
    { "double", "D AH B AX L", 0 },
    { "couple", "K AH P AX L", 0 },
    { "money", "M AH N IY", 0 },
    { "honey", "HH AH N IY", 0 },
    { "nothing", "N AH TH IH NG", 0 },
    { "mother", "M AH DH ER", 0 },
    { "father", "F AA DH ER", 0 },
    { "brother", "B R AH DH ER", 0 },
    { "other", "AH DH ER", 0 },
    { "another", "AX N AH DH ER", 1 },
    { "together", "T AX G EH DH ER", 1 },
    { "forever", "F AO R EH V ER", 1 },
    { "beauty", "B Y UW T IY", 0 },
    { "beautiful", "B Y UW T AX F UH L", 0 },
    { "different", "D IH F R AX N T", 0 },
    { "every", "EH V R IY", 0 },
    { "everything", "EH V R IY TH IH NG", 0 },
    { "everyone", "EH V R IY W AH N", 0 },
    { "again", "AX G EH N", 1 },
    { "against", "AX G EH N S T", 1 },
    { "already", "AO L R EH D IY", 1 },
    { "attack", "AX T AE K", 1 },
    { "always", "AO L W EY Z", 0 },
    { "almost", "AO L M OW S T", 0 },
    { "because", "B IH K AO Z", 1 },
    { "behind", "B IH HH AY N D", 1 },
    { "believe", "B IH L IY V", 1 },
    { "between", "B IH T W IY N", 1 },
    { "beyond", "B IH Y AA N D", 1 },
    { "change", "T SH EY N D ZH", 0 },
    { "child", "T SH AY L D", 0 },
    { "children", "T SH IH L D R AX N", 0 },
    { "city", "S IH T IY", 0 },
    { "color", "K AH L ER", 0 },
    { "create", "K R IY EY T", 1 },
    { "dance", "D AE N S", 0 },
    { "dream", "D R IY M", 0 },
    { "energy", "EH N ER D ZH IY", 0 },
    { "evening", "IY V N IH NG", 0 },
    { "family", "F AE M AX L IY", 0 },
    { "feeling", "F IY L IH NG", 0 },
    { "finally", "F AY N AX L IY", 0 },
    { "fire", "F AY ER", 0 },
    { "follow", "F AA L OW", 0 },
    { "forest", "F AO R AX S T", 0 },
    { "free", "F R IY", 0 },
    { "great", "G R EY T", 0 },
    { "break", "B R EY K", 0 },
    { "steak", "S T EY K", 0 },
    { "head", "HH EH D", 0 },
    { "dead", "D EH D", 0 },
    { "bread", "B R EH D", 0 },
    { "ready", "R EH D IY", 0 },
    { "heavy", "HH EH V IY", 0 },
    { "measure", "M EH ZH ER", 0 },
    { "pleasure", "P L EH ZH ER", 0 },
    { "treasure", "T R EH ZH ER", 0 },
    { "weather", "W EH DH ER", 0 },
    { "feather", "F EH DH ER", 0 },
    { "leather", "L EH DH ER", 0 },
    { "breath", "B R EH TH", 0 },
    { "breathe", "B R IY DH", 0 },
    { "hello", "HH EH L OW", 1 },
    { "goodbye", "G UH D B AY", 1 },
    { "welcome", "W EH L K AX M", 0 },
    { "morning", "M AO R N IH NG", 0 },
    { "midnight", "M IH D N AY T", 0 },
    { "shadow", "SH AE D OW", 0 },
    { "window", "W IH N D OW", 0 },
    { "yellow", "Y EH L OW", 0 },
    { "tomorrow", "T AX M AA R OW", 1 },
    { "today", "T AX D EY", 1 },
    { "tonight", "T AX N AY T", 1 },
    { "system", "S IH S T AX M", 0 },
    { "signal", "S IH G N AX L", 0 },
    { "filter", "F IH L T ER", 0 },
    { "stereo", "S T EH R IY OW", 0 },
    { "audio", "AO D IY OW", 0 },
    { "piano", "P IY AE N OW", 1 },
    { "melody", "M EH L AX D IY", 0 },
    { "harmony", "HH AA R M AX N IY", 0 },
    { "groove", "G R UW V", 0 },
    { "tempo", "T EH M P OW", 0 },
    { "electric", "IH L EH K T R IH K", 1 },
    { "digital", "D IH D ZH IH T AX L", 0 },
    { "analog", "AE N AX L AO G", 0 },
    { "cosmic", "K AA Z M IH K", 0 },
    { "crystal", "K R IH S T AX L", 0 },
    { "phantom", "F AE N T AX M", 0 },
    { "cyber", "S AY B ER", 0 },
    { "neon", "N IY AA N", 0 },
    { "laser", "L EY Z ER", 0 },
    { "robot", "R OW B AA T", 0 },
    { "alien", "EY L IY AX N", 0 },
    { "monster", "M AA N S T ER", 0 },
    { "giant", "D ZH AY AX N T", 0 },
    { "elder", "EH L D ER", 0 },
    { "better", "B EH T ER", 0 },
    { "little", "L IH T AX L", 0 },
    { "butter", "B AH T ER", 0 },
    { "party", "P AA R T IY", 0 },
    { "letter", "L EH T ER", 0 },
    { "metal", "M EH T AX L", 0 },
    { "oh", "OW", 0 },
};

const PhonemeSpec * resolvePhoneme(std::string_view name)
{
    if (name == "DX") {
        return &FlapPhoneme;
    }
    return speechPhoneme(name);
}

size_t apostropheLength(std::string_view text, size_t pos)
{
    if (text[pos] == '\'') {
        return 1;
    }
    if (pos + 2 < text.size()
        && static_cast<unsigned char>(text[pos]) == 0xE2
        && static_cast<unsigned char>(text[pos + 1]) == 0x80
        && static_cast<unsigned char>(text[pos + 2]) == 0x99) {
        return 3;
    }
    return 0;
}

bool isApostrophe(std::string_view text, size_t pos)
{
    return apostropheLength(text, pos) > 0;
}

std::string digitWord(char d)
{
    static constexpr std::string_view Digits[] {
        "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
    };
    return std::string { Digits[static_cast<size_t>(d - '0')] };
}

void appendWordSpaced(std::string & out, std::string_view word)
{
    if (!out.empty() && !std::isspace(static_cast<unsigned char>(out.back()))) {
        out += ' ';
    }
    out += word;
}

std::string cardinalBelowThousand(int n)
{
    static constexpr std::string_view Ones[] {
        "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
        "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
    };
    static constexpr std::string_view Tens[] {
        "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
    };

    std::string out;
    if (n >= 100) {
        out += Ones[n / 100];
        out += " hundred";
        n %= 100;
        if (n > 0) {
            out += ' ';
        }
    }
    if (n >= 20) {
        out += Tens[n / 10];
        if (n % 10 > 0) {
            out += ' ';
            out += Ones[n % 10];
        }
    } else if (n > 0) {
        out += Ones[n];
    }
    return out;
}

std::string cardinalNumber(int n)
{
    if (n == 0) {
        return "zero";
    }
    if (n >= 1000) {
        std::string out = cardinalBelowThousand(n / 1000);
        out += " thousand";
        const int rem = n % 1000;
        if (rem > 0) {
            out += ' ';
            out += cardinalBelowThousand(rem);
        }
        return out;
    }
    return cardinalBelowThousand(n);
}

std::string expandDigitRun(std::string_view digits)
{
    if (digits.empty()) {
        return {};
    }
    if (digits.size() >= 7 || (digits.size() > 1 && digits[0] == '0')) {
        std::string out;
        for (const char d : digits) {
            appendWordSpaced(out, digitWord(d));
        }
        return out;
    }

    int value = 0;
    for (const char d : digits) {
        value = value * 10 + (d - '0');
    }

    // Classic drum machine / synth model numbers: 808 -> "eight oh eight"
    if (digits.size() == 3 && digits[0] != '0' && digits[1] == '0' && digits[2] != '0') {
        return digitWord(digits[0]) + " oh " + digitWord(digits[2]);
    }

    // Four-digit years 1100..2099
    if (digits.size() == 4 && value >= 1100 && value <= 2099) {
        if (value >= 2000 && value <= 2009) {
            std::string out = "two thousand";
            if (value > 2000) {
                out += ' ';
                out += digitWord(digits[3]);
            }
            return out;
        }
        if (value >= 2010) {
            return "twenty " + cardinalBelowThousand(value % 100);
        }
        const int hi = value / 100;
        const int lo = value % 100;
        if (lo == 0) {
            return cardinalBelowThousand(hi) + " hundred";
        }
        if (lo < 10) {
            return cardinalBelowThousand(hi) + " oh " + digitWord(digits[3]);
        }
        return cardinalBelowThousand(hi) + " " + cardinalBelowThousand(lo);
    }

    return cardinalNumber(value);
}

std::string expandNumbers(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 32);

    size_t pos = 0;
    while (pos < text.size()) {
        if (text[pos] == '/') {
            const size_t end = text.find('/', pos + 1);
            const size_t copyEnd = (end == std::string_view::npos) ? text.size() : (end + 1);
            out.append(text.substr(pos, copyEnd - pos));
            pos = copyEnd;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(text[pos]))) {
            const size_t start = pos;
            while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
                pos++;
            }
            appendWordSpaced(out, expandDigitRun(text.substr(start, pos - start)));
            if (pos < text.size() && std::isalpha(static_cast<unsigned char>(text[pos]))) {
                out += ' ';
            }
            continue;
        }
        out += text[pos];
        pos++;
    }
    return out;
}

void appendSpacePhonemes(std::string_view phonemeList, PhonemeEventList & events, bool markFirstAsWordStart)
{
    const size_t firstEvent = events.size();
    size_t pos = 0;
    while (pos < phonemeList.size()) {
        while (pos < phonemeList.size() && std::isspace(static_cast<unsigned char>(phonemeList[pos]))) {
            pos++;
        }
        const size_t start = pos;
        while (pos < phonemeList.size() && !std::isspace(static_cast<unsigned char>(phonemeList[pos]))) {
            pos++;
        }
        if (pos > start) {
            if (const auto * spec = resolvePhoneme(phonemeList.substr(start, pos - start)); spec) {
                events.push_back({ spec, false, markFirstAsWordStart && events.size() == firstEvent });
            }
        }
    }
}

void markWordSyllables(PhonemeEventList & events, size_t from, size_t to)
{
    constexpr size_t maxOnset = 2;
    bool seenVowel = false;
    for (size_t i = from; i < to; i++) {
        if (events[i].spec->type != PhonemeType::Vowel) {
            continue;
        }
        size_t onset = i;
        while (onset > from
               && events[onset - 1].spec->type != PhonemeType::Vowel
               && events[onset - 1].spec->type != PhonemeType::Silence) {
            onset--;
        }
        if (seenVowel && i - onset > maxOnset) {
            onset = i - maxOnset;
        }
        events[onset].syllableStart = true;
        seenVowel = true;
    }
    if (to > from) {
        const bool hasSyllableStart = std::any_of(
          events.begin() + static_cast<ptrdiff_t>(from),
          events.begin() + static_cast<ptrdiff_t>(to),
          [](const PhonemeEvent & e) { return e.syllableStart; });
        if (!hasSyllableStart) {
            events[from].syllableStart = true;
        }
    }
}

void applyLexiconStress(PhonemeEventList & events, size_t from, size_t to, int stressedSyllable)
{
    if (stressedSyllable < 0) {
        for (size_t i = from; i < to; i++) {
            events[i].lengthScale = FunctionWordLength;
        }
        return;
    }

    size_t syllables = 0;
    for (size_t i = from; i < to; i++) {
        if (events[i].syllableStart) {
            syllables++;
        }
    }
    if (!syllables) {
        return;
    }

    const size_t targetSyllable = std::min(static_cast<size_t>(stressedSyllable), syllables - 1);
    size_t syllable = 0;
    bool started = false;
    for (size_t i = from; i < to; i++) {
        if (events[i].syllableStart) {
            if (started) {
                syllable++;
            }
            started = true;
        }
        const bool stressed = started && syllable == targetSyllable;
        events[i].stressed = stressed;
        if (syllables > 1 && events[i].spec->type == PhonemeType::Vowel) {
            events[i].lengthScale *= stressed ? StressedLength : UnstressedLength;
        }
    }
}

void appendWordFallback(std::string_view rawWord, bool lineStartContext, PhonemeEventList & events)
{
    std::string probe;
    if (lineStartContext && rawWord == "A") {
        probe = "a bo";
    } else {
        probe = std::string { rawWord } + " bo";
    }

    auto sub = textToPhonemes(probe);
    size_t lastWordStart = sub.size();
    for (size_t i = sub.size(); i > 0; i--) {
        if (sub[i - 1].wordStart) {
            lastWordStart = i - 1;
            break;
        }
    }
    if (lastWordStart < sub.size()) {
        sub.erase(sub.begin() + static_cast<ptrdiff_t>(lastWordStart), sub.end());
    }
    for (auto & event : sub) {
        event.lineStart = false;
        events.push_back(event);
    }
}

bool isSingleCapitalLetterToken(std::string_view expanded, size_t tokenStart, size_t tokenEnd, bool pendingLineStart)
{
    if (tokenEnd != tokenStart + 1) {
        return false;
    }
    const char c = expanded[tokenStart];
    if (!std::isupper(static_cast<unsigned char>(c))) {
        return false;
    }
    if (pendingLineStart && c == 'A') {
        size_t p = tokenEnd;
        while (p < expanded.size() && (expanded[p] == ' ' || PausePunctuation.find(expanded[p]) != std::string_view::npos)) {
            p++;
        }
        if (p < expanded.size() && std::isupper(static_cast<unsigned char>(expanded[p]))) {
            const size_t next = p + 1;
            if (next >= expanded.size() || (!std::isalpha(static_cast<unsigned char>(expanded[next])) && !isApostrophe(expanded, next))) {
                return true;
            }
        }
        return false;
    }
    return true;
}

bool isVoicelessObstruent(const PhonemeSpec * spec)
{
    return (spec->type == PhonemeType::Plosive || (spec->type == PhonemeType::Fricative && spec->name != "HH"))
      && spec->voicing <= 0.0;
}

void applyContextAwareThe(PhonemeEventList & events, const std::vector<std::pair<size_t, size_t>> & theSpans)
{
    const auto * iy = speechPhoneme("IY");
    if (!iy) {
        return;
    }
    for (const auto & [from, to] : theSpans) {
        if (to < events.size() && events[to].spec->type == PhonemeType::Vowel) {
            for (size_t i = from; i < to; i++) {
                if (events[i].spec->type == PhonemeType::Vowel) {
                    events[i].spec = iy;
                    events[i].lengthScale = 0.85;
                }
            }
        }
    }
}

void applyAlveolarFlap(PhonemeEventList & events)
{
    if (events.size() < 3) {
        return;
    }
    for (size_t i = 1; i + 1 < events.size(); i++) {
        const auto name = events[i].spec->name;
        if ((name == "T" || name == "D")
            && !events[i].wordStart
            && (events[i - 1].spec->type == PhonemeType::Vowel || events[i - 1].spec->name == "R")
            && events[i + 1].spec->type == PhonemeType::Vowel
            && !events[i + 1].stressed) {
            events[i].spec = &FlapPhoneme;
            events[i].lengthScale = 1.0;
        }
    }
}

void applyPreFortisClipping(PhonemeEventList & events)
{
    for (size_t i = 0; i + 1 < events.size(); i++) {
        if (events[i].spec->type != PhonemeType::Vowel) {
            continue;
        }
        if (events[i + 1].wordStart || events[i + 1].spec->type == PhonemeType::Silence) {
            continue;
        }
        if (isVoicelessObstruent(events[i + 1].spec)) {
            events[i].lengthScale *= PreFortisClipScale;
        } else if ((events[i + 1].spec->type == PhonemeType::Nasal || events[i + 1].spec->type == PhonemeType::Liquid)
                   && i + 2 < events.size()
                   && !events[i + 2].wordStart
                   && events[i + 2].spec->type != PhonemeType::Silence
                   && isVoicelessObstruent(events[i + 2].spec)) {
            events[i].lengthScale *= PreFortisClusterClipScale;
        }
    }
}

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
            if (events[j].spec->type == PhonemeType::Vowel) {
                events[j].lengthScale *= PhraseFinalLength;
            }
        }
    }
}

} // namespace

const PhonemeSpec & speechFlapPhoneme()
{
    return FlapPhoneme;
}

PhonemeEventList textToPhonemesV2(std::string_view text)
{
    const std::string expanded = expandNumbers(text);
    const std::string_view view { expanded };

    PhonemeEventList events;
    std::vector<std::pair<size_t, size_t>> theSpans;
    bool pendingLineStart = true;

    size_t pos = 0;
    while (pos < view.size()) {
        const size_t lineCandidate = events.size();

        if (view[pos] == '/') {
            const size_t end = std::min(view.find('/', pos + 1), view.size());
            const auto span = view.substr(pos + 1, end - pos - 1);
            std::string upper;
            upper.reserve(span.size());
            for (const char ch : span) {
                upper += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            const size_t before = events.size();
            appendSpacePhonemes(upper, events, true);
            markWordSyllables(events, before, events.size());
            pos = std::min(end + 1, view.size());
        } else if (std::isalpha(static_cast<unsigned char>(view[pos])) || isApostrophe(view, pos)) {
            const size_t wordStart = pos;
            std::string rawWord;
            std::string lowerWord;
            while (pos < view.size()) {
                if (std::isalpha(static_cast<unsigned char>(view[pos]))) {
                    rawWord += view[pos];
                    lowerWord += static_cast<char>(std::tolower(static_cast<unsigned char>(view[pos])));
                    pos++;
                } else if (const size_t apLen = apostropheLength(view, pos); apLen > 0) {
                    rawWord.append(view.substr(pos, apLen));
                    lowerWord += '\'';
                    pos += apLen;
                } else {
                    break;
                }
            }

            const size_t before = events.size();
            const bool singleCap = isSingleCapitalLetterToken(view, wordStart, pos, pendingLineStart);
            const auto lexIt = (!singleCap)
              ? std::ranges::find(Lexicon, lowerWord, &LexiconEntry::word)
              : std::ranges::end(Lexicon);

            if (lexIt != std::ranges::end(Lexicon)) {
                appendSpacePhonemes(lexIt->phonemes, events, true);
                markWordSyllables(events, before, events.size());
                applyLexiconStress(events, before, events.size(), lexIt->stressedSyllable);
                if (lexIt->word == "the") {
                    theSpans.emplace_back(before, events.size());
                }
            } else {
                appendWordFallback(rawWord, pendingLineStart, events);
            }
        } else {
            if (PausePunctuation.find(view[pos]) != std::string_view::npos) {
                events.push_back({ &speechSilence(), true, true });
                if (LineEndPunctuation.find(view[pos]) != std::string_view::npos) {
                    pendingLineStart = true;
                }
            }
            pos++;
        }

        if (pendingLineStart && events.size() > lineCandidate && events[lineCandidate].spec->type != PhonemeType::Silence) {
            events[lineCandidate].lineStart = true;
            pendingLineStart = false;
        }
    }

    applyContextAwareThe(events, theSpans);
    applyAlveolarFlap(events);
    applyPreFortisClipping(events);
    shortenUnaspiratedStops(events);
    applyPhraseFinalLengthening(events);
    return events;
}

bool speechV2LexiconIsWellFormed()
{
    return std::ranges::all_of(Lexicon, [](const LexiconEntry & entry) {
        if (entry.word.empty() || entry.phonemes.empty()) {
            return false;
        }
        size_t pos = 0;
        size_t count = 0;
        while (pos < entry.phonemes.size()) {
            while (pos < entry.phonemes.size() && std::isspace(static_cast<unsigned char>(entry.phonemes[pos]))) {
                pos++;
            }
            const size_t start = pos;
            while (pos < entry.phonemes.size() && !std::isspace(static_cast<unsigned char>(entry.phonemes[pos]))) {
                pos++;
            }
            if (pos > start) {
                if (!resolvePhoneme(entry.phonemes.substr(start, pos - start))) {
                    return false;
                }
                count++;
            }
        }
        return count > 0;
    });
}

} // namespace noteahead
