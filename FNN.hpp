#include <vector>
#include <cmath>
#include <string>

using namespace std;
#include <functional>


struct FNNLayer {
    vector<vector<double>> weights; // input x output
    vector<double> biases; // output

    function<double(double)> activation;

    void setActivation(string act) {
        if (act == "sigmoid") {
            activation = [](double x) {
                if (x >= 0) return 1.0 / (1.0 + exp(-x));
                double e = exp(x);
                return e / (1.0 + e);
            };
        } else if (act == "relu") {
            activation = [](double x) { return x > 0 ? x : 0.0; };
        } else { // tanh
            activation = [](double x) { return tanh(x); };
        }
    }

    FNNLayer(vector<vector<double>> weights, vector<double> biases, string act)
        : weights(weights),
          biases(biases) {
        setActivation(act);
    }
};

struct FNN {

    vector<FNNLayer> layers;
    
    void addLayer(FNNLayer layer) {
        layers.emplace_back(layer);
    }

    void initialize(const vector<vector<vector<double>>>& weights) {

        int num_layers = weights.size()/2;

        for (int i = 0; i < num_layers; ++i) {
            string activation = "relu";
            if (i == num_layers - 1) activation = "sigmoid";
            auto layer = FNNLayer(weights[i*2], weights[i*2+1][0], activation);
            addLayer(layer);
        }
    }
    
    vector<double> forward(const vector<double>& input) {
        vector<double> current_output = input;
        for (int i = 0; i < layers.size(); ++i) {
            vector<vector<double>> weights = layers[i].weights;
            vector<double> biases = layers[i].biases;
            vector<double> next_output(biases.size(), 0.0);
            for (int j = 0; j < biases.size(); ++j) {
                double sum = biases[j];
                for (int k = 0; k < current_output.size(); ++k) {
                    sum += current_output[k] * weights[k][j];
                }
                next_output[j] = layers[i].activation(sum);
            }
            current_output = next_output;
        }
        return current_output;
    }

    vector<double> predict(const vector<double>& input) {
        return forward(input);
    }
};