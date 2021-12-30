
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MeshRenderer.h"
#include "Application.h"
#include "Engine/Mesh.h"
#include "ImGui/ImGuiHelpers.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/EntityManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include <imgui.h>

namespace maple
{
	MeshRenderer::MeshRenderer(const std::shared_ptr<Mesh> &mesh) :
	    mesh(mesh)
	{
	}

	auto MeshRenderer::loadFromModel() -> void
	{
		getMesh(meshName);
		if (mesh)
			mesh->setMaterial(material);
	}

	auto MeshRenderer::getMesh(const std::string &name) -> void
	{
		Entity ent{entity, Application::get()->getSceneManager()->getCurrentScene()};
		auto   model = ent.tryGetComponent<Model>();
		if (model != nullptr)
		{
			switch (model->type)
			{
				case PrimitiveType::Cube:
					mesh = Mesh::createCube();
					break;
				case PrimitiveType::Sphere:
					mesh = Mesh::createSphere();
					break;
				case PrimitiveType::File:
					mesh = model->resource->find(name);
					break;
				case PrimitiveType::Pyramid:
					mesh = Mesh::createPyramid();
					break;
			}
		}
		else
		{
			model = ent.tryGetComponentFromParent<Model>();
			mesh  = model->resource->find(name);
		}
	}

	auto MeshRenderer::isActive() const -> bool
	{
		return mesh ? mesh->isActive() : false;
	}

	Model::Model(const std::string &file) :
	    filePath(file)
	{
		resource = Application::getCache()->emplace<MeshResource>(file);
	}

	auto Model::load() -> void
	{
		if (type == PrimitiveType::File)
		{
			resource = Application::getCache()->emplace<MeshResource>(filePath);
		}
	}

};        // namespace maple
