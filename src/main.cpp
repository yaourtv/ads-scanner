#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <vector>
#include <curl/curl.h>

#include <nlohmann/json.hpp>
#include "kufar.hpp"
#include "telegram.hpp"
#include "networking.hpp"
#include "helperfunctions.hpp"
#include "logging.hpp"

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
    string logPath;
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
    Log::info("Loading file: \"" + JSONFilePath + "\"");

    if (!fileExists(JSONFilePath)) {
        if (isCache) {
            Log::info("Cache file not found, creating empty cache");
            saveFile(JSONFilePath, json::array().dump());
            return json::array();
        } else {
            Log::error("File does not exist: " + JSONFilePath);
            exit(1);
        }
    }
    if (getFileSize(JSONFilePath) > 4000000) {
        Log::error("File over 4MB: " + JSONFilePath);
        exit(1);
    }

    try {
        json textFromFile = json::parse(getTextFromFile(JSONFilePath));
        return textFromFile;
    } catch (const exception &exc) {
        Log::error("Cannot read/parse file \"" + JSONFilePath + "\": " + exc.what());
        exit(1);
    }
}

const string prefixConfigurationFile = "--config=";
const string prefixCacheFile = "--cache=";
const string prefixLogFile = "--log=";

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
        } else if (stringHasPrefix(currentArgument, prefixLogFile)) {
            currentArgument.erase(0, prefixLogFile.length());
            files.logPath = currentArgument;
        }
    }

    if (files.logPath.empty()) files.logPath = "ads-scanner.log";
    if (!Log::init(files.logPath)) {
        cerr << "Cannot open log file: " << files.logPath << endl;
        exit(1);
    }

    if (files.configuration.path.empty() || files.cache.path.empty()) {
        optional<string> applicationDirectory = getWorkingDirectory();

        if (!applicationDirectory.has_value()) {
            Log::error("Cannot resolve application directory. Pass --config= and --cache= paths.");
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

static volatile sig_atomic_t g_shutdown_requested = 0;

static void shutdown_handler(int sig) {
    (void)sig;
    g_shutdown_requested = 1;
}

int main(int argc, char **argv) {
    ProgramConfiguration programConfiguration;
    vector<AdPrice> cachedAds;

    programConfiguration.files = getFiles(argc, argv);

    Log::info("Starting ads-scanner pid=" + to_string(getpid()) +
              " config=" + programConfiguration.files.configuration.path +
              " cache=" + programConfiguration.files.cache.path +
              " log=" + programConfiguration.files.logPath);

    loadJSONConfigurationData(
        programConfiguration.files.configuration
            .contents,
        programConfiguration);
    cachedAds = programConfiguration.files.cache.contents
        .get<vector<AdPrice>>();

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        Log::error("curl_global_init failed");
        return 1;
    }

#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    signal(SIGTERM, shutdown_handler);
    signal(SIGINT, shutdown_handler);

    unsigned long long loopNum = 0;
    while (true) {
        if (g_shutdown_requested) {
            Log::info("Shutdown requested by signal, exiting.");
            break;
        }

        Log::info("Loop " + to_string(loopNum) + " started");
        if (loopNum > 0 && loopNum % 20 == 0)
            Log::info("Heartbeat: " + to_string(loopNum) + " loops completed");

        for (auto requestConfiguration :
            programConfiguration.kufarConfiguration) {
            unsigned int sentCount = 0;
            string tagStr = requestConfiguration.tag.value_or("(no tag)");
            Log::info("Processing query tag=" + tagStr);

            try {
                vector<Ad> currentAds = getAds(requestConfiguration);
                Log::info("getAds returned " + to_string(currentAds.size()) + " ads for tag=" + tagStr);

                // Process each ad
                for (const auto &advert : currentAds) {
                    auto cachedPrice =
                        getPriceFromCache(cachedAds, advert.id);

                    if (!cachedPrice.has_value()) {
                        Log::info("New ad: Title=" + advert.title + " ID=" + to_string(advert.id) +
                                  " Tag=" + (advert.tag.value_or("")) + " Link=" + advert.link);
                        cachedAds.push_back({advert.id, advert.price});
                        sentCount += 1;

                        try {
                            sendAdvert(
                                programConfiguration
                                    .telegramConfiguration,
                                advert);
                        } catch (const exception &exc) {
                            Log::error("sendAdvert failed: " + string(exc.what()));
                        }
                    } else if (advert.price <
                               cachedPrice.value()) {
                        Log::info("Price drop: Title=" + advert.title + " ID=" + to_string(advert.id) +
                                  " Old=" + to_string(cachedPrice.value()) + " New=" + to_string(advert.price));

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
                            Log::error("sendAdvert failed: " + string(exc.what()));
                        }
                    }
                    usleep(300000); // 0.3s
                }
            } catch (const exception &exc) {
                Log::error("getAds failed for tag=" + tagStr + ": " + exc.what());
            }

            sleep(programConfiguration.queryDelaySeconds);
            if (sentCount > 0) {
                try {
                    saveFile(
                            programConfiguration.files.cache.path,
                            ((json)cachedAds).dump());
                    Log::info("Cache saved to " + programConfiguration.files.cache.path);
                } catch (const exception &exc) {
                    Log::error("saveFile failed: " + string(exc.what()));
                }
            }
        }

        Log::info("Loop " + to_string(loopNum) + " finished, sleeping " +
                  to_string(programConfiguration.loopDelaySeconds) + "s");
        sleep(programConfiguration.loopDelaySeconds);
        loopNum++;
    }

    curl_global_cleanup();
    Log::info("Exiting.");
    return 0;
}
