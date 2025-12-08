#include <vector>
#include "utils.hpp"

const vector<pair<int, int>> FLOW_NET_DIMS = {
    {8, 7},
    {7, 5},
    {5, 4},
    {4, 2}
};

using namespace std;

struct Genome {
    vector<vector<vector<double>>> flowNetWeights;

    Genome() {
        auto addLayer = [&](const std::pair<int, int> &d,
                            std::vector<std::vector<std::vector<double>>> &net) {
            int in = d.first, out = d.second;
            double s = std::sqrt(2.0 / in);
            std::vector<std::vector<double>> W(in, std::vector<double>(out));
            for (int i = 0; i < in; ++i)
                for (int j = 0; j < out; ++j)
                    W[i][j] = Random::gaussian(0.0, s);
            std::vector<std::vector<double>> bias_row(1, std::vector<double>(out, 0.0));
            net.push_back(std::move(W));
            net.push_back(std::move(bias_row));
        };

        for (const auto &d : FLOW_NET_DIMS)
            addLayer(d, flowNetWeights);
    }

    vector<vector<vector<double>>> getFlowNetWeights() const {
        return flowNetWeights;
    }

    void setFlowNetWights(vector<double> weights) {
        int index = 0;
        for (const auto &layer_dim : FLOW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            for (int i = 0; i < in; ++i) {
                for (int j = 0; j < out; ++j) {
                    flowNetWeights[index][i][j] = weights.front();
                    weights.erase(weights.begin());
                }
            }
            index++;
            for (int j = 0; j < out; ++j) {
                flowNetWeights[index][0][j] = weights.front();
                weights.erase(weights.begin());
            }
            index++;
        }
    }

    void mutate(double mutation_rate, double mutation_strength) {
        for (auto &weight_matrix : flowNetWeights) {
            for (auto &weight_vector : weight_matrix) {
                for (auto &weight : weight_vector) {
                    if (Random::uniform(0.0, 1.0) < mutation_rate) {
                        weight += Random::uniform(-mutation_strength, mutation_strength);
                    }
                }
            }
        }
    }

    vector<double> serialize() const {
        vector<double> genome_vector;
        for (const auto &layer_weights : flowNetWeights) {
            for (const auto &weight_vector : layer_weights) {
                for (const auto &weight : weight_vector) {
                    genome_vector.push_back(weight);
                }
            }
        }
        return genome_vector;
    }
};

void deleteGenomeRecordsAfter(int gen) {

    gen -= 1; // adjust to zero-based index

    ifstream file("data/genome_fitness.csv");
    if (!file)
        throw runtime_error("Could not open file");

    string line;
    vector<string> lines;
    getline(file, line); // keep header
    lines.push_back(line);

    while (getline(file, line)) {
        string genStr = line.substr(0, line.find(';'));
        if (stoi(genStr) <= gen) {
            lines.push_back(line);
        }
    }
    file.close();

    ofstream outFile("data/genome_fitness.csv", ios::trunc);
    for (const auto &l : lines) {
        outFile << l << "\n";
    }
    outFile.close();
}

Genome readGenome(int gen) {

    ifstream file("data/genome_fitness.csv");
    if (!file)
        throw runtime_error("Could not open file");

    string line;
    getline(file, line); // skip header

    // before reading lines
    int targetGen = (gen == -1) ? -1 : gen - 1;

    string selectedLine, lastLine;
    while (getline(file, line)) {
        lastLine = line;
        if (targetGen != -1) {
            string genStr = line.substr(0, line.find(';'));
            if (stoi(genStr) == targetGen) {
                selectedLine = line;
                break;
            }
        }
    }

    if (selectedLine.empty())
        selectedLine = lastLine;

    stringstream ss(selectedLine);
    string genStr, bestFitnessStr, averageFitnessStr, genomeStr;
    getline(ss, genStr, ';');
    getline(ss, bestFitnessStr, ';');
    getline(ss, averageFitnessStr, ';');
    getline(ss, genomeStr, ';');

    vector<double> weights;
    int weights_size = 0;
    for (const auto &layer_dim : FLOW_NET_DIMS)
    {
        int in = layer_dim.first;
        int out = layer_dim.second;
        weights_size += in * out + out;
    }
    weights.reserve(weights_size);
    stringstream rulesStream(genomeStr);
    for (string token; getline(rulesStream, token, ' ');)
    {
        weights.push_back(static_cast<double>(stod(token)));
    }

    int grow_net_size = 0;

    vector<double> weights_a(weights.begin(), weights.begin() + grow_net_size);
    vector<double> weights_b;
    weights_b.assign(weights.begin() + grow_net_size, weights.end());

    Genome genome;
    genome.setFlowNetWights(weights_b);

    return genome;
}

int getLastGenerationNumber()
{
    ifstream file("data/genome_fitness.csv");
    if (!file)
        throw runtime_error("Could not open file");

    string line;
    getline(file, line); // skip header

    string lastLine;
    while (getline(file, line))
    {
        lastLine = line;
    }

    if (lastLine.empty())
        return -1;

    stringstream ss(lastLine);
    string genStr;
    getline(ss, genStr, ';');

    return stoi(genStr);
}