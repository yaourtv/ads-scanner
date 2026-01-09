#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>
#include "kufar.hpp"
#include "telegram.hpp"
#include "networking.hpp"
#include "helperfunctions.hpp"

using namespace std;
using namespace Kufar;
using namespace Telegram;
using nlohmann::json;

const string CACHE_FILE_NAME = "cached-data.json";
const string CONFIGURATION_FILE_NAME = "kufar-configuration.json";

struct ConfigurationFile {
    string path;
    json contents;
};

struct CacheFile {
    string path;
    json contents;
};

struct Files {
    ConfigurationFile configuration;
    CacheFile cache;
};

struct ProgramConfiguration {
    vector<KufarConfiguration> kufarConfiguration;
    TelegramConfiguration telegramConfiguration;
    Files files;

    int queryDelaySeconds = 5;
    int loopDelaySeconds = 30;
};

void loadJSONConfigurationData(
    const json &data,
    ProgramConfiguration &programConfiguration) {
    {
        json telegramData = data.at("telegram");
        auto botToken =
            telegramData.at("bot-token");
        programConfiguration.telegramConfiguration
            .botToken = botToken;
        auto chatId = telegramData.at("chat-id");
        programConfiguration.telegramConfiguration.chatID =
            chatId;
    }
    {
        json queriesData = data.at("queries");

        unsigned int index = 0;
        for (const json &query : queriesData) {
            KufarConfiguration kufarConfiguration;

            kufarConfiguration.onlyTitleSearch =
                getOptionalValue<bool>(
                    query, "only-title-search");
            kufarConfiguration.tag = getOptionalValue<string>(query, "tag");
            if (query.contains("price")) {
                json queryPriceData = query.at("price");
                kufarConfiguration.priceRange.priceMin =
                    getOptionalValue<int>(
                        queryPriceData, "min");
                kufarConfiguration.priceRange.priceMax =
                    getOptionalValue<int>(
                        queryPriceData, "max");
            }

            kufarConfiguration.language =
                getOptionalValue<string>(
                    query, "language");
            kufarConfiguration.limit = getOptionalValue<int>(query, "limit");
            kufarConfiguration.currency =
                getOptionalValue<string>(
                    query, "currency");
            kufarConfiguration.condition =
                getOptionalValue<ItemCondition>(
                    query, "condition");
            kufarConfiguration.sellerType =
                getOptionalValue<SellerType>(
                    query, "seller-type");
            kufarConfiguration.kufarDeliveryRequired =
                getOptionalValue<bool>(
                    query, "kufar-delivery-required");
            kufarConfiguration.kufarPaymentRequired =
                getOptionalValue<bool>(
                    query, "kufar-payment-required");
            kufarConfiguration.kufarHalvaRequired =
                getOptionalValue<bool>(
                    query, "kufar-halva-required");
            kufarConfiguration.onlyWithPhotos =
                getOptionalValue<bool>(
                    query, "only-with-photos");
            kufarConfiguration.onlyWithVideos =
                getOptionalValue<bool>(
                    query, "only-with-videos");
            kufarConfiguration.onlyWithExchangeAvailable =
                getOptionalValue<bool>(
                    query, "only-with-exchange-available");
            kufarConfiguration.sortType =
                getOptionalValue<SortType>(
                    query, "sort-type");
            kufarConfiguration.category =
                getOptionalValue<Category>(
                    query, "category");
            kufarConfiguration.subCategory =
                getOptionalValue<int>(
                    query, "sub-category");
            kufarConfiguration.region =
                getOptionalValue<Region>(
                    query, "region");
            kufarConfiguration.areas =
                getOptionalValue<vector<int>>(query, "areas");
            programConfiguration.kufarConfiguration
                .push_back(kufarConfiguration);

            index += 1;
        }
    }
    {
        if (data.contains("delays")) {
            json delaysData = data.at("delays");
            programConfiguration.queryDelaySeconds = delaysData.at("query");
            programConfiguration.loopDelaySeconds = delaysData.at("loop");
        }
    }
}

json getJSONDataFromPath(const string &JSONFilePath,
                          bool isCache = false) {
    cout << "[Loading file]: " << '"' << JSONFilePath << '"'
         << endl;

    if (!fileExists(JSONFilePath)) {
        if (isCache) {
            // For cache file, create empty array and return
            cout << "[Info]: Cache file not found, creating empty "
                 << "cache" << endl;
            saveFile(JSONFilePath, json::array().dump());
            return json::array();
        } else {
            cout << "[Err]: File does not exist." << endl;
            exit(1);
        }
    }
    if (getFileSize(JSONFilePath) > 4000000) {
        cout << "[Err]: File's over 4MB." << endl;
        exit(1);
    }

    try {
        json textFromFile = json::parse(getTextFromFile(JSONFilePath));
        return textFromFile;
    } catch (const exception &exc) {
        cout << "[Err]: Can't read file " << '"' << JSONFilePath
             << '"' << endl;
        cout << "::: " << exc.what() << " :::" << endl;
        exit(1);
    }
}

const string prefixConfigurationFile = "--config=";
const string prefixCacheFile = "--cache=";

Files getFiles(const int &argsCount, char **args) {
    Files files;

    for (int i = 0; i < argsCount; i++){
        string currentArgument = args[i];

        if(stringHasPrefix(currentArgument, prefixConfigurationFile)) {
            currentArgument.erase(0, prefixConfigurationFile.length());
            files.configuration.path = currentArgument;
        } else if (stringHasPrefix(currentArgument, prefixCacheFile)) {
            currentArgument.erase(0, prefixCacheFile.length());
            files.cache.path = currentArgument;
        }
    }

    if (files.configuration.path.empty() || files.cache.path.empty()) {
        optional<string> applicationDirectory = getWorkingDirectory();

        if (!applicationDirectory.has_value()) {
            cout << "[Err]: Cannot automatically create a path to a current"
                    << " folder. Pass config/cache file paths as an argument."
                    << endl;
            exit(1);
        }

        if (files.configuration.path.empty()) {
            files.configuration.path = applicationDirectory.value() 
                + PATH_SEPARATOR + CONFIGURATION_FILE_NAME;
        }

        if (files.cache.path.empty()) {
            files.cache.path = applicationDirectory.value() + PATH_SEPARATOR 
                + CACHE_FILE_NAME;
        }
    }
    files.configuration.contents =
        getJSONDataFromPath(files.configuration.path);
    files.cache.contents = getJSONDataFromPath(files.cache.path, true);

    return files;
}

int main(int argc, char **argv) {
    ProgramConfiguration programConfiguration;
    vector<AdPrice> cachedAds;

    programConfiguration.files = getFiles(argc, argv);
    loadJSONConfigurationData(
        programConfiguration.files.configuration
            .contents,
        programConfiguration);
    cachedAds = programConfiguration.files.cache.contents
        .get<vector<AdPrice>>();

    while (true) {
        for (auto requestConfiguration :
            programConfiguration.kufarConfiguration) {
            unsigned int sentCount = 0;
            vector<int> currentAdIds;

            try {
                vector<Ad> currentAds = getAds(requestConfiguration);

                // Collect all current ad IDs
                for (const auto &ad : currentAds) {
                    currentAdIds.push_back(ad.id);
                }

                // Clean up cache - remove ads no longer in API
                cleanupCacheByIds(cachedAds, currentAdIds);

                // Process each ad
                for (const auto &advert : currentAds) {
                    auto cachedPrice =
                        getPriceFromCache(cachedAds, advert.id);

                    if (!cachedPrice.has_value()) {
                        // New ad not seen before
                        cout << "[New]: Adding [Title: "
                            << advert.title << "], [ID: "
                            << advert.id << "], [Tag: "
                            << advert.tag << "], [Link: "
                            << advert.link << "]" << endl;
                        cachedAds.push_back({advert.id, advert.price});
                        sentCount += 1;

                        try {
                            sendAdvert(
                                programConfiguration
                                    .telegramConfiguration,
                                advert);
                        } catch (const exception &exc) {
                            cout << "[ERROR (sendAdvert)]: "
                                    << exc.what() << endl;
                            cerr << "[ERROR (sendAdvert)]: "
                                    << exc.what() << endl;
                        }
                    } else if (advert.price <
                               cachedPrice.value()) {
                        // Price dropped - send alert
                        cout << "[Price Drop]: [Title: "
                            << advert.title << "], [ID: "
                            << advert.id << "], [Old Price: "
                            << cachedPrice.value()
                            << "], [New Price: "
                            << advert.price << "]" << endl;

                        // Update cached price
                        for (auto &item : cachedAds) {
                            if (item.id == advert.id) {
                                item.price = advert.price;
                                break;
                            }
                        }
                        sentCount += 1;

                        try {
                            sendAdvert(
                                programConfiguration
                                    .telegramConfiguration,
                                advert);
                        } catch (const exception &exc) {
                            cout << "[ERROR (sendAdvert)]: "
                                    << exc.what() << endl;
                            cerr << "[ERROR (sendAdvert)]: "
                                    << exc.what() << endl;
                        }
                    }
                    usleep(300000); // 0.3s
                }
            } catch (const exception &exc) {
                cout << "[ERROR (getAds)]: " << exc.what() << endl;
                cerr << "[ERROR (getAds)]: " << exc.what() << endl;
            }

            sleep(programConfiguration.queryDelaySeconds);
            if (sentCount > 0) {
                saveFile(
                        programConfiguration.files.cache.path,
                        ((json)cachedAds).dump());
            }
        }

        sleep(programConfiguration.loopDelaySeconds);
    }
    return 0;
}
