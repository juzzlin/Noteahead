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

#ifndef EFFECT_PRESETS_TEST_HPP
#define EFFECT_PRESETS_TEST_HPP

#include <QObject>

namespace noteahead {

class EffectPresetsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_delayPresets_names_shouldBeUniqueAndAlphabetical();
    void test_delayPresets_shouldNameOnlyKnownParameters();
    void test_delayPresets_shouldAllSetAMixAboveZero();
    void test_delayPresets_syncedDivisions_shouldLandOnNoteValues();
    void test_delayPresets_dottedEighth_shouldMatchTheSourceNotes();
    void test_delayPresets_slapback_shouldMatchTheSourceNotes();

    void test_autoDuckerPresets_names_shouldBeUniqueAndAlphabetical();
    void test_autoDuckerPresets_shouldNameOnlyKnownParameters();
    void test_autoDuckerPresets_shouldAllDuckRatherThanBoost();
    void test_autoDuckerPresets_classicPump_shouldMatchTheSourceNotes();
    void test_autoDuckerPresets_shouldNeverNameASideChainSource();

    void test_applyFactoryPreset_shouldKeepTheSideChainSource();
    void test_applyPresetParametersFromXml_shouldKeepTheSideChainSource();
};

} // namespace noteahead

#endif // EFFECT_PRESETS_TEST_HPP
