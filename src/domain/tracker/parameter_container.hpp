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

#ifndef PARAMETER_CONTAINER_HPP
#define PARAMETER_CONTAINER_HPP

#include "parameter.hpp"

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class ParameterContainer
{
public:
    using ParameterOpt = std::optional<std::reference_wrapper<Parameter>>;
    using ConstParameterOpt = std::optional<std::reference_wrapper<const Parameter>>;

    ParameterContainer() = default;
    virtual ~ParameterContainer();

    ParameterContainer(const ParameterContainer &) = default;
    ParameterContainer & operator=(const ParameterContainer &) = default;
    ParameterContainer(ParameterContainer &&) = default;
    ParameterContainer & operator=(ParameterContainer &&) = default;

    void addParameter(Parameter parameter);

    ParameterOpt parameter(const std::string & name);
    ConstParameterOpt parameter(const std::string & name) const;

    std::map<std::string, Parameter> & parameters();
    const std::map<std::string, Parameter> & parameters() const;

    virtual void reset();

    virtual void serializeParametersToXml(ProjectWriter & writer) const;
    virtual void deserializeParametersFromXml(ProjectReader & reader);
    void deserializeParameter(ProjectReader & reader);

private:
    std::map<std::string, Parameter> m_parameters;
    std::map<std::string, std::string> m_legacyNameMap;
};

} // namespace noteahead

#endif // PARAMETER_CONTAINER_HPP
