#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <optional>
#include <cstdint>
#include <unistd.h>
#include <limits.h>
#include <iostream>
#include <libgen.h>
#include "helperfunctions.hpp"

using namespace std;

bool vectorContains(const vector<int> &vector, const int &value) {
    if (find(vector.begin(), vector.end(), value) != vector.end()) {
        return true;
    }
    return false;
}

std::optional<int> getPriceFromCache(
    const std::vector<AdPrice> &cache,
    int id) {
    for (const auto &item : cache) {
        if (item.id == id) {
            return item.price;
        }
    }
    return std::nullopt;
}

void cleanupCacheByIds(
    std::vector<AdPrice> &cache,
    const std::vector<int> &currentIds) {
    auto it = cache.begin();
    int removedCount = 0;
    while (it != cache.end()) {
        if (find(currentIds.begin(), currentIds.end(), it->id) ==
            currentIds.end()) {
            it = cache.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    if (removedCount > 0) {
        cout << "[Cache Cleanup]: Removed " << removedCount
             << " outdated entries" << endl;
    }
}

bool fileExists(const string &path) {
    ifstream f(path);
    return f.good();
}

uint64_t getFileSize(const string &path) {
    ifstream f(path, ios::binary | ios::ate);
    if (!f) {
        return 0;
    }
    auto pos = f.tellg();
    if (pos == -1) {
        return 0;
    }
    return static_cast<uint64_t>(pos);
}

string getTextFromFile(const string &path) {
    ifstream ifs(path);
    return string((istreambuf_iterator<char>(ifs)),
                    (istreambuf_iterator<char>()));
}

time_t zuluToTimestamp(const string &zuluDate) {
    tm t{};
    istringstream stringStream(zuluDate);

    stringStream >> get_time(&t, "%Y-%m-%dT%H:%M:%S");
    if (stringStream.fail()) {
        throw runtime_error{"failed to parse time string"};
    }

    return mktime(&t);
}

string joinIntVector(const vector<int> &nums, const string &delim) {
    stringstream result;
    copy(nums.begin(), nums.end(),
            std::ostream_iterator<int>(result, delim.c_str()));

    string temp = result.str();
    if (!temp.empty()) {
        temp.pop_back();
    }

    return temp;
}

time_t timestampShift(const time_t &timestamp, int shift) {
    return timestamp + (3600 * shift);
}

bool stringHasPrefix(const string &originalString, const string &prefix) {
    return originalString.rfind(prefix, 0) == 0;
}

void saveFile(const string &path, const string &contents) {
    cout << "[Saving viewed advertisement IDs]" << endl;
    ofstream ofs(path, ofstream::trunc);
    ofs << contents;
    ofs.close();
}

#ifdef __APPLE__
    #include <mach-o/dyld.h>
    #include <filesystem>

    optional<string> getWorkingDirectory() {
        char buffer[PATH_MAX];
        uint32_t buffsize = PATH_MAX;

        if (_NSGetExecutablePath(buffer, &buffsize) == 0) {
            return dirname(buffer);
        }

        return nullopt;
    }
#elif __linux__
    #include <linux/limits.h>

    optional<string> getWorkingDirectory() {
        char result[PATH_MAX];
        size_t count = readlink("/proc/self/exe", result, PATH_MAX);
        if (count != -1) {
            return dirname(result);
        }
        return nullopt;
    }
#else
    optional<string> getWorkingDirectory() { return nullopt; }
#endif
