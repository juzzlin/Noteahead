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

#include <QGuiApplication>
#include <QQmlApplicationEngine>

Application::Application(int & argc, char ** argv)
  : m_app(std::make_unique<QGuiApplication>(argc, argv))
  , m_engine(std::make_unique<QQmlApplicationEngine>())
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
      m_engine.get(), &QQmlApplicationEngine::objectCreated,
      m_app.get(), [url](QObject * obj, const QUrl & objUrl) {
          if (!obj && url == objUrl)
              QCoreApplication::exit(EXIT_FAILURE);
      },
      Qt::QueuedConnection);
    m_engine->load(url);
}

int Application::run()
{
    return m_app->exec();
}

Application::~Application() = default;
