// This file is part of Cacophony.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include <cstdlib>
#include <iostream>

#include <QCoreApplication>
#include <QSettings>

#include "application.hpp"
#include "constants.hpp"

int main(int argc, char ** argv)
{
    QCoreApplication::setOrganizationName(cacophony::constants::application::QSETTINGS_COMPANY_NAME);
    QCoreApplication::setApplicationName(cacophony::constants::application::QSETTINGS_SOFTWARE_NAME);
#ifdef Q_OS_WIN32
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    try {
        return cacophony::Application(argc, argv).run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
