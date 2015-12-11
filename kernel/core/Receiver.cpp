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
// Authors: Omar Richardson
//
//////////////////////////////////////////////////////////////////////////

#include "Receiver.h"

namespace Kernel {
    Receiver::Receiver(std::vector<float> location, std::shared_ptr<PSTDFileSettings> config, int id,
                       std::shared_ptr<Domain> container) : x(location.at(0)), y(location.at(1)), z(location.at(2)) {
        this->config = config;
        this->location = location;
        this->container_domain = container;
        this->grid_location = std::make_shared<Point>(Point((int) this->x, (int) this->y, (int) this->z));
        for (int i = 0; i < this->location.size(); i++) {
            this->grid_offset.push_back(this->location.at(i) = this->grid_location->array.at(i));
        }
        this->id = id;
    }

    float Receiver::compute_local_pressure() {
        float pressure;
        if (this->config->GetSpectralInterpolation()) {
            pressure = this->compute_with_si();
        } else {
            pressure = this->compute_with_nn();
        }
        this->received_values.push_back(pressure);
        return pressure;
    }

    std::shared_ptr<Eigen::ArrayXcf> Receiver::get_fft_factors(Point size, CalcDirection bt) {
        int primary_dimension = 0;
        if (bt == CalcDirection::X) {
            primary_dimension = size.x;
        } else {
            primary_dimension = size.y;
        }
        float dx = this->config->GetGridSpacing();
        int wave_length_number = (int) (2 * this->config->GetWaveLength() + primary_dimension + 1);
        //Pressure grid is staggered, hence + 1
        WaveNumberDiscretizer::Discretization discr = this->container_domain->wnd->get_discretization(dx,
                                                                                                      wave_length_number);
        float offset = this->grid_offset.at(static_cast<unsigned long>(bt));
        Eigen::ArrayXcf fft_factors(discr.wave_numbers->rows());
        for (int i = 0; i < discr.wave_numbers->rows(); i++) {
            std::complex<float> wave_number = (*discr.wave_numbers)(i);
            std::complex<float> complex_factor = (*discr.complex_factors)(i);
            fft_factors(i) = exp(offset * dx * wave_number * complex_factor);
        }
        return std::make_shared<Eigen::ArrayXcf>(fft_factors);
    }

    float Receiver::compute_with_nn() {
        Point rel_location =
                *(this->grid_location) - *(this->container_domain->top_left);
        float nn_value = this->container_domain->current_values->p0(rel_location.x, rel_location.y);
        return nn_value;
    }

    float Receiver::compute_with_si() {
        std::shared_ptr<Domain> top_domain = this->container_domain->get_neighbour_at(Direction::TOP, this->location);
        std::shared_ptr<Domain> bottom_domain = this->container_domain->get_neighbour_at(Direction::BOTTOM,
                                                                                         this->location);
        std::shared_ptr<Eigen::ArrayXXf> p0dx = this->calc_domain_fields(this->container_domain,
                                                                             CalcDirection::X);
        int rel_x_point = this->grid_location->x - this->container_domain->top_left->x;
        std::shared_ptr<Eigen::ArrayXXf> p0dx_slice = std::make_shared<Eigen::ArrayXXf>(
                p0dx->middleCols(rel_x_point, 1));

        std::shared_ptr<Eigen::ArrayXXf> p0dx_top = this->calc_domain_fields(top_domain, CalcDirection::X);
        int top_rel_x_point = this->grid_location->x - top_domain->top_left->x;
        std::shared_ptr<Eigen::ArrayXXf> p0dx_top_slice = std::make_shared<Eigen::ArrayXXf>(
                p0dx_top->middleCols(top_rel_x_point, 1));

        std::shared_ptr<Eigen::ArrayXXf> p0dx_bottom = this->calc_domain_fields(bottom_domain,
                                                                                    CalcDirection::X);
        int bottom_rel_x_point = this->grid_location->x - bottom_domain->top_left->x;
        std::shared_ptr<Eigen::ArrayXXf> p0dx_bottom_slice = std::make_shared<Eigen::ArrayXXf>(
                p0dx_bottom->middleCols(bottom_rel_x_point, 1));

        std::shared_ptr<Eigen::ArrayXcf> z_fact = this->get_fft_factors(Point(1, this->container_domain->size->y),
                                                       CalcDirection::Y);
        float wave_number = 2 * this->config->GetWaveLength() + this->container_domain->size->y + 1;
        int opt_wave_number = next2Power(wave_number);

        Eigen::Array<float, 4, 2> rho; //TODO: Needs the rho matrix of the bottom and top domains

        Eigen::ArrayXXf p0shift = spatderp3(p0dx_bottom_slice, p0dx_slice, p0dx_top_slice,
                                            z_fact, rho,config->GetWindow(), config->GetWindowSize(),
                                            CalculationType::PRESSURE , CalcDirection::Y);

        int rel_y_point = this->grid_location->y - this->container_domain->top_left->y;
        float si_value = p0shift(rel_y_point, 0);
        return si_value;
    }

    // Todo: Different name;
    std::shared_ptr<Eigen::ArrayXXf> Receiver::calc_domain_fields(std::shared_ptr<Domain> domain, CalcDirection bt) {
        Eigen::ArrayXXf domain_result = domain->calc(bt, CalculationType::PRESSURE,
                                                     this->get_fft_factors(*(domain->size), bt));
        return std::make_shared<Eigen::ArrayXXf>(domain_result);
    }
}