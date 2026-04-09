#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

class Config {
public:
    std::unordered_map<std::string, std::string> data;

    bool load(const std::string& filename);

    int get_int(const std::string& key);
    double get_double(const std::string& key);
    std::string get_string(const std::string& key);
};

#endif
