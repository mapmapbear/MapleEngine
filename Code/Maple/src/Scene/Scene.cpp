//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene.h"
#include "Entity/Entity.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/SystemBuilder.inl"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/LightProbe.h"
#include "Scene/System/ExecutePoint.h"

#include "2d/Sprite.h"
#include "Scripts/Mono/MonoSystem.h"
#include "Scripts/Mono/MonoComponent.h"

#include "FileSystem/Skeleton.h"
#include "Loaders/Loader.h"

#include "Devices/Input.h"
#include "Engine/Camera.h"
#include "Engine/CameraController.h"
#include "Engine/Material.h"
#include "Engine/Profiler.h"
#include "Engine/Mesh.h"

#include "Others/Serialization.h"
#include "Others/StringUtils.h"

#include "Others/Console.h"
#include "Scripts/Mono/MonoSystem.h"
#include <filesystem>
#include <fstream>

#include <ecs/ecs.h>

#include "Application.h"

namespace maple
{
	namespace
	{
		inline auto addEntity(Scene* scene, Entity parent, Skeleton* skeleton, int32_t idx, std::vector<Entity> & outEntities) -> Entity
		{
			auto& bone = skeleton->getBone(idx);
			auto entity = scene->createEntity(bone.name);
			auto & transform = entity.addComponent<component::Transform>();
			auto& boneComp = entity.addComponent<component::BoneComponent>();

			transform.setOffsetTransform(bone.offsetMatrix);
			transform.setLocalTransform(bone.localTransform);
			boneComp.boneIndex = idx;
			boneComp.skeleton = skeleton;

			entity.setParent(parent);

			for (auto child : bone.children)
			{
				addEntity(scene, entity, skeleton, child,outEntities);
			}
			outEntities.emplace_back(entity);
			return entity;
		}

	}

	Scene::Scene(const std::string &initName) :
	    name(initName)
	{
	}

	auto Scene::setSize(uint32_t w, uint32_t h) -> void
	{
		width  = w;
		height = h;
	}

	auto Scene::saveTo(const std::string &path, bool binary) -> void
	{
		PROFILE_FUNCTION();
		if (dirty)
		{
			LOGV("save to disk");
			if (path != "" && path != filePath)
			{
				filePath = path + StringUtils::delimiter + name + ".scene";
			}
			if (filePath == "")
			{
				filePath = name + ".scene";
			}
			Serialization::serialize(this);
			dirty = false;
		}
	}

	auto Scene::loadFrom() -> void
	{
		PROFILE_FUNCTION();
		if (filePath != "")
		{
			Application::getExecutePoint()->clear();
			hierarchy::disconnectOnConstruct(Application::getExecutePoint(), true);
			Serialization::loadScene(this, filePath);
			hierarchy::disconnectOnConstruct(Application::getExecutePoint(), false);
		}
	}

	auto Scene::createEntity() -> Entity
	{
		PROFILE_FUNCTION();
		dirty       = true;
		auto entity = Application::getExecutePoint()->create();
		if (onEntityAdd)
			onEntityAdd(entity);
		return entity;
	}

	auto Scene::createEntity(const std::string &name) -> Entity
	{
		PROFILE_FUNCTION();
		dirty          = true;
		int32_t i      = 0;
		auto    entity = Application::getExecutePoint()->getEntityByName(name);
		while (entity.valid())
		{
			entity = Application::getExecutePoint()->getEntityByName(name + "(" + std::to_string(i + 1) + ")");
			i++;
		}
		auto newEntity = Application::getExecutePoint()->create(i == 0 ? name : name + "(" + std::to_string(i) + ")");
		if (onEntityAdd)
			onEntityAdd(newEntity);
		return newEntity;
	}

	auto Scene::duplicateEntity(const Entity &entity, const Entity &parent) -> void
	{
		PROFILE_FUNCTION();
		dirty = true;

		Entity newEntity = Application::getExecutePoint()->create();

		if (parent)
			newEntity.setParent(parent);

		copyComponents(entity, newEntity);
	}

	auto Scene::duplicateEntity(const Entity &entity) -> void
	{
		PROFILE_FUNCTION();

		dirty            = true;
		Entity newEntity = Application::getExecutePoint()->create();
		//COPY
		copyComponents(entity, newEntity);
	}

	auto Scene::getCamera() -> std::pair<Camera *, component::Transform *>
	{
		PROFILE_FUNCTION();

		using CameraQuery = ecs::Chain
			::Write<Camera>
			::Write<component::Transform>
			::To<ecs::Query>;

		CameraQuery query{ 
			Application::getExecutePoint()->getRegistry(), 
			Application::getExecutePoint()->getGlobalEntity()
		};

		if (useSceneCamera)
		{
			for (auto entity : query)
			{
				auto & [sceneCam, sceneCamTr] = query.convert(entity);
				return { &sceneCam, &sceneCamTr };
			}
		}
		return {overrideCamera, overrideTransform};
	}

	auto Scene::removeAllChildren(entt::entity entity) -> void
	{
		PROFILE_FUNCTION();
		Application::getExecutePoint()->removeAllChildren(entity);
	}

	auto Scene::calculateBoundingBox() -> void
	{
		PROFILE_FUNCTION();
		boxDirty = false;

		sceneBox.clear();
		
		using Query = ecs::Chain
			::Write<component::MeshRenderer>
			::To<ecs::Query>;

		Query query(Application::getExecutePoint()->getRegistry(), Application::getExecutePoint()->getGlobalEntity());

		for (auto entity : query)
		{
			auto [meshRender] = query.convert(entity);
			if (auto mesh = meshRender.mesh)
			{
				if (mesh->isActive())
					sceneBox.merge(mesh->getBoundingBox());
			}
		}
		auto & aabb = Application::getExecutePoint()->getGlobalComponent<component::BoundingBoxComponent>();
		aabb.box = &sceneBox;
	}

	auto Scene::onMeshRenderCreated() -> void
	{
		boxDirty = true;
	}

	auto Scene::addMesh(const std::string& file) -> Entity
	{
		PROFILE_FUNCTION();
		auto name = StringUtils::getFileNameWithoutExtension(file);
		auto modelEntity = createEntity(name);

		std::vector<std::shared_ptr<IResource>> resources;
		Loader::load(file, resources);

		bool hasSkeleton = std::find_if(resources.begin(), resources.end(), [](auto & res) {
			return res->getResourceType() == FileType::Skeleton;
		}) != resources.end();

		for (auto& res : resources)
		{
			if (res->getResourceType() == FileType::Skeleton)
			{
				auto skeleton = std::static_pointer_cast<Skeleton>(res);
				skeleton->buildRoot();

				std::vector<Entity> outEntities;
				auto rootEntity = addEntity(this, modelEntity, skeleton.get(), skeleton->getRoot(), outEntities);

				if (skeleton->isBuildOffset())
				{
					hierarchy::updateTransform(modelEntity, ecs::World{ Application::getExecutePoint()->getRegistry(), entt::null });
					for (auto entity : outEntities)
					{
						auto& transform = entity.getComponent<component::Transform>();
						transform.setOffsetTransform(transform.getWorldMatrixInverse());
					}
				}
			}
			else if(res->getResourceType() == FileType::Model)
			{
				for (auto mesh : std::static_pointer_cast<MeshResource>(res)->getMeshes())
				{
					auto child = createEntity(mesh.first);
					if (hasSkeleton) 
					{
						auto& meshRenderer = child.addComponent<component::SkinnedMeshRenderer>();
						meshRenderer.mesh = mesh.second;
						meshRenderer.meshName = mesh.first;
						meshRenderer.filePath = file;
					}
					else 
					{
						auto& meshRenderer = child.addComponent<component::MeshRenderer>();
						meshRenderer.type = component::PrimitiveType::File;
						meshRenderer.mesh = mesh.second;
						meshRenderer.meshName = mesh.first;
						meshRenderer.filePath = file;
					}
					child.setParent(modelEntity);
				}
			}
		}
		return modelEntity;
	}

	auto Scene::copyComponents(const Entity& from, const Entity& to) -> void
	{
		LOGW("Not implementation {0}", __FUNCTION__);
	}

	auto Scene::onInit() -> void
	{
		PROFILE_FUNCTION();
		if (initCallback != nullptr)
		{
			initCallback(this);
		}
		using MonoQuery = ecs::Chain
			::Write<component::MonoComponent>
			::To<ecs::Query>;

		MonoQuery query{ Application::getExecutePoint()->getRegistry() ,Application::getExecutePoint()->getGlobalEntity() };
		mono::recompile(query);
		mono::callMonoStart(query);

	}

	auto Scene::onClean() -> void
	{
	}
	
	using ControllerQuery = ecs::Chain
		::Write<component::CameraControllerComponent>
		::Write<component::Transform>
		::To<ecs::Query>;

	auto Scene::updateCameraController(float dt) -> void
	{
		PROFILE_FUNCTION();

		ControllerQuery query(Application::getExecutePoint()->getRegistry(), Application::getExecutePoint()->getGlobalEntity());

		for (auto entity : query)
		{
			auto [con, trans] = query.convert(entity);
			const auto mousePos = Input::getInput()->getMousePosition();
			if (Application::get()->isSceneActive() &&
				Application::get()->getEditorState() == EditorState::Play &&
				con.cameraController)
			{
				con.cameraController->handleMouse(trans, dt, mousePos.x, mousePos.y);
				con.cameraController->handleKeyboard(trans, dt);
			}
		}
	}

	auto Scene::onUpdate(float dt) -> void
	{
		PROFILE_FUNCTION();
		auto& deltaTime = Application::getExecutePoint()->getGlobalComponent<component::DeltaTime>();
		deltaTime.dt = dt;
		updateCameraController(dt);
		getBoundingBox();
	}

	auto Scene::create() -> Entity
	{
		auto& registry = Application::getExecutePoint()->getRegistry();

		return Entity(registry.create(), registry);
	}

	auto Scene::create(const std::string& name) -> Entity
	{
		auto& registry = Application::getExecutePoint()->getRegistry();
		auto e = registry.create();
		registry.emplace<component::NameComponent>(e, name);
		return Entity(e, registry);
	}

	namespace
	{
		inline auto meshInOut(component::MeshRenderer& mesh, Entity entity, ecs::World world)
		{
			Application::getCurrentScene()->calculateBoundingBox();
		}
	}
	namespace mesh
	{
		auto registerMeshModule(std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->onConstruct<component::MeshRenderer, &meshInOut>();
			executePoint->onDestory<component::MeshRenderer, &meshInOut>();
		}
	}
};        // namespace maple
