#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>

struct Level {
    std::string id;
    std::string name;
    std::string type;
    int nodes = 0;
    int elements = 0;
    int seed = 0;
};

struct Act {
    int act;
    std::string title;
    std::string description;
    std::vector<Level> levels;
};

class LevelLoader {
public:
    std::vector<Act> acts;

    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LevelLoader] Error opening: " << filepath << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string content = buffer.str();

        const std::regex actPattern(
            R"REGEX(\{\s*"act"\s*:\s*(\d+)\s*,\s*"title"\s*:\s*"([^"]*)"\s*,\s*"description"\s*:\s*"([^"]*)"\s*,\s*"levels"\s*:\s*\[([\s\S]*?)\]\s*\})REGEX");
        const std::regex levelPattern(R"REGEX(\{([^}]*)\})REGEX");
        const std::regex idPattern(R"REGEX("id"\s*:\s*"([^"]*)")REGEX");
        const std::regex namePattern(R"REGEX("name"\s*:\s*"([^"]*)")REGEX");
        const std::regex typePattern(R"REGEX("type"\s*:\s*"([^"]*)")REGEX");
        const std::regex nodesPattern(R"REGEX("nodes"\s*:\s*(\d+))REGEX");
        const std::regex elementsPattern(R"REGEX("elements"\s*:\s*(\d+))REGEX");
        const std::regex seedPattern(R"REGEX("seed"\s*:\s*(\d+))REGEX");

        acts.clear();
        for (std::sregex_iterator actIt(content.begin(), content.end(), actPattern), end; actIt != end; ++actIt) {
            const std::smatch& match = *actIt;
            Act act;
            act.act = std::stoi(match[1].str());
            act.title = match[2].str();
            act.description = match[3].str();

            const std::string levelText = match[4].str();
            for (std::sregex_iterator levelIt(levelText.begin(), levelText.end(), levelPattern); levelIt != end; ++levelIt) {
                const std::string fields = (*levelIt)[1].str();
                Level level;
                if (!readString(fields, idPattern, level.id) ||
                    !readString(fields, namePattern, level.name) ||
                    !readString(fields, typePattern, level.type)) {
                    continue;
                }
                readInt(fields, nodesPattern, level.nodes);
                readInt(fields, elementsPattern, level.elements);
                readInt(fields, seedPattern, level.seed);
                act.levels.push_back(level);
            }
            acts.push_back(act);
        }

        if (acts.empty()) {
            std::cerr << "[LevelLoader] No acts found in: " << filepath << std::endl;
            return false;
        }
        return true;
    }

private:
    static bool readString(const std::string& fields, const std::regex& pattern, std::string& value) {
        std::smatch match;
        if (!std::regex_search(fields, match, pattern)) return false;
        value = match[1].str();
        return true;
    }

    static void readInt(const std::string& fields, const std::regex& pattern, int& value) {
        std::smatch match;
        if (std::regex_search(fields, match, pattern)) value = std::stoi(match[1].str());
    }
};

#endif
