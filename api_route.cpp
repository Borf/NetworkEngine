#include "api.h"

#include "Route.h"

#include <algorithm>


Api route_add("route/add", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	Route* r = new Route();
	if (data.find("id") != data.end())
	{
		printf("Got forced route id!\n");
		r->id = data["id"].get<std::string>();
	}
	for (size_t i = 0; i < data["nodes"].size(); i++)
	{
		r->addNode(glm::vec3(data["nodes"][i]["pos"][0], data["nodes"][i]["pos"][1], data["nodes"][i]["pos"][2]),
			glm::vec3(data["nodes"][i]["dir"][0], data["nodes"][i]["dir"][1], data["nodes"][i]["dir"][2]));
	}
	r->finish();
	engine->routes.push_back(r);
	packet["status"] = "ok";
	packet["data"]["uuid"] = r->id;
});



Api route_update("route/update", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	for (auto & route : engine->routes)
	{
		if (route->id == data["id"])
		{
			for (const auto &n : data["nodes"])
			{
				int index = n["index"];
				if (n.find("pos") != n.end())
					route->setNode(index, glm::vec3(n["pos"][0], n["pos"][1], n["pos"][2]), glm::vec3(n["dir"][0], n["dir"][1], n["dir"][2]));
				else
					route->removeNode(index);
			}
			packet["status"] = "ok";
			return;
		}
	}
	packet["error"] = "Route not found";
});


Api route_delete("route/delete", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	for (size_t i = 0; i < engine->routeFollowers.size(); i++)
	{
		if (engine->routeFollowers[i].route->id == data["id"])
		{
			packet["error"] = "Route is still in use";
			return;
		}
	}

	for (size_t i = 0; i <engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["id"])
		{
			engine->routes.erase(engine->routes.begin() + i);
			packet["status"] = "ok";
			return;
		}
	}
	packet["error"] = "Route not found";
});



Api route_follow("route/follow", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	bool found = false;
	for (size_t i = 0; i <engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["route"])
		{
			found = true;
			vrlib::tien::Node* n = engine->tien.scene.findNodeWithGuid(data["node"]);
			if (!n)
			{
				packet["error"] = "node not found";
				break;
			}
			for (size_t ii = 0; ii < engine->routeFollowers.size(); ii++)
			{
				auto & f = engine->routeFollowers[ii];
				if (f.node == n)
				{
					engine->routeFollowers.erase(engine->routeFollowers.begin() + ii);
					packet["status"] = "ok";
					packet["data"]["msg"] = "removed";
					return;
				}
			}

			RouteFollower f;
			f.node = n;
			f.route = engine->routes[i];
			f.speed = data["speed"];
			f.offset = data.value("offset", 0.0f);
			f.followHeight = data.value("followHeight", false);
			f.rotate = RouteFollower::Rotate::NONE;
			if (data.find("rotate") != data.end())
			{
				if (data["rotate"] == "XZ")
					f.rotate = RouteFollower::Rotate::XZ;
				else if (data["rotate"] == "XYZ")
					f.rotate = RouteFollower::Rotate::XYZ;
			}
			if (data.find("rotateOffset") != data.end())
				f.rotateOffset = glm::quat(glm::vec3(data["rotateOffset"][0], data["rotateOffset"][1], data["rotateOffset"][2]));
			if (data.find("positionOffset") != data.end())
				f.positionOffset = glm::vec3(data["positionOffset"][0], data["positionOffset"][1], data["positionOffset"][2]);
			f.smoothing = data.value("smoothing", 1.0f);
			engine->routeFollowers.push_back(f);
			packet["status"] = "ok";
		}
	}
	if(!found && packet.find("error") == packet.end())
		packet["error"] = "Route not found";
});

Api route_follow_speed("route/follow/speed", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	for (auto &f : engine->routeFollowers)
	{
		if (f.node->guid == data["node"])
		{
			f.speed = data["speed"];
			packet["status"] = "ok";
			return;
		}
	}
	packet["error"] = "Node ID not found";
});

Api route_show("route/show", [](NetworkEngine* engine, const nlohmann::json &data, nlohmann::json &packet)
{
	if (data.find("show") == data.end())
		engine->showRoutes = !engine->showRoutes;
	else
		engine->showRoutes = data["show"];

	packet["status"] = "ok";
	packet["data"]["show"] = engine->showRoutes;
});