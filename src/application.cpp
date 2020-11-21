// This file is part of Cacophony.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
//
// Heimer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Heimer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "application.hpp"
#include "config.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

namespace cacophony {

Application::Application(int & argc, char ** argv)
  : m_application(std::make_unique<QGuiApplication>(argc, argv))
  , m_engine(std::make_unique<QQmlApplicationEngine>())
  , m_config(std::make_unique<Config>())
{
    qmlRegisterType<Config>("Cacophony", 1, 0, "Config");
}

int Application::run()
{
    m_engine->rootContext()->setContextProperty("config", m_config.get());

    const QUrl mainUrl(QStringLiteral("qrc:/Main.qml"));
    m_engine->load(mainUrl);
    if (m_engine->rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    return m_application->exec();
}

Application::~Application() = default;

} // namespace cacophony
