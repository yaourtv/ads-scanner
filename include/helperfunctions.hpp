#ifndef helperfunctions_hpp
#define helperfunctions_hpp

#ifdef DEBUG
    #define DEBUG_MSG(str) do { std::cout << str << std::endl; } while( false )
#else
    #define DEBUG_MSG(str) do { } while ( false )
#endif

#if defined(WIN32) || defined(_WIN32)
    #define PATH_SEPARATOR '\\'
#else
    #define PATH_SEPARATOR '/'
#endif

#include <optional>
#include <nlohmann/json.hpp>

static const std::string PROPERTY_UNDEFINED = "[UNDEFINED]";

template<typename T>
std::optional<T> getOptionalValue(const nlohmann::json &obj,
                                    const std::string &key) try {
    return obj.at(key).get<T>();
} catch (...) {
    return std::nullopt;
}

template<typename T>
std::ostream &operator << (std::ostream &os, std::optional<T> const &opt) {
    if (opt.has_value()) {
        if constexpr (std::is_same_v<T, bool>) {
            return (os << (opt.value() == true ? "Yes" : "No"));
        }
        return (os << opt.value());
    }
    return (os << PROPERTY_UNDEFINED);
}

struct AdPrice {
    int id;
    int price;
};

namespace nlohmann {
    template <>
    struct adl_serializer<AdPrice> {
        static void to_json(json &j, const AdPrice &ad) {
            j = json{{"id", ad.id}, {"price", ad.price}};
        }

        static void from_json(const json &j, AdPrice &ad) {
            ad.id = j.at("id").get<int>();
            ad.price = j.at("price").get<int>();
        }
    };
}

bool vectorContains(const std::vector<int> &, const int &);
std::optional<int> getPriceFromCache(
    const std::vector<AdPrice> &cache,
    int id);
void cleanupCacheByIds(
    std::vector<AdPrice> &cache,
    const std::vector<int> &currentIds);
bool fileExists(const std::string &);
uint64_t getFileSize(const std::string &);
std::string getTextFromFile(const std::string &);
time_t zuluToTimestamp(const std::string &);
std::string joinIntVector(const std::vector<int> &, const std::string &);
time_t timestampShift(const time_t &, int);
bool stringHasPrefix(const std::string &, const std::string &);
void saveFile(const std::string &path, const std::string &contents);
std::optional<std::string> getWorkingDirectory();

#endif
