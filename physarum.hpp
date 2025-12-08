#include <vector>
#include <deque>
#include <memory>
#include <numeric>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>
#include <cmath>

using namespace std;

#include "decision.hpp"

const int MAX_TUBES_PER_JUNCTION = 7;
const double TUBE_LENGTH = 12.0;

const double DEFAULT_FLOW_RATE = 1.0;
const double FLOW_RATE_CHANGE_STEP = 0.1;
const double MAX_TUBE_FLOW_RATE = 2.0;

// const double MAX_JUNCTION_ENERGY = 20.0;

const double DRYING_RATE = 0.99;

struct Junction;
struct Tube;

struct Tube
{
    const double x1;
    const double y1;
    const double x2;
    const double y2;

    double flowRate;

    bool reversed = false;

    Junction *fromJunction;
    Junction *toJunction;
};

struct Junction
{
    const double x;
    const double y;
    double energy;
    bool sun;

    struct TubeInfo
    {
        Tube *tube;
        double angle;
    };
    vector<TubeInfo> inTubes;
    vector<TubeInfo> outTubes;

    int numInTubes()
    {
        return inTubes.size();
    }

    int numOutTubes()
    {
        return outTubes.size();
    }

    double averageInFlowRate()
    {
        if (inTubes.empty())
            return 0.0;
        double sum = accumulate(inTubes.begin(), inTubes.end(), 0.0,
                                [](double acc, const TubeInfo &ti)
                                { return acc + ti.tube->flowRate; });
        return sum / inTubes.size();
    }

    double averageOutFlowRate()
    {
        if (outTubes.empty())
            return 0.0;
        double sum = accumulate(outTubes.begin(), outTubes.end(), 0.0,
                                [](double acc, const TubeInfo &ti)
                                { return acc + ti.tube->flowRate; });
        return sum / outTubes.size();
    }

    double averageAngleInTubes()
    {
        if (inTubes.empty())
            return Random::uniform(0.0, 2.0 * M_PI);
        double sum = accumulate(inTubes.begin(), inTubes.end(), 0.0,
                                [](double acc, const TubeInfo &ti)
                                { return acc + ti.angle; });
        return sum / inTubes.size();
    }

    double averageAngleOutTubes()
    {
        if (outTubes.empty())
            return Random::uniform(0.0, 2.0 * M_PI);
        double sum = accumulate(outTubes.begin(), outTubes.end(), 0.0,
                                [](double acc, const TubeInfo &ti)
                                { return acc + ti.angle; });
        return sum / outTubes.size();
    }

    double getSummedFlowRate()
    {
        double totalFlow = 0.0;
        for (const auto &t : inTubes)
        {
            totalFlow += t.tube->flowRate;
        }
        for (const auto &t : outTubes)
        {
            totalFlow -= t.tube->flowRate;
        }
        return totalFlow;
    }

    // moves tube from inTubes to outTubes or vice versa
    void switchTubeDirection(Tube &tube)
    {
        auto it_in = std::find_if(inTubes.begin(), inTubes.end(),
                                  [&tube](const TubeInfo &ti)
                                  { return ti.tube == &tube; });
        if (it_in != inTubes.end())
        {
            outTubes.push_back(*it_in);
            inTubes.erase(it_in);
            return;
        }
        auto it_out = std::find_if(outTubes.begin(), outTubes.end(),
                                   [&tube](const TubeInfo &ti)
                                   { return ti.tube == &tube; });
        if (it_out != outTubes.end())
        {
            inTubes.push_back(*it_out);
            outTubes.erase(it_out);
            return;
        }
    }

    int getTotalTubes()
    {
        return inTubes.size() + outTubes.size();
    }
};

struct World
{
    Genome genome;

    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;

    FlowDecisionNet flowDecisionNet;

    bool sun = false;

    double fitness = 0.0;

    World(const Genome &g)
        : genome(g),
          flowDecisionNet(g) {}

    Genome getGenome() const
    {
        return genome;
    }

    void mutateGenome(double mutation_rate, double mutation_strength)
    {
        genome.mutate(mutation_rate, mutation_strength);
    }

    void placeNewJunctions(vector<unique_ptr<Junction>> &&newJunctions)
    {
        for (auto &junc : newJunctions)
        {
            junctions.push_back(std::move(junc));
        }
    }

    void placeNewTubes(vector<unique_ptr<Tube>> &&newTubes)
    {
        for (auto &tube : newTubes)
        {
            tubes.push_back(std::move(tube));
        }
    }

    void run(int steps = 100, bool save = false)
    {
        if (save)
        {
            namespace fs = std::filesystem;
            fs::path p("data/animation_frames.csv");
            std::error_code ec;
            if (p.has_parent_path())
            {
                fs::create_directories(p.parent_path(), ec);
            }
            if (fs::exists(p, ec))
            {
                auto sz = fs::file_size(p, ec);
                if (!ec && sz > 0)
                {
                    std::ofstream ofs(p, std::ios::trunc);
                }
            }
            else
            {
                std::ofstream ofs(p);
            }
        }

        if (save)
        {
            std::ofstream file("data/animation_frames.csv", std::ios::app);
            file << "step,"
                 << "fitness,"
                 << "j_x,j_y,j_energy,sun,";
            file
                << "t_x1,t_y1,t_x2,t_y2,t_flow_rate,reversed\n";
        }

        if (save)
        {
            this->saveFrame(0);
        }
        for (int step = 1; step <= steps; ++step)
        {
            if (step % 100 == 1)
            {
                sun = !sun;
            }
            this->step();
            if (save)
            {
                this->saveFrame(step);
            }
        }
    }

    void saveFrame(int step) {
        std::ofstream file("data/animation_frames.csv", std::ios::app);

        for (const auto &junc : junctions) {
            if (junc->energy < 1e-6)
                junc->energy = 0.0;
            file << step << ','
                 << fitness << ','
                 << junc->x << ','
                 << junc->y << ','
                 << junc->energy << ','
                 << (junc->sun ? 1 : 0) << ',';
            file << ",,,,,,\n";
        }
        for (const auto &tube : tubes) {
            file << step << ','
                 << fitness << ",,,,,";
            file << tube->x1 << ',' << tube->y1 << ','
                 << tube->x2 << ',' << tube->y2 << ','
                 << tube->flowRate << ','
                 << (tube->reversed ? 1 : 0) << "\n";
        }
    }

    void step() {
        updateFlow();
        updateEnergy();
        updateFitness();
    }

    void updateFlow() {

        for (auto &tube : tubes) {

            // let flow rate decision net decide on flow rate changes
            double currFlowRate = tube->flowRate;
            double inJunctionAverageFlowRate = static_cast<double>(tube->fromJunction->getSummedFlowRate());
            double outJunctionAverageFlowRate = static_cast<double>(tube->toJunction->getSummedFlowRate());
            double averageAngleInTubes = tube->fromJunction->averageAngleInTubes();
            double averageAngleOutTubes = tube->toJunction->averageAngleOutTubes();
            double fromJunctionEnergy = tube->fromJunction->energy;
            double toJunctionEnergy = tube->toJunction->energy;
            double hitBySun = tube->fromJunction->sun ? 1.0 : 0.0;

            flowDecisionNet.decideAction(currFlowRate,
                                         inJunctionAverageFlowRate,
                                         outJunctionAverageFlowRate,
                                         averageAngleInTubes,
                                         averageAngleOutTubes,
                                         fromJunctionEnergy,
                                         toJunctionEnergy,
                                         hitBySun);

            // adjust flow rate based on decision net
            if (Random::uniform() < flowDecisionNet.increaseFlowProb) {
                tube->flowRate += FLOW_RATE_CHANGE_STEP;
                tube->flowRate = min(tube->flowRate, MAX_TUBE_FLOW_RATE);
            }
            if (Random::uniform() < flowDecisionNet.decreaseFlowProb && tube->flowRate > 0) {
                tube->flowRate -= FLOW_RATE_CHANGE_STEP;
            }

            // rearrange tube directube direction if flow rate changes to negative
            if (tube->flowRate < 0) {
                std::swap(tube->fromJunction, tube->toJunction);
                tube->flowRate = -tube->flowRate;
                tube->fromJunction->switchTubeDirection(*tube);
                tube->toJunction->switchTubeDirection(*tube);
                tube->reversed = !tube->reversed;
            }
            tube->flowRate = min(tube->flowRate, tube->fromJunction->energy); // limit by available energy
        }
    }

    void updateEnergy() {
        for (auto &junc : junctions) {
            for (auto &tubeInfo : junc->outTubes) {

                if (tubeInfo.tube->flowRate > junc->energy) {
                    tubeInfo.tube->flowRate = junc->energy;
                }
                junc->energy -= tubeInfo.tube->flowRate;
                tubeInfo.tube->toJunction->energy += tubeInfo.tube->flowRate;
            }

            if (junc->energy < 0.0) {
                junc->energy = 0.0;
            }
            if (sun) {
                if (junc->x > 0) {
                    junc->sun = true;
                    junc->energy *= DRYING_RATE;
                } else {
                    junc->sun = false;
                    // junc->energy += 0.05;
                }
            } else {
                if (junc->x <= 0) {
                    junc->sun = true;
                    junc->energy *= DRYING_RATE;
                } else {
                    junc->sun = false;
                    // junc->energy += 0.05;
                }
            }
        }
    }

    void updateFitness() {
        calculateFitness();
    }

    void calculateFitness() {
        // double energy_right = 0.0;
        // for (const auto &junc : junctions) {
        //     energy_right += junc->x * junc->energy;
        // }
        // fitness = energy_right;

        double totalEnergy = 0.0;
        for (const auto &junc : junctions)
        {
            totalEnergy += junc->energy;
        }
        fitness = totalEnergy;
    }
};