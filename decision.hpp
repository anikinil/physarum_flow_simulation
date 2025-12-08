#include <vector>
#include "genome.hpp"
#include "FNN.hpp"

using namespace std;

struct FlowDecisionNet {

    Genome genome;
    FNN net;

    double increaseFlowProb = 0.0;
    double decreaseFlowProb = 0.0;

    FlowDecisionNet(const Genome &genome)
    {

        auto net = FNN();
        net.initialize(genome.getFlowNetWeights());
        this->net = net;
    }

    void decideAction(double currentFlowRate,
                      double inJunctionAverageFlowRate,
                      double outJunctionAverageFlowRate,
                      double averageAngleInTubes,
                      double averageAngleOutTubes,
                      double fromJunctionEnergy,
                      double toJunctionEnergy,
                      double hitBySun)
    {

        vector<double> input = {
            currentFlowRate,
            inJunctionAverageFlowRate,
            outJunctionAverageFlowRate,
            averageAngleInTubes,
            averageAngleOutTubes,
            fromJunctionEnergy,
            toJunctionEnergy,
            hitBySun
        };

        vector<double> pred = net.predict(input);
        increaseFlowProb = pred[0];
        decreaseFlowProb = pred[1];
    }
};
