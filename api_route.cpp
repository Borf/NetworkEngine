#include "api.h"

#include "Route.h"

#include <algorithm>


Api route_add("route/add", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
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

	data["id"] = r->id;

	json packet;
	packet["id"] = "route/add";
	packet["data"]["status"] = "ok";
	packet["data"]["uuid"] = r->id;
	tunnel->send(packet);
});



Api route_update("route/update", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{


});


Api route_delete("route/delete", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	for (size_t i = 0; i < engine->routeFollowers.size(); i++)
	{
		if (engine->routeFollowers[i].route->id == data["id"])
		{
			json packet;
			packet["id"] = "route/delete";
			packet["data"]["status"] = "error";
			packet["data"]["error"] = "Route is still in use";
			tunnel->send(packet);
			return;
		}
	}

	for (size_t i = 0; i <engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["id"])
		{
			engine->routes.erase(engine->routes.begin() + i);
			json packet;
			packet["id"] = "route/delete";
			packet["data"]["status"] = "ok";
			tunnel->send(packet);
			return;
		}
	}
	json packet;
	packet["id"] = "route/delete";
	packet["data"]["status"] = "error";
	packet["data"]["error"] = "Route not found";
	tunnel->send(packet);


});



Api route_follow("route/follow", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, json &data)
{
	json packet;
	packet["id"] = "route/follow";
	packet["data"]["status"] = "error";

	bool found = false;
	for (size_t i = 0; i <engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["route"])
		{
			found = true;
			vrlib::tien::Node* n = engine->tien.scene.findNodeWithGuid(data["node"]);
			if (!n)
			{
				packet["data"]["error"] = "node not found";
				break;
			}
			for (size_t ii = 0; ii < engine->routeFollowers.size(); ii++)
			{
				auto & f = engine->routeFollowers[ii];
				if (f.node == n)
				{
					engine->routeFollowers.erase(engine->routeFollowers.begin() + ii);
					packet["data"]["status"] = "ok";
					packet["data"]["msg"] = "removed";
					tunnel->send(packet);
					return;
				}
			}

			RouteFollower f;
			f.node = n;
			f.route = engine->routes[i];
			f.speed = data["speed"];
			f.offset = data.find("offset") != data.end() ? data["offset"] : 0.0f;
			f.followHeight = false;
			if(data.find("followHeight") != data.end())
				f.followHeight = data["followHeight"];
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

			engine->routeFollowers.push_back(f);
			packet["data"]["status"] = "ok";
		}
	}
	if(!found && packet["data"].find("error") == packet["data"].end())
		packet["data"]["error"] = "Route not found";
	tunnel->send(packet);
});

Api route_follow_speed("route/follow/speed", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, const json &data)
{
	for (auto &f : engine->routeFollowers)
	{
		if (f.node->guid == data["node"])
		{
			f.speed = data["speed"];
			sendOk(tunnel, "route/follow/speed");
			return;
		}
	}
	sendError(tunnel, "route/follow/speed", "Node ID not found");
});

Api route_show("route/show", [](NetworkEngine* engine, vrlib::Tunnel* tunnel, const json &data)
{
	if (data.find("show") == data.end())
		engine->showRoutes = !engine->showRoutes;
	else
		engine->showRoutes = data["show"];

	json ret;
	ret["id"] = "route/show";
	ret["data"]["status"] = "ok";
	ret["data"]["show"] = engine->showRoutes;

});