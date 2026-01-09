#ifndef TELEGRAM_HPP
#define TELEGRAM_HPP

#include <string>
#include <cstdint>
#include "kufar.hpp"

namespace Telegram {
    struct TelegramConfiguration {
        std::string botToken;
        uint64_t chatID;
    };

    void sendAdvert(const TelegramConfiguration &, const Kufar::Ad &);
};

#endif /* TELEGRAM_HPP */
