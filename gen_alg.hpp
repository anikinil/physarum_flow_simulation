#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 30;
const int NUM_TRIES = 1;

const int NUM_STEPS = 400;

const float ELITE_PROPORTION = 0.4f;
const float CROSSED_PROPORTION = 0.1f;

const double DEFAULT_MUTATION_RATE = 0.2;
const double MUTATION_STRENGTH = 0.5;

const double INITIAL_ENERGY = 10.0;

const int NUM_JUNCTIONS = 800;

const double MAX_DIST_FROM_ORIG = 100.0;

vector<unique_ptr<Junction>> initializeJunctions() {
    // junctions
    vector<unique_ptr<Junction>> junctions;
    for (int i = 0; i < NUM_JUNCTIONS; ++i) {
        double x = Random::uniform(-MAX_DIST_FROM_ORIG, MAX_DIST_FROM_ORIG);
        double y = Random::uniform(-MAX_DIST_FROM_ORIG, MAX_DIST_FROM_ORIG);
        double energy = INITIAL_ENERGY;
        double radius = 0.5 * TUBE_LENGTH;
        junctions.push_back(make_unique<Junction>(Junction{x, y, energy}));
    }
    return junctions;
}

vector<unique_ptr<Tube>> initializeTubes(vector<unique_ptr<Junction>> &junctions) {
    vector<unique_ptr<Tube>> tubes;
    for (size_t i = 0; i < junctions.size(); ++i) {
        for (size_t j = i + 1; j < junctions.size(); ++j) {
            double dx = junctions[i]->x - junctions[j]->x;
            double dy = junctions[i]->y - junctions[j]->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist <= TUBE_LENGTH && junctions[i]->getTotalTubes() < MAX_TUBES_PER_JUNCTION && junctions[j]->getTotalTubes() < MAX_TUBES_PER_JUNCTION) {
                auto newTube = make_unique<Tube>(Tube{
                    junctions[i]->x,
                    junctions[i]->y,
                    junctions[j]->x,
                    junctions[j]->y,
                    DEFAULT_FLOW_RATE,
                    false,
                    junctions[i].get(),
                    junctions[j].get()}
                );
                double angle = atan2(dy, dx);
                junctions[i]->outTubes.push_back({newTube.get(), angle});
                junctions[j]->inTubes.push_back({newTube.get(), angle + M_PI});
                tubes.push_back(std::move(newTube));
            }
        }
    }
    return tubes;
}