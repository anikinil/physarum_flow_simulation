#include "gen_alg.hpp"

#include <vector>
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>

#include <fstream>

#include <filesystem>
#include <regex>

#include <chrono>
#include <algorithm>

using namespace std;


vector<unique_ptr<World>> generateInitialPopulation(const Genome* initialGenome = nullptr) {
    vector<unique_ptr<World>> population;

    int pop_size = POPULATION_SIZE;

    if (initialGenome != nullptr) {
        pop_size -= 1;
        vector<unique_ptr<Junction>> junctions = initializeJunctions();
        vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
        auto world = make_unique<World>(*initialGenome);
        world->placeNewJunctions(std::move(junctions));
        world->placeNewTubes(std::move(tubes));
        population.push_back(std::move(world));
    }

    for (int i = 0; i < pop_size; i++) {

        vector<unique_ptr<Junction>> junctions = initializeJunctions();
        vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
        auto world = make_unique<World>(Genome());
        world->placeNewJunctions(std::move(junctions));
        world->placeNewTubes(std::move(tubes));
        population.push_back(std::move(world));
    }

    return population;
}

void saveGenomeAndFitness(const Genome& genome, double fitness, double averageFitness, int generation) {
    std::ofstream file("data/genome_fitness.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;best_fitness;average_fitness;genome\n";

    file << generation << ";" << fitness << ";" << averageFitness << ";";

    // Serialize genome
    for (const auto& weight : genome.serialize()) {
        file << weight << " ";
    }
    file << "\n";
}

void sortByFitness(vector<unique_ptr<World>>& population) {
    std::sort(population.begin(), population.end(),
    [](const unique_ptr<World>& a, const unique_ptr<World>& b) {
        return a->fitness > b->fitness;
    });
}

void crossOverGenomes(const Genome& parent1, const Genome& parent2, Genome& child) {
    vector<double> p1_weights = parent1.serialize();
    vector<double> p2_weights = parent2.serialize();

    vector<double> child_weights;
    child_weights.reserve(p1_weights.size());

    // Single point crossover
    int crossover_point = Random::randint(0, p1_weights.size() - 1);
    for (int i = 0; i < p1_weights.size(); i++) {
        if (i < crossover_point) {
            child_weights.push_back(p1_weights[i]);
        } else {
            child_weights.push_back(p2_weights[i]);
        }
    }
}

vector<unique_ptr<World>> createNextGeneration(vector<unique_ptr<World>>& currentPopulation) {
    
    vector<unique_ptr<World>> nextGeneration;

    // Select elite individuals

    // Copy elites
    int numElite = POPULATION_SIZE * ELITE_PROPORTION;
    for (int i = 0; i < numElite; i++) {
        vector<unique_ptr<Junction>> junctions = initializeJunctions();
        vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
        nextGeneration.push_back(make_unique<World>(currentPopulation[i]->getGenome()));
        nextGeneration.back()->placeNewJunctions(std::move(junctions));
        nextGeneration.back()->placeNewTubes(std::move(tubes));
    }

    int numCrossed = POPULATION_SIZE * CROSSED_PROPORTION;
    // Generate offspring through crossover and mutation
    while (nextGeneration.size() < numElite + numCrossed) {
        int parent1Idx = Random::randint(0, numElite - 1);
        int parent2Idx = Random::randint(0, numElite - 1);
        Genome childGenome;
        crossOverGenomes(currentPopulation[parent1Idx]->getGenome(),
                         currentPopulation[parent2Idx]->getGenome(),
                         childGenome);

        // Mutate child genome
        childGenome.mutate(DEFAULT_MUTATION_RATE, MUTATION_STRENGTH);

        // Create new World with child genome
        vector<unique_ptr<Junction>> junctions = initializeJunctions();
        vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
        auto childWorld = make_unique<World>(childGenome);
        childWorld->placeNewJunctions(std::move(junctions));
        childWorld->placeNewTubes(std::move(tubes));
        nextGeneration.push_back(std::move(childWorld));
    }

    while (nextGeneration.size() < POPULATION_SIZE) {
        // Fill the rest of the population with mutated copies of elites
        int eliteIdx = Random::randint(0, numElite - 1);
        Genome mutatedGenome = currentPopulation[eliteIdx]->getGenome();
        mutatedGenome.mutate(DEFAULT_MUTATION_RATE, MUTATION_STRENGTH);
        vector<unique_ptr<Junction>> junctions = initializeJunctions();
        vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
        nextGeneration.push_back(make_unique<World>(mutatedGenome));
        nextGeneration.back()->placeNewJunctions(std::move(junctions));
        nextGeneration.back()->placeNewTubes(std::move(tubes));
    }

    return nextGeneration;
}

void printETA(int currentGeneration, const vector<chrono::duration<double>>& gen_durations) {
    if (gen_durations.empty()) return;
    double avg_gen_time = std::accumulate(gen_durations.begin(), gen_durations.end(), 0.0, [](double sum, const auto& d) { return sum + d.count(); }) / gen_durations.size();
    double remaining_seconds = (NUM_GENERATIONS - currentGeneration - 1) * avg_gen_time;
    long long remaining = static_cast<long long>(remaining_seconds + 0.5); // round to nearest second
    long long hours = remaining / 3600;
    long long minutes = (remaining % 3600) / 60;
    cout << "Estimated time remaining: " << hours << " hours " << minutes << " minutes" << endl;
}

void runGeneticAlgorithm(Genome* initialGenome = nullptr, int startGen = 0) {

    vector<std::unique_ptr<World>> population = generateInitialPopulation(initialGenome);

    if (startGen == -1) {
        startGen = getLastGenerationNumber() + 1;
        cout << "Starting from last generation: " << startGen << endl;
    } else if (startGen == 0) {
        cout << "Starting from scratch." << endl;
    }else {
        cout << "Starting from generation: " << startGen << endl;
    }

    // For timing
    vector<chrono::duration<double>> gen_durations;

    for (int gen = startGen; gen < NUM_GENERATIONS; gen++) {

        auto gen_start = std::chrono::high_resolution_clock::now();

        cout << "-------------------------------------" << endl;
        cout << "Generation " << gen+1 << "/" << NUM_GENERATIONS << endl;
        cout << "-------------------------------------" << endl;
        
        int count = 0;
        for (const auto& ind : population) {

            // cout << "Individual " << count+1 << "/" << POPULATION_SIZE << endl;

            // progress bar
            string progBar = "[";
            count++;

            for (int i = 0; i < 30; i++) {
                if (i < (static_cast<int>((static_cast<double>(count) / POPULATION_SIZE) * 30))) progBar += ">";
                else progBar += " ";
            }

            vector<double> ind_fitnesses;
            
            for (int t = 0; t < NUM_TRIES; t++) {

                string progText = "Ind " + to_string(count) + "/" + to_string(POPULATION_SIZE) + " | Try " + to_string(t+1) + "/" + to_string(NUM_TRIES);

                ind->run(NUM_STEPS, false);
                ind->calculateFitness();
                ind_fitnesses.push_back(ind->fitness);

                if (t < NUM_TRIES - 1) {
                    // Reset world for next try
                    ind->fitness = 0.0;

                    ind->junctions.clear();
                    ind->tubes.clear();

                    vector<unique_ptr<Junction>> junctions = initializeJunctions();
                    vector<unique_ptr<Tube>> tubes = initializeTubes(junctions);
                    ind->junctions = std::move(junctions);
                    ind->tubes = std::move(tubes);
                }

                cout << progText << "\n";
                cout << progBar << "] " << static_cast<int>((static_cast<double>(count) / POPULATION_SIZE) * 100) << "%\n";

                cout << "\033[2A";

            }

            std::sort(ind_fitnesses.begin(), ind_fitnesses.end(), std::less<double>());

            ind->fitness = std::accumulate(ind_fitnesses.begin(), ind_fitnesses.end(), 0.0) / NUM_TRIES; // fitness is the average
        
            cout << "\033[2K\r";           // clear current line
            cout << "\033[1B\033[2K\r";    // move down, clear next line
            cout << "\033[1A";             // restore cursor position

        }

        // cout << "\033[A\033[A\033[2K\033[1G";


        sortByFitness(population);
        
        double bestFitness = population.front()->fitness;
        double averageFitness = accumulate(population.begin(), population.end(), 0.0, [](double sum, const unique_ptr<World>& w) { return sum + w->fitness; }) / population.size();
        
        saveGenomeAndFitness(population.front()->getGenome(), bestFitness, averageFitness, gen);

        // plot
        system("python3 plot.py");

        cout << "Population fitness:" << endl;
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i]->fitness;
            if (i < population.size() - 1) cout << ", ";
        }
        cout << endl;

        cout << "-------------------------------------" << endl;
        cout << "Best fitness: " << bestFitness << endl;
        cout << "Average fitness: " << averageFitness << endl;

        population = createNextGeneration(population);

        // ==== Timing and ETA ====
        auto gen_end = chrono::high_resolution_clock::now();
        gen_durations.push_back(gen_end - gen_start);
        cout << "Generation time: " << gen_durations.back().count() << " seconds.\n";

        printETA(gen, gen_durations);
    }
}

int main(int argc, char* argv[]) {

    // load genome by generation number
    if (argc > 1) {
        int gen = stoi(argv[1]);
        Genome genome = readGenome(gen);
        if (gen != -1) deleteGenomeRecordsAfter(gen);
        runGeneticAlgorithm(&genome, gen);
    } else {
        runGeneticAlgorithm();
    }
}