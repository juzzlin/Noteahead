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

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>

class QGuiApplication;
class QQmlApplicationEngine;

class Application
{
public:
    Application(int & argc, char ** argv);

    ~Application();

    int run();

private:
    std::unique_ptr<QGuiApplication> m_app;

    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

#endif // APPLICATION_HPP
