########################################################################
# #
# This file is part of openPSTD.                                       #
# #
# openPSTD is free software: you can redistribute it and/or modify     #
# it under the terms of the GNU General Public License as published by #
# the Free Software Foundation, either version 3 of the License, or    #
# (at your option) any later version.                                  #
# #
# openPSTD is distributed in the hope that it will be useful,          #
# but WITHOUT ANY WARRANTY; without even the implied warranty of       #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        #
# GNU General Public License for more details.                         #
#                                                                      #
# You should have received a copy of the GNU General Public License    #
# along with openPSTD.  If not, see <http://www.gnu.org/licenses/>.    #
#                                                                      #
########################################################################

import struct
import concurrent
import multiprocessing as mp
from kernel.core.classes import *

try:
    import ConfigParser as configparser
except:
    import configparser

# Abbreviate different type of calculation directions and measures
h, v = BoundaryType.HORIZONTAL, BoundaryType.VERTICAL
P, V = CalculationType.PRESSURE, CalculationType.VELOCITY


def update_domain_values(domain, cfg, sub_frame):
    domain.update_values(cfg, sub_frame)  # Note that cfg can be generalized to the solver.


def calc_domain(domain):
    for boundary_type, calculation_type in [(h, P), (v, P), (h, V), (v, V)]:
        if not domain.is_rigid():
            # Calculate sound propagations for non-rigid domains
            if domain.should_update(boundary_type):
                domain.calc(boundary_type, calculation_type)


def update_domain(domain, sub_frame):
    domain.rk_update(sub_frame)


class SingleThreaded:
    def __init__(self, cfg, scene, data_writer, receiver_files, output_fn):
        # Loop over time steps
        for frame in range(int(cfg.TRK)):
            output_fn({'status': 'running', 'message': "Calculation frame:%d" % (frame + 1), 'frame': frame + 1})

            # Keep a reference to current matrix contents
            for domain in scene.domains: domain.push_values()

            # Loop over subframes
            for sub_frame in range(6):
                # Loop over calculation directions and measures
                for domain in scene.domains:
                    calc_domain(domain)
                # Update acoustic values
                for domain in scene.domains:
                    if not domain.is_rigid():
                        update_domain(domain, sub_frame)

                # Sum the pressure components
                for domain in scene.domains:
                    domain.field_dict['p0'] = domain.field_dict['px0'] + domain.field_dict['pz0']

            # Apply pml matrices to boundary domains
            scene.apply_pml_matrices()

            for rf, r in zip(receiver_files, scene.receivers):
                rf.write(struct.pack('f', r.calc()))
                rf.flush()

            if frame % cfg.save_nth_frame == 0:
                data_writer.write_to_file(frame)


class MultiThreaded:
    def __init__(self, cfg, scene, data_writer, receiver_files, output_fn):
        # Loop over time steps
        executor = concurrent.futures.ThreadPoolExecutor(4)
        for frame in range(int(cfg.TRK)):
            output_fn({'status': 'running', 'message': "Calculation frame:%d" % (frame + 1), 'frame': frame + 1})

            # Keep a reference to current matrix contents
            for domain in scene.domains:
                domain.push_values()

            # Loop over sub-frames
            for sub_frame in range(2):
                job_list = []
                # Loop over domains
                for domain in scene.domains:
                    job_list.append(executor.submit(calc_domain, domain))
                [job.result() for job in job_list]
                job_list = []
                for domain in scene.doains:
                    if not domain.is_rigid():
                        job_list.append(executor.submit(update_domain, domain, sub_frame))
                [job.result() for job in job_list]

                # Sum the pressure components
                for domain in scene.domains:
                    domain.field_dict['p0'] = domain.field_dict['px0'] + domain.field_dict['pz0']

            # Apply pml matrices to boundary domains
            scene.apply_pml_matrices()

            for rf, r in zip(receiver_files, scene.receivers):
                rf.write(struct.pack('f', r.calc()))
                rf.flush()

            if frame % cfg.save_nth_frame == 0:
                data_writer.write_to_file(frame)
        executor.shutdown()

    def _distribute_domains(self,scene,num_workers):
        self.domain_solver_list = []
        for i in num_workers:
            self.domain_solver_list[i] = DomainWorker()
        worker = 0
        for domain in scene.domains:
            # if not domain.is_rigid()
            self.domain_solver_list[worker].add_domain(domain)
            worker += 1
            worker %= num_workers


class DomainWorker(mp.Process):
    def __init__(self):
        super()
        self.domain_list = []

    def add_domain(self,domain: Domain):
        assert domain not in self.domain_list
        self.domain_list.append(domain)

    def get_dependencies(self):
        return [neighbour.id for domain in self.domain_list for neighbour in domain.neighbour_dict.values()]

    def calc_domains(self):
        for domain in self.domain_list:
            for boundary_type, calculation_type in [(h, P), (v, P), (h, V), (v, V)]:
                if not domain.is_rigid():  # should be checked earlier
                    if domain.should_update(boundary_type):
                        source = "L%s%s" % calculation_type,boundary_type
                        domain.dom_obj[source] = domain.calc(boundary_type, calculation_type)

    def update_domains(self,sub_frame):
        for domain in self.domain_list:
            domain.rk_update(sub_frame)

    def get_numerical_data_dict(self):
        return {domain.id:domain.dom_obj for domain in self.domain_list}

    def set_numerical_data_dict(self,data: dict):
        assert data.keys() == set(self.get_dependencies()) # move to unit testing
        for key in data:
            pass
