#pragma once

#define BACKUP_REQUEST() \
    [](){ \
        geode::utils::web::WebRequest req; \
        const std::string token = Mod::get()->getSavedValue<std::string>("authorization-token"); \
        if (!token.empty()) { \
            req.header("Authorization", token); \
        } \
        return req; \
    }()