//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "Scene.h"
#include "Entity/Entity.h"
#include "Entity/EntityManager.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Sprite.h"
#include "Scene/Component/Transform.h"
#include "Scene/Component/VolumetricCloud.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/LightProbe.h"

#include "FileSystem/Skeleton.h"

#include "SceneGraph.h"

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
		inline auto addEntity(Scene * scene, Entity parent, Skeleton* skeleton, const Bone& bone) -> void
		{
			auto entity = scene->createEntity(bone.name);
			auto & transform = entity.addComponent<component::Transform>();
			transform.setLocalTransform(bone.offsetMatrix);
			entity.setParent(parent);

			for (auto child : bone.children)
			{
				addEntity(scene, entity, skeleton, skeleton->getBones()[child]);
			}
		}
	}

	Scene::Scene(const std::string &initName) :
	    name(initName)
	{
		entityManager = std::make_shared<EntityManager>(this);
		entityManager->addDependency<Camera, component::Transform>();
		entityManager->addDependency<component::Light, component::Transform>();
		entityManager->addDependency<component::MeshRenderer, component::Transform>();
		entityManager->addDependency<component::SkinnedMeshRenderer, component::Transform>();
		entityManager->addDependency<component::Model, component::Transform>();
		entityManager->addDependency<component::Sprite, component::Transform>();
		entityManager->addDependency<component::AnimatedSprite, component::Transform>();
		entityManager->addDependency<component::VolumetricCloud, component::Light>();
		entityManager->addDependency<component::LightProbe, component::Transform>();

		sceneGraph = std::make_shared<SceneGraph>();
		sceneGraph->init(entityManager->getRegistry());
		entityManager->getRegistry().on_construct<component::MeshRenderer>().connect<&Scene::onMeshRenderCreated>(this);

		globalEntity = createEntity("Global");

		getGlobalComponent<component::BoundingBoxComponent>();
		getGlobalComponent<component::DeltaTime>();
	}

	auto Scene::getRegistry() -> entt::registry &
	{
		return entityManager->getRegistry();
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
			entityManager->clear();
			sceneGraph->disconnectOnConstruct(true, getRegistry());
			Serialization::loadScene(this, filePath);
			sceneGraph->disconnectOnConstruct(false, getRegistry());
		}
	}

	auto Scene::createEntity() -> Entity
	{
		PROFILE_FUNCTION();
		dirty       = true;
		auto entity = entityManager->create();
		if (onEntityAdd)
			onEntityAdd(entity);
		return entity;
	}

	auto Scene::createEntity(const std::string &name) -> Entity
	{
		PROFILE_FUNCTION();
		dirty          = true;
		int32_t i      = 0;
		auto    entity = entityManager->getEntityByName(name);
		while (entity.valid())
		{
			entity = entityManager->getEntityByName(name + "(" + std::to_string(i + 1) + ")");
			i++;
		}
		auto newEntity = entityManager->create(i == 0 ? name : name + "(" + std::to_string(i) + ")");
		if (onEntityAdd)
			onEntityAdd(newEntity);
		return newEntity;
	}

	auto Scene::duplicateEntity(const Entity &entity, const Entity &parent) -> void
	{
		PROFILE_FUNCTION();
		dirty = true;

		Entity newEntity = entityManager->create();

		if (parent)
			newEntity.setParent(parent);

		copyComponents(entity, newEntity);
	}

	auto Scene::duplicateEntity(const Entity &entity) -> void
	{
		PROFILE_FUNCTION();

		dirty            = true;
		Entity newEntity = entityManager->create();
		//COPY
		copyComponents(entity, newEntity);
	}

	auto Scene::getCamera() -> std::pair<Camera *, component::Transform *>
	{
		PROFILE_FUNCTION();

		auto camsEttView = getRegistry().group<Camera>(entt::get<component::Transform>);   //entityManager->getEntitiesWithTypes<Camera, Transform>();

		if (!camsEttView.empty() && useSceneCamera)
		{
			Entity     entity(camsEttView.front(), getRegistry());
			Camera &   sceneCam   = entity.getComponent<Camera>();
			component::Transform &sceneCamTr = entity.getComponent<component::Transform>();
			return {&sceneCam, &sceneCamTr};
		}
		return {overrideCamera, overrideTransform};
	}

	auto Scene::removeAllChildren(entt::entity entity) -> void
	{
		PROFILE_FUNCTION();

		entityManager->removeAllChildren(entity);
	}

	auto Scene::calculateBoundingBox() -> void
	{
		PROFILE_FUNCTION();
		boxDirty = false;

		sceneBox.clear();
		
		using Query = ecs::Chain
			::Write<component::MeshRenderer>
			::To<ecs::Query>;

		Query query(entityManager->getRegistry(),globalEntity);

		for (auto entity : query)
		{
			auto [meshRender] = query.convert(entity);
			if (auto mesh = meshRender.getMesh())
			{
				sceneBox.merge(mesh->getBoundingBox());
			}
		}
		auto & aabb = getGlobalComponent<component::BoundingBoxComponent>();
		aabb.box = &sceneBox;
	}

	auto Scene::onMeshRenderCreated() -> void
	{
		boxDirty = true;
	}

	auto Scene::addMesh(const std::string& file) -> Entity
	{
		PROFILE_FUNCTION();


		auto  name = StringUtils::getFileNameWithoutExtension(file);
		auto  modelEntity = createEntity(name);
		auto& model = modelEntity.addComponent<component::Model>(file);
		if (model.resource->getMeshes().size() == 1)
		{
			modelEntity.addComponent<component::MeshRenderer>(model.resource->getMeshes().begin()->second);
		}
		else
		{
			for (auto& mesh : model.resource->getMeshes())
			{
				auto child = createEntity(mesh.first);
				if (model.resource->getSkeleton())
				{
					child.addComponent<component::SkinnedMeshRenderer>(mesh.second);
				}
				else
				{
					child.addComponent<component::MeshRenderer>(mesh.second);
				}
				child.setParent(modelEntity);
			}

			auto skeleton = model.resource->getSkeleton();
			if (skeleton)
			{
				skeleton->buildRoot();
				for (auto child : skeleton->getRoots())
				{
					addEntity(this, modelEntity, skeleton.get(), skeleton->getBones()[child]);
				}
			}
		}
		model.type = component::PrimitiveType::File;
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
		Application::get()->getSystemManager()->getSystem<MonoSystem>()->onStart(this);
	}

	auto Scene::onClean() -> void
	{
	}

	auto Scene::updateCameraController(float dt) -> void
	{
		PROFILE_FUNCTION();
		auto controller = entityManager->getRegistry().group<component::CameraControllerComponent>(entt::get<component::Transform>);
		for (auto entity : controller)
		{
			const auto mousePos = Input::getInput()->getMousePosition();
			auto &[con, trans]  = controller.get<component::CameraControllerComponent, component::Transform>(entity);
			if (Application::get()->isSceneActive() &&
			    Application::get()->getEditorState() == EditorState::Play &&
			    con.getController())
			{
				con.getController()->handleMouse(trans, dt, mousePos.x, mousePos.y);
				con.getController()->handleKeyboard(trans, dt);
			}
		}
	}

	auto Scene::onUpdate(float dt) -> void
	{
		PROFILE_FUNCTION();
		auto& deltaTime = getGlobalComponent<component::DeltaTime>();
		deltaTime.dt = dt;
		updateCameraController(dt);
		getBoundingBox();
		sceneGraph->update(entityManager->getRegistry());
		auto view = entityManager->getRegistry().view<component::AnimatedSprite, component::Transform>();
		for (auto entity : view)
		{
			const auto &[anim, trans] = view.get<component::AnimatedSprite, component::Transform>(entity);
			anim.onUpdate(dt);
		}
	}
};        // namespace maple
