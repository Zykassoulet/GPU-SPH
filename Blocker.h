#pragma once

#include "SimulatorComputeStage.h"

class Blocker : protected SimulatorComputeStage {
public:
private:
    virtual void createPipelines() final;
    virtual void createDescriptorPool() final;
    virtual void createDescriptorSets() final;
};




