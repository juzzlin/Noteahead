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

#include "plugin_catalog.hpp"

#include "../controllers/device_dialog_model.hpp"

#include <algorithm>

namespace noteahead {

namespace {

std::vector<PluginCatalog::Effect> & effectList()
{
    static std::vector<PluginCatalog::Effect> list;
    return list;
}

std::vector<PluginCatalog::Device> & deviceList()
{
    static std::vector<PluginCatalog::Device> list;
    return list;
}

} // namespace

void PluginCatalog::ensureRegistered()
{
    // On first use rather than from main(), so that every test sees the same catalog the
    // application does without having to set it up
    static const bool registered = [] {
        registerEffectPlugins();
        registerDevicePlugins();
        return true;
    }();
    (void)registered;
}

void PluginCatalog::addEffect(Effect effect)
{
    effectList().push_back(std::move(effect));
}

void PluginCatalog::addDevice(Device device)
{
    deviceList().push_back(std::move(device));
}

const std::vector<PluginCatalog::Effect> & PluginCatalog::effects()
{
    ensureRegistered();
    return effectList();
}

const std::vector<PluginCatalog::Device> & PluginCatalog::devices()
{
    ensureRegistered();
    return deviceList();
}

std::optional<PluginCatalog::Effect> PluginCatalog::effectByType(const QString & type)
{
    const auto & list = effects();
    if (const auto it = std::ranges::find_if(list, [&type](auto && effect) { return effect.type == type; }); it != list.end()) {
        return *it;
    }
    return {};
}

std::optional<PluginCatalog::Device> PluginCatalog::deviceByTypeId(const std::string & typeId)
{
    const auto & list = devices();
    if (const auto it = std::ranges::find_if(list, [&typeId](auto && device) { return device.typeId == typeId; }); it != list.end()) {
        return *it;
    }
    return {};
}

} // namespace noteahead
