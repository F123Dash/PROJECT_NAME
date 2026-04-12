#include "config.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

using namespace std;

bool Config::load(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[Config] ERROR: Cannot open " << filename << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Parse key=value
        size_t pos = line.find('=');
        if (pos == string::npos) continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        // Trim key and value
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        data[key] = value;
    }

    cout << "[Config] Loaded " << data.size() << " parameters from " << filename << endl;
    return true;
}

int Config::get_int(const string& key) {
    if (data.find(key) == data.end())
        throw runtime_error("Missing config: " + key);
    return stoi(data[key]);
}

double Config::get_double(const string& key) {
    if (data.find(key) == data.end())
        throw runtime_error("Missing config: " + key);
    return stod(data[key]);
}

string Config::get_string(const string& key) {
    if (data.find(key) == data.end())
        throw runtime_error("Missing config: " + key);
    return data[key];
}

bool Config::get_bool(const string& key) {
    if (data.find(key) == data.end())
        throw runtime_error("Missing config: " + key);
    return get_int(key) != 0;
}
