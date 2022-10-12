#include "api.h"

#include "Route.h"
#include "NetworkEngine.h"

#include <VrLib/tien/components/TerrainRenderer.h>
#include <VrLib/tien/components/MeshRenderer.h>
#include <VrLib/tien/components/Transform.h>
#include <VrLib/tien/Terrain.h>
#include <VrLib/Texture.h>


void buildMesh(vrlib::tien::components::MeshRenderer::Mesh* mesh, const Route& route, const std::function<glm::vec3(const glm::vec2 &)>& getPos)
{
	float inc = 0.1f;
	glm::vec2 lastPos = route.getPositionXY(0);
	vrlib::gl::VertexP3N2B2T2T2 v;
	vrlib::gl::setN3(v, glm::vec3(0, 1, 0));
	vrlib::gl::setTan3(v, glm::vec3(1, 0, 0));
	vrlib::gl::setBiTan3(v, glm::vec3(0, 0, 1));

	float t = 0;
	int count = 0;

	for (float f = inc; f < route.length; f += inc)
	{
		int i = mesh->vertices.size();

		glm::vec2 pos = route.getPositionXY(f);
		glm::vec2 diff = lastPos - pos;

		glm::vec2 angled = glm::normalize(glm::vec2(diff.y, -diff.x));

		for (float ff = -2.5f; ff <= 2.5f; ff += 0.1f)
		{
			vrlib::gl::setP3(v, getPos(pos + angled * ff));			vrlib::gl::setT2(v, glm::vec2(t, ff));			mesh->vertices.push_back(v);
			if (count <= 0)
				count--;
		}
		if (count < 0)
			count = -count;

		lastPos = pos;
		t += inc * 1.0f;
	}
	for (size_t i = 0; i < mesh->vertices.size() / count; i++)
	{
		for (int ii = 0; ii < count - 1; ii++)
		{
			mesh->indices.push_back((count * i + ii + 0) % mesh->vertices.size());
			mesh->indices.push_back((count * i + ii + 1) % mesh->vertices.size());
			mesh->indices.push_back((count * i + ii + count) % mesh->vertices.size());

			mesh->indices.push_back((count * i + ii + 1) % mesh->vertices.size());
			mesh->indices.push_back((count * i + ii + count + 1) % mesh->vertices.size());
			mesh->indices.push_back((count * i + ii + count) % mesh->vertices.size());
		}
	}


	for (size_t i = 0; i < mesh->indices.size(); i += 3)
	{
		glm::vec3 normal(0, 1, 0), tan(1, 0, 0), bitan(0, 0, 1);
		for (int ii = 0; ii < 3; ii++)
		{
			vrlib::gl::setN3(mesh->vertices[mesh->indices[i + ii]], normal);
			vrlib::gl::setTan3(mesh->vertices[mesh->indices[i + ii]], tan);
			vrlib::gl::setBiTan3(mesh->vertices[mesh->indices[i + ii]], bitan);
		}
	}
}



Api scene_road_add("scene/road/add", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	float heightOffset = 0.01f;
	if (data.find("heightoffset") != data.end())
		heightOffset = data["heightoffset"];

	for (size_t i = 0; i < engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["route"])
		{
			const vrlib::tien::Node* terrainRenderingNode = engine->tien.scene.findNodeWithComponent<vrlib::tien::components::TerrainRenderer>();
			
			//TODO: use terrainRenderingNode->transform->position.y (with parents) in the code down here
			const Route& route = *engine->routes[i];
			auto getPos = [&engine, &terrainRenderingNode](const glm::vec2 &p)
			{
				if (!engine->terrain)
					return glm::vec3(p.x, 0.01, p.y);
				glm::vec2 offset(0, 0);
				if (terrainRenderingNode)
					offset = glm::vec2(terrainRenderingNode->transform->position.x, terrainRenderingNode->transform->position.z);
				return engine->terrain->getPosition(p - offset) + glm::vec3(offset.x, 0.01f, offset.y);
			};

			{
				auto mesh = new vrlib::tien::components::MeshRenderer::Mesh();
				mesh->material.texture = vrlib::Texture::loadCached(data.value("diffuse", "data/NetworkEngine/textures/tarmac_diffuse.png"));
				mesh->material.normalmap = vrlib::Texture::loadCached(data.value("normal", "data/NetworkEngine/textures/tarmac_normal.png"));
				mesh->material.specularmap = vrlib::Texture::loadCached(data.value("specular", "data/NetworkEngine/textures/tarmac_specular.png"));
				mesh->material.color.shinyness = 1;

				buildMesh(mesh, route, getPos);

				vrlib::tien::Node* n = new vrlib::tien::Node("Road", &engine->tien.scene);
				packet["status"] = "ok";
				packet["data"]["uuid"] = n->guid;
//				if(terrainRenderingNode)
//					n->addComponent(new vrlib::tien::components::Transform(terrainRenderingNode->transform->position, terrainRenderingNode->transform->rotation, terrainRenderingNode->transform->scale));
//				else
					n->addComponent(new vrlib::tien::components::Transform(glm::vec3(0, 0, 0)));
				n->addComponent(new vrlib::tien::components::MeshRenderer(mesh));
			}
			return;
		}
	}
	packet["error"] = "Route not found";
});



Api scene_road_update("scene/road/update", [](NetworkEngine* engine, nlohmann::json &data, nlohmann::json &packet)
{
	for (size_t i = 0; i < engine->routes.size(); i++)
	{
		if (engine->routes[i]->id == data["route"])
		{
			auto renderingNode = engine->tien.scene.findNodeWithName(data["id"]);
			const vrlib::tien::Node* terrainRenderingNode = engine->tien.scene.findNodeWithComponent<vrlib::tien::components::TerrainRenderer>();
			const Route& route = *engine->routes[i];
			auto getPos = [&engine, &terrainRenderingNode](const glm::vec2 &p)
			{
				if (!engine->terrain)
					return glm::vec3(p.x, 0.01, p.y);
				glm::vec2 offset(0, 0);
				if (terrainRenderingNode)
					offset = glm::vec2(terrainRenderingNode->transform->position.x, terrainRenderingNode->transform->position.z);
				return engine->terrain->getPosition(p - offset) + glm::vec3(offset.x, 0.01f, offset.y);
			};

			auto meshRenderer = renderingNode->getComponent<vrlib::tien::components::MeshRenderer>();
			buildMesh(meshRenderer->mesh, route, getPos);
			meshRenderer->updateMesh();
		}
	}
	packet["error"] = "Route not found";
});