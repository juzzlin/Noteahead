// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#include "ui_logger.hpp"

#include "../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

UiLogger::UiLogger(QObject * parent)
  : QObject { parent }
{
}

void UiLogger::trace(QString tag, QString message) const
{
    juzzlin::L(tag.toStdString()).trace() << message.toStdString();
}

void UiLogger::debug(QString tag, QString message) const
{
    juzzlin::L(tag.toStdString()).debug() << message.toStdString();
}

void UiLogger::info(QString tag, QString message) const
{
    juzzlin::L(tag.toStdString()).info() << message.toStdString();
}

void UiLogger::warning(QString tag, QString message) const
{
    juzzlin::L(tag.toStdString()).warning() << message.toStdString();
}

void UiLogger::error(QString tag, QString message) const
{
    juzzlin::L(tag.toStdString()).error() << message.toStdString();
}

} // namespace noteahead
