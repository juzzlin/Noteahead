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

#include <string>

namespace noteahead {

class Parameter
{
public:
    Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault, int xmlScale = 1);

    const std::string & name() const;

    float value() const;
    void setValue(float val);

    int xmlValue() const;
    int xmlMin() const;
    int xmlMax() const;
    int xmlDefault() const;
    int xmlScale() const;

    void setFromXml(int xmlVal);

    void reset();

private:
    std::string m_name;
    float m_value { 0.0f };
    int m_xmlMin { 0 };
    int m_xmlMax { 100 };
    int m_xmlDefault { 0 };
    int m_xmlScale { 1 };
};

} // namespace noteahead

#endif // PARAMETER_HPP
