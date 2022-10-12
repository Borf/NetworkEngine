#pragma once

#include <string>
#include <functional>
#include <VrLib/json.hpp>
#include <VrLib/ServerConnection.h>
#include "NetworkEngine.h"

struct Api
{
	Api(const std::string &route, const std::function<void(NetworkEngine*, nlohmann::json &, nlohmann::json &)> &callback);
};


//void sendError(vrlib::Tunnel* tunnel, const std::string &packet, const std::string &error);
//void sendOk(vrlib::Tunnel* tunnel, const std::string &packet);