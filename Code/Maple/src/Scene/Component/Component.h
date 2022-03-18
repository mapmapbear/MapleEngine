//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <IconsMaterialDesignIcons.h>
#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace maple
{
	//TODO serialize function is not implementation
	class Entity;
	class TextureCube;
	class Texture;
	class Texture2D;

	namespace component 
	{

		struct DeltaTime 
		{
			constexpr static char* ICON = ICON_MDI_TIMELAPSE;

			float dt;
		};

		class MAPLE_EXPORT Component
		{
		public:
			virtual ~Component() = default;
			Entity       getEntity();
			inline auto& getEntityId() const
			{
				return entity;
			}
			inline auto& getEntityId()
			{
				return entity;
			}
			auto setEntity(entt::entity entity) -> void;

		protected:
			entt::entity entity = entt::null;
		};

		class MAPLE_EXPORT NameComponent : public Component
		{
		public:
			NameComponent() = default;

			NameComponent(const std::string& name) :
				name(name)
			{}
			template <typename Archive>
			auto serialize(Archive& archive) -> void
			{
				archive(cereal::make_nvp("Name", name), entity);
			}
			std::string name;
		};

		class MAPLE_EXPORT StencilComponent : public Component
		{
		public:
			StencilComponent() = default;
		};

		class MAPLE_EXPORT ActiveComponent : public Component
		{
		public:
			ActiveComponent() = default;
			ActiveComponent(bool active) :
				active(active)
			{}
			bool active = true;
			template <typename Archive>
			auto serialize(Archive& archive) -> void
			{
				archive(cereal::make_nvp("Active", active), entity);
			}
		};

		class MAPLE_EXPORT Hierarchy : public Component
		{
		public:
			Hierarchy(entt::entity p);
			Hierarchy();

			inline auto getParent() const
			{
				return parent;
			}
			inline auto getNext() const
			{
				return next;
			}
			inline auto getPrev() const
			{
				return prev;
			}
			inline auto getFirst() const
			{
				return first;
			}

			// Return true if current entity is an ancestor of current entity
			//seems that the entt as a reference could have a bug....
			//TODO
			auto compare(const entt::registry& registry, entt::entity entity) const -> bool;
			auto reset() -> void;

			//delegate method
			// update hierarchy components when hierarchy component is added
			static auto onConstruct(entt::registry& registry, entt::entity entity) -> void;
			static auto onDestroy(entt::registry& registry, entt::entity entity) -> void;
			static auto onUpdate(entt::registry& registry, entt::entity entity) -> void;

			//adjust the parent
			static auto reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy) -> void;

			entt::entity parent = entt::null;
			entt::entity first = entt::null;
			entt::entity next = entt::null;
			entt::entity prev = entt::null;

			template <typename Archive>
			auto serialize(Archive& archive) -> void
			{
				archive(cereal::make_nvp("First", first), cereal::make_nvp("Next", next), cereal::make_nvp("Previous", prev), cereal::make_nvp("Parent", parent), entity);
			}
		};

		class MAPLE_EXPORT Environment : public Component
		{
		public:
			constexpr static char* ICON = ICON_MDI_EARTH;

			static constexpr int32_t IrradianceMapSize = 32;
			static constexpr int32_t PrefilterMapSize = 128;
			Environment();
			Environment(const std::string& filePath);
			auto init(const std::string& filePath) -> void;

			template <class Archive>
			auto save(Archive& archive) const -> void
			{
				archive(filePath, entity, pseudoSky);
			}

			template <class Archive>
			auto load(Archive& archive) -> void
			{
				archive(filePath, entity, pseudoSky);
				init(filePath);
			}

		private:
			std::shared_ptr<Texture2D>   equirectangularMap;
			std::shared_ptr<TextureCube> environment;
			std::shared_ptr<TextureCube> prefilteredEnvironment;
			std::shared_ptr<TextureCube> irradianceMap;

			bool pseudoSky = false;

			uint32_t    numMips = 0;
			uint32_t    width = 0;
			uint32_t    height = 0;
			std::string filePath;

			glm::vec3 skyColorTop = glm::vec3(0.5, 0.7, 0.8) * 1.05f;
			glm::vec3 skyColorBottom = glm::vec3(0.9, 0.9, 0.95);

		public:
			inline auto& getSkyColorBottom()  const
			{
				return skyColorBottom;
			}
			inline auto& getSkyColorTop()const
			{
				return skyColorTop;
			}

			inline auto& getSkyColorBottom()
			{
				return skyColorBottom;
			}
			inline auto& getSkyColorTop()
			{
				return skyColorTop;
			}

			inline auto& getEnvironment() const
			{
				return environment;
			}
			inline auto& getEquirectangularMap() const
			{
				return equirectangularMap;
			}

			inline auto setEnvironmnet(std::shared_ptr<TextureCube> environmnet)
			{
				this->environment = environmnet;
			}

			inline auto& getPrefilteredEnvironment() const
			{
				return prefilteredEnvironment;
			}
			inline auto setPrefilteredEnvironment(std::shared_ptr<TextureCube> prefilteredEnvironment)
			{
				this->prefilteredEnvironment = prefilteredEnvironment;
			}

			inline auto& getIrradianceMap() const
			{
				return irradianceMap;
			}
			inline auto setIrradianceMap(std::shared_ptr<TextureCube> irradianceMap)
			{
				this->irradianceMap = irradianceMap;
			}

			inline auto& getNumMips() const
			{
				return numMips;
			}
			inline auto setNumMips(uint32_t numMips)
			{
				this->numMips = numMips;
			}

			inline auto& getWidth() const
			{
				return width;
			}
			inline auto setWidth(uint32_t width)
			{
				this->width = width;
			}

			inline auto isPseudoSky() const
			{
				return pseudoSky;
			}
			inline auto setPseudoSky(bool pseudoSky)
			{
				this->pseudoSky = pseudoSky;
			}

			inline auto& getHeight() const
			{
				return height;
			}
			inline auto setHeight(uint32_t height)
			{
				this->height = height;
			}

			inline auto& getFilePath() const
			{
				return filePath;
			}
			inline auto setFilePath(std::string filePath)
			{
				this->filePath = filePath;
			}
		};
	}
};        // namespace maple
