#include <iostream>
#include <string>
#include <curl/curl.h>
#include "networking.hpp"
#include "helperfunctions.hpp"
#include "logging.hpp"

namespace Networking {
    using std::string;
    using std::endl;
    using std::cout;

    namespace {
        static size_t writeFunction(void *ptr, size_t size,
                                        size_t nmemb,
                                        string *data) {
            data->append((char*)ptr, size * nmemb);
            return size * nmemb;
        }
    }

    string urlEncode(const string &text) {
        CURL *curl = curl_easy_init();
        char *encoded = curl_easy_escape(curl, text.c_str(), 0);
        string tempVariable = encoded;
        curl_free(encoded);
        curl_easy_cleanup(curl);
        return tempVariable;
    }

    string getJSONFromURL(const string &url) {
        DEBUG_MSG("[URL: " << url << "]");
        if (Log::isInitialized())
            Log::info("HTTP GET (url length=" + std::to_string(url.size()) + ")");

        auto curl = curl_easy_init();
        string responseString;

        if (!curl) {
            if (Log::isInitialized()) Log::error("getJSONFromURL: curl_easy_init failed");
            return "";
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 0L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);

        string headerString;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerString);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            if (Log::isInitialized())
                Log::error("getJSONFromURL: curl_easy_perform failed: " + string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return "";
        }
        if (Log::isInitialized())
            Log::info("HTTP response size=" + std::to_string(responseString.size()));
        curl_easy_cleanup(curl);
        return responseString;
    }
}
