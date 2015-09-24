//////////////////////////////////////////////////////////////////////////
// This file is part of openPSTD.                                       //
//                                                                      //
// openPSTD is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU General Public License as published by //
// the Free Software Foundation, either version 3 of the License, or    //
// (at your option) any later version.                                  //
//                                                                      //
// openPSTD is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of       //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        //
// GNU General Public License for more details.                         //
//                                                                      //
// You should have received a copy of the GNU General Public License    //
// along with openPSTD.  If not, see <http://www.gnu.org/licenses/>.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
// Date: 17-9-15
//
//
// Authors: omar
//
//
// Purpose:
//
//
//////////////////////////////////////////////////////////////////////////
#ifndef OPENPSTD_GEOMETRY_H
#define OPENPSTD_GEOMETRY_H

#include <vector>
namespace Kernel {
    class Point {
    public:
        const int x, y, z;
        std::vector<int> array;

        /**
         * Constructor for a point in grid coordinates
         */
        Point(int x, int y, int z);

        friend Point operator+(Point a, Point b);

        friend Point operator-(Point a, Point b);
        //Todo (0mar): Add string override method
    };
}
#endif //OPENPSTD_GEOMETRY_H
