//
// Created by michiel on 24-1-2016.
//

#include "MockKernel.h"
#include <boost/lexical_cast.hpp>

void MockKernel::start_kernel(std::shared_ptr<PSTDFileConfiguration> config)
{
    _conf = config;
}

void MockKernel::run(KernelCallback *callback)
{
    if(!_conf)
        throw PSTDKernelNotConfiguredException();

    auto meta = get_metadata();

    callback->Callback(CALLBACKSTATUS::STARTING, "Starting to mock", -1);

    for (int i = 0; i < meta.Framecount; ++i)
    {
        callback->Callback(CALLBACKSTATUS::RUNNING, "Starting to mock", i);
        for (int j = 0; j < _conf->Domains.size(); ++j)
        {
            int type = j%1;
            if(type == 0)
            {
                callback->WriteFrame(i, boost::lexical_cast<std::string>(j), CreateRandomFrame(meta.DomainMetadata[j][0], meta.DomainMetadata[j][1]));
            }
        }
    }

    callback->Callback(CALLBACKSTATUS::FINISHED, "finished mocking", -1);
}

SimulationMetadata MockKernel::get_metadata()
{
    if(!_conf)
        throw PSTDKernelNotConfiguredException();

    SimulationMetadata result;

    float grid = _conf->Settings.GetGridSpacing();
    for (int i = 0; i < _conf->Domains.size(); ++i)
    {
        std::vector<int> d;
        d.push_back((int) (roundf(_conf->Domains[i].Size.x() / grid) * grid));
        d.push_back((int) (roundf(_conf->Domains[i].Size.y() / grid) * grid));
        d.push_back(1);

        result.DomainMetadata.push_back(d);
    }

    result.Framecount = (int) (_conf->Settings.GetRenderTime() / _conf->Settings.GetTimeStep());

    return result;
}

std::shared_ptr<std::vector<float>> MockKernel::CreateRandomFrame(int x, int y)
{
    std::shared_ptr<std::vector<float>> result = std::make_shared<std::vector<float>>();
    result->reserve((unsigned long) (x * y));
    for (int i = 0; i < x * y; ++i)
    {
        float v = rand()/((float)RAND_MAX);
        result->push_back(v);
    }
    return result;
}