#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
inline web::WebRequest createBackupRequest() {
    web::WebRequest req;
    const std::string token = Mod::get()->getSavedValue<std::string>("authorization-token");
    if (!token.empty()) {
        req.header("Authorization", token);
    }
    return req;
}