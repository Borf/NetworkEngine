#include "GrassComponent.h"

#include <VrLib/gl/VAO.h>
#include <VrLib/gl/shader.h>
#include <VrLib/Texture.h>

#include <VrLib/tien/Terrain.h>
#include <VrLib/tien/components/Transform.h>
#include <VrLib/tien/Node.h>


GrassComponent::GrassComponent(vrlib::tien::Terrain &terrain) : terrain(terrain)
{
	renderContextDeferred = GrassRenderContext::getInstance();

	grassTexture = vrlib::Texture::loadCached("data/NetworkEngine/textures/grass/grassPack.png");

	std::vector<vrlib::gl::VertexP3> vertices;

	int gridSize = 1; //determines number of grass patches (lower is more)

	for (int x = 0; x < terrain.getWidth() - gridSize; x += gridSize)
	{
		for (int y = 0; y < terrain.getHeight() - gridSize; y += gridSize)
		{
			vrlib::gl::VertexP3 v;
			vrlib::gl::setP3(v, glm::vec3(x, terrain[x][y], y));
			vertices.push_back(v);
		}
	}

	vbo.bind();
	vbo.setData(vertices.size(), &vertices[0], GL_STATIC_DRAW);
	vao = new vrlib::gl::VAO(&vbo);
}


void GrassComponent::update(float elapsedTime, vrlib::tien::Scene& scene)
{
	time += elapsedTime;
}


void GrassComponent::drawDeferredPass()
{
	glDisable(GL_CULL_FACE);
	vrlib::tien::components::Transform* t = node->getComponent<vrlib::tien::components::Transform>();

	GrassRenderContext* context = dynamic_cast<GrassRenderContext*>(renderContextDeferred);
	context->renderShader->use();

	context->renderShader->setUniform(GrassRenderContext::RenderUniform::gSampler, 0);
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::time, time);
	//projectionmatrix and viewmatrix already set in frameSetup() method
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::modelMatrix, t->globalTransform);
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::normalMatrix, glm::transpose(glm::inverse(glm::mat3(t->globalTransform))));
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::vColor, glm::vec4(1, 1, 1, 1));
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::fAlphaTest, 0.25f);
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::fAlphaMultiplier, 1.5f);
	context->renderShader->setUniform(GrassRenderContext::RenderUniform::grassSize, 1.5f);

	grassTexture->bind();
	vao->bind();
	glDrawArrays(GL_POINTS, 0, vbo.getLength());
	vao->unBind();

	glEnable(GL_CULL_FACE);

}

void GrassComponent::drawShadowMap()
{
}


void GrassComponent::GrassRenderContext::frameSetup(const glm::mat4 &projectionMatrix, const glm::mat4 &viewMatrix)
{
	renderShader->use();
	renderShader->setUniform(RenderUniform::projectionMatrix, projectionMatrix);
	renderShader->setUniform(RenderUniform::viewMatrix, viewMatrix);
	viewprojection = projectionMatrix * viewMatrix;
}


void GrassComponent::GrassRenderContext::init()
{
	renderShader = new vrlib::gl::Shader<RenderUniform>("data/NetworkEngine/shaders/grass.vert", "data/NetworkEngine/shaders/grass.frag", "data/NetworkEngine/shaders/grass.geom");
	renderShader->bindFragLocation("outputColor", 0);
	renderShader->bindFragLocation("outputNormal", 1);
	renderShader->link();
	renderShader->registerUniform(RenderUniform::modelMatrix, "modelMatrix");
	renderShader->registerUniform(RenderUniform::projectionMatrix, "projectionMatrix");
	renderShader->registerUniform(RenderUniform::viewMatrix, "viewMatrix");
	renderShader->registerUniform(RenderUniform::normalMatrix, "normalMatrix");
	renderShader->registerUniform(RenderUniform::time, "time");
	renderShader->registerUniform(RenderUniform::vColor, "vColor");
	renderShader->registerUniform(RenderUniform::fAlphaTest, "fAlphaTest");
	renderShader->registerUniform(RenderUniform::fAlphaMultiplier, "fAlphaMultiplier");
	renderShader->registerUniform(RenderUniform::gSampler, "gSampler");
	renderShader->registerUniform(GrassRenderContext::RenderUniform::grassSize, "grassSize");
	renderShader->use();
	renderShader->setUniform(RenderUniform::s_texture, 0);
}


nlohmann::json GrassComponent::toJson(nlohmann::json &meshes) const
{
	nlohmann::json ret;
	ret["type"] = "grass";

	return ret;
}
