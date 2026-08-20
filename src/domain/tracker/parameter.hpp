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

#ifndef PARAMETER_HPP
#define PARAMETER_HPP

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

using LegacyNameList = std::vector<std::string>;

//! Rescales a value that arrived under a legacy name into the current parameter's domain. Needed
//! when a parameter is not merely renamed but reinterpreted, since the stored number is a position
//! within whatever range was in effect when the project was saved.
using LegacyValueConverter = std::function<float(float)>;

class Parameter
{
public:
    enum class Type
    {
        Continuous,
        Discrete,
        Boolean
    };

    Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault, int xmlScale = 1, Type type = Type::Continuous, LegacyNameList legacyNames = {}, LegacyValueConverter legacyValueConverter = {});

    const std::string & name() const;

    //! The value in effect: what the DSP runs on and what a knob shows. Automation moves this one.
    float value() const;
    //! The value the user set, and the only one that is ever saved. Automation leaves it alone.
    float authoredValue() const;

    //! Writes both layers. Everything that authors a value -- a dialog, a preset the user picked,
    //! a project being loaded -- goes through here, and any automation in effect is dropped.
    void setValue(float val);
    bool update(float val);

    //! Writes the live layer only, leaving the document untouched. MIDI CC, automations and
    //! anything else the transport generates must use this: it is what keeps a song that is
    //! playing from rewriting the patch it is playing.
    void setAutomationValue(float val);
    //! Puts the authored value back in effect. A no-op unless automation has written here.
    void clearAutomation();
    bool isAutomated() const;

    int xmlValue() const;
    //! The authored value in XML units. This, not xmlValue(), is what serialization writes.
    int xmlAuthoredValue() const;
    int xmlMin() const;
    int xmlMax() const;
    int xmlDefault() const;
    //! The default in internal units, for callers that need to restore it without authoring it.
    float defaultValue() const;
    int xmlScale() const;

    Type type() const;
    bool isDiscrete() const;
    bool isBoolean() const;

    const LegacyNameList & legacyNames() const;
    const LegacyValueConverter & legacyValueConverter() const;

    void setFromXml(int xmlVal, std::optional<int> xmlMin = std::nullopt, std::optional<int> xmlMax = std::nullopt);

    void reset();

    static float xmlValueToInternal(int xmlVal, int xmlMin, int xmlMax);
    static int internalToXmlValue(float value, int xmlMin, int xmlMax);

private:
    int xmlValueOf(float value) const;
    float clampToRange(float val) const;

    std::string m_name;
    float m_value = 0.0f;
    float m_authoredValue = 0.0f;
    bool m_automated = false;
    int m_xmlMin = 0;
    int m_xmlMax = 100;
    int m_xmlDefault = 0;
    int m_xmlScale = 1;
    Type m_type = Type::Continuous;
    LegacyNameList m_legacyNames;
    LegacyValueConverter m_legacyValueConverter;
};

} // namespace noteahead

#endif // PARAMETER_HPP
