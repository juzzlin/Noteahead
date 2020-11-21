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

#include <cstdlib>
#include <iostream>

#include "application.hpp"

int main(int argc, char ** argv)
{
    try {
        return cacophony::Application(argc, argv).run();
    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
