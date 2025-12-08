#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

#include "gen_alg.hpp"

const int WIN_WIDTH = 2500;
const int WIN_HEIGHT = 2000;

const double JUNCTION_RADIUS_FACTOR = 1.0;

const float MIN_TUBE_THICKNESS = .05f;
const double TUBE_THICKNESS_FACTOR = 2.0;

const float FPS = 60.0f;
const float DEFAULT_ZOOM = 0.15f;

const float LINE_THICKNESS = 2.0f;

const int OBJECT_TRANSPARENCY = 90;

const sf::Color BACKGROUND_COLOR(28, 43, 52);
const sf::Color JUNCTION_COLOR(220, 160, 30);
const sf::Color JUNCTION_SUN_COLOR(255, 215, 0);
const sf::Color DEPLETED_JUNCTION_COLOR(180, 140, 20);
const sf::Color TUBE_COLOR = JUNCTION_COLOR;
const sf::Color ARROW_COLOR(255, 100, 100);

const float TUBE_THICKNESS = 2.0f;

const float ARROW_SIZE = 1.0f;


struct JunctionVisual {
    double x;
    double y;
    double energy;
    bool sun;
};

struct TubeVisual {
    double x1;
    double y1;
    double x2;
    double y2;
    double flowRate;
    bool reversed;
};

struct Frame {
    vector<JunctionVisual> junctions;
    vector<TubeVisual> tubes;

    double fitness = 0.0;

    void addObject(const string& line) {
        stringstream ss(line);
        string token;
        vector<string> fields;

        while (getline(ss, token, ',')) {
            fields.push_back(token);
        }

        fitness = stod(fields[1]);

        if (!fields[2].empty()) {
            JunctionVisual j{stod(fields[2]), stod(fields[3]), stod(fields[4]), (fields[5] == "1")};
            junctions.push_back(j);
        } else if (!fields[6].empty()) {
            TubeVisual t{
                stod(fields[6]),
                stod(fields[7]),
                stod(fields[8]),
                stod(fields[9]),
                stod(fields[10]),
                (fields[11] == "1")
            };
            tubes.push_back(t);
        }
    }
};