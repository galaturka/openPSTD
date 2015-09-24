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
// Authors: 0mar
//
//
// Purpose:
//
//
//////////////////////////////////////////////////////////////////////////
#ifndef OPENPSTD_SPEAKER_H
#define OPENPSTD_SPEAKER_H
#include <string>
#include <memory>
#include <vector>
#include <eigen/Eigen/Dense>

namespace Kernel {
    class Speaker {
    public:
        const double x;
        const double y;
        const double z;
        std::vector<double> location;

        Speaker(const double x, const double y, const double z);

        // ~Speaker();

        Eigen::ArrayXXf EigencomputeDomainContribution(std::shared_ptr<Domain> domain);

    };
}
#endif //OPENPSTD_SPEAKER_H