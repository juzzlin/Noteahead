// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#include "application/application.hpp"
#include "common/constants.hpp"
#include "simple_logger.hpp"

#include <cstdlib>
#include <iostream>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

static const auto TAG = "main";

static size_t tsMs()
{
    using namespace std::chrono;
    return static_cast<size_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

static void initLogger()
{
    using juzzlin::L;

    const QString logPath { QDir::tempPath() + QDir::separator() + noteahead::Constants::applicationName() + "-" + QString::number(tsMs()) + ".log" };
    L::initialize(logPath.toStdString());
    L::enableEchoMode(true);
    L::setTimestampMode(L::TimestampMode::ISODateTimeMilliseconds);
    L::setTimestampSeparator(" ");
    const std::map<L::Level, std::string> symbols = {
        { L::Level::Debug, "D" },
        { L::Level::Error, "E" },
        { L::Level::Fatal, "F" },
        { L::Level::Info, "I" },
        { L::Level::Trace, "T" },
        { L::Level::Warning, "W" }
    };
    for (auto && symbol : symbols) {
        L::setLevelSymbol(symbol.first, "[" + symbol.second + "]");
    }

#if defined(NDEBUG) or defined(QT_NO_DEBUG)
    L::setLoggingLevel(L::Level::Info);
#else
    L::setLoggingLevel(L::Level::Debug);
#endif

    L(TAG).info() << noteahead::Constants::applicationName().toStdString() << " version " << noteahead::Constants::applicationVersion().toStdString();
    L(TAG).info() << "Licensed under " << noteahead::Constants::license().toStdString();
    L(TAG).info() << noteahead::Constants::copyright().toStdString();
    L(TAG).info() << "Compiled against Qt version " << QT_VERSION_STR;
}

int main(int argc, char ** argv)
{
    QCoreApplication::setOrganizationName(noteahead::Constants::qSettingsCompanyName());
    QCoreApplication::setApplicationName(noteahead::Constants::qSettingSoftwareName());
#ifdef Q_OS_WIN32
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    try {
        initLogger();
        return noteahead::Application(argc, argv).run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
