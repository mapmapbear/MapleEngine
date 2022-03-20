
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MeshRenderer.h"
#include "Engine/Mesh.h"
#include "ImGui/ImGuiHelpers.h"
#include "Scene/Component/Transform.h"
#include "Scene/Entity/EntityManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Loaders/Loader.h"
#include "FileSystem/MeshResource.h"

#include "Application.h"

namespace maple
{
	namespace component
	{
		MeshRenderer::MeshRenderer(const std::shared_ptr<Mesh>& mesh) :
			mesh(mesh)
		{
		}

		auto MeshRenderer::getMesh(const std::string& name) -> void
		{
			auto currentScene = Application::get()->getSceneManager()->getCurrentScene();
			Entity ent{ entity, currentScene->getRegistry() };
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
				mesh = model->resource->find(name);
			}
		}

		auto MeshRenderer::isActive() const -> bool
		{
			return mesh ? mesh->isActive() : false;
		}

		Model::Model(const std::string& file) :
			filePath(file)
		{
			type = PrimitiveType::File;
			load();
		}

		auto Model::load() -> void
		{
			if (type == PrimitiveType::File)
			{
				std::vector<std::shared_ptr<IResource>> out;
				Loader::load(filePath, out);
				if (!out.empty())
				{
					for (auto res : out)
					{
						if (res->getResourceType() == FileType::Model)
						{
							resource = std::static_pointer_cast<MeshResource>(res);
						}
					}
				}
			}
		}


//##########################################################################################################################################
		SkinnedMeshRenderer::SkinnedMeshRenderer(const std::shared_ptr<Mesh>& mesh)
			:mesh(mesh)
		{

		}

		auto SkinnedMeshRenderer::getMesh(const std::string& name) -> void
		{
			auto currentScene = Application::get()->getSceneManager()->getCurrentScene();
			Entity ent{ entity, currentScene->getRegistry() };
			auto model = ent.tryGetComponentFromParent<Model>();
			mesh = model->resource->find(name);
		}

		auto SkinnedMeshRenderer::isActive() const -> bool
		{
			return mesh ? mesh->isActive() : false;
		}
	};
};        // namespace maple
