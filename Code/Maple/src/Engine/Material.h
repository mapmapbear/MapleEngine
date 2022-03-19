//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "FileSystem/IResource.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace maple
{
	class Texture2D;
	class Shader;
	class Pipeline;
	class UniformBuffer;
	class DescriptorSet;

	static constexpr float   PBR_WORKFLOW_SEPARATE_TEXTURES  = 0.0f;
	static constexpr float   PBR_WORKFLOW_METALLIC_ROUGHNESS = 1.0f;
	static constexpr float   PBR_WORKFLOW_SPECULAR_GLOSINESS = 2.0f;
	static constexpr int32_t MATERAL_LAYOUT_INDEX            = 1;

	struct MaterialProperties
	{
		glm::vec4 albedoColor       = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 roughnessColor    = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::vec4 metallicColor     = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 emissiveColor     = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		float     usingAlbedoMap    = 1.0f;
		float     usingMetallicMap  = 1.0f;
		float     usingRoughnessMap = 1.0f;
		float     usingNormalMap    = 1.0f;
		float     usingAOMap        = 1.0f;
		float     usingEmissiveMap  = 1.0f;
		float     workflow          = PBR_WORKFLOW_SEPARATE_TEXTURES;

		//padding in vulkan
		float padding = 0.0f;
	};

	struct PBRMataterialTextures
	{
		std::shared_ptr<Texture2D> albedo;
		std::shared_ptr<Texture2D> normal;
		std::shared_ptr<Texture2D> metallic;
		std::shared_ptr<Texture2D> roughness;
		std::shared_ptr<Texture2D> ao;
		std::shared_ptr<Texture2D> emissive;
	};

	class MAPLE_EXPORT Material : public IResource
	{
	  public:
		enum class RenderFlags : uint32_t
		{
			NONE                 = 0,
			DepthTest            = BIT(0),
			Wireframe            = BIT(1),
			ForwardRender        = BIT(2),
			DeferredRender       = BIT(3),
			ForwardPreviewRender = BIT(4),
			NoShadow             = BIT(5),
			TwoSided             = BIT(6),
			AlphaBlend           = BIT(7)
		};

	  protected:
		int32_t renderFlags = 0;

	  public:
		Material(const std::shared_ptr<Shader> &shader, const MaterialProperties &properties = MaterialProperties(), const PBRMataterialTextures &textures = PBRMataterialTextures());
		Material();

		~Material();

		inline auto setRenderFlags(int32_t flags)
		{
			renderFlags = flags;
		}

		inline auto setRenderFlag(Material::RenderFlags flag)
		{
			renderFlags |= static_cast<int32_t>(flag);
		}

		inline auto removeRenderFlag(Material::RenderFlags flag)
		{
			renderFlags &= ~static_cast<int32_t>(flag);
		}

		auto loadPBRMaterial(const std::string &name, const std::string &path, const std::string &extension = ".png") -> void;
		auto loadMaterial(const std::string &name, const std::string &path) -> void;
		/**
		 * layoutID : id in shader layout(set = ?)
		 */
		auto createDescriptorSet(int32_t layoutID = MATERAL_LAYOUT_INDEX, bool pbr = true) -> void;

		auto updateDescriptorSet() -> void;
		auto updateUniformBuffer() -> void;

		auto setMaterialProperites(const MaterialProperties &properties) -> void;
		auto setTextures(const PBRMataterialTextures &textures) -> void;
		auto setAlbedoTexture(const std::string &path) -> void;
		auto setAlbedo(const std::shared_ptr<Texture2D> &texture) -> void;
		auto setNormalTexture(const std::string &path) -> void;
		auto setRoughnessTexture(const std::string &path) -> void;
		auto setMetallicTexture(const std::string &path) -> void;
		auto setAOTexture(const std::string &path) -> void;
		auto setEmissiveTexture(const std::string &path) -> void;

		auto bind() -> void;

		inline auto &isTexturesUpdated() const
		{
			return texturesUpdated;
		}

		inline auto setTexturesUpdated(bool updated)
		{
			texturesUpdated = updated;
		}

		inline auto &getTextures()
		{
			return pbrMaterialTextures;
		}
		inline const auto &getTextures() const
		{
			return pbrMaterialTextures;
		}

		inline auto getShader() const
		{
			return shader;
		}

		inline auto getRenderFlags() const
		{
			return renderFlags;
		}

		inline auto isFlagOf(RenderFlags flag) const -> bool
		{
			return (uint32_t) flag & renderFlags;
		}

		inline auto &getName() const
		{
			return name;
		}

		inline const auto &getProperties() const
		{
			return materialProperties;
		}

		inline auto &getProperties()
		{
			return materialProperties;
		}

		inline auto getDescriptorSet()
		{
			return descriptorSet;
		}

		inline auto &getMaterialId() const
		{
			return materialId;
		}

		auto setShader(const std::string &path) -> void;
		auto setShader(const std::shared_ptr<Shader> &shader) -> void;

		template <typename Archive>
		inline auto save(Archive &archive) const -> void
		{
			archive(
			    cereal::make_nvp("Albedo", pbrMaterialTextures.albedo ? pbrMaterialTextures.albedo->getFilePath() : ""),
			    cereal::make_nvp("Normal", pbrMaterialTextures.normal ? pbrMaterialTextures.normal->getFilePath() : ""),
			    cereal::make_nvp("Metallic", pbrMaterialTextures.metallic ? pbrMaterialTextures.metallic->getFilePath() : ""),
			    cereal::make_nvp("Roughness", pbrMaterialTextures.roughness ? pbrMaterialTextures.roughness->getFilePath() : ""),
			    cereal::make_nvp("AO", pbrMaterialTextures.ao ? pbrMaterialTextures.ao->getFilePath() : ""),
			    cereal::make_nvp("Emissive", pbrMaterialTextures.emissive ? pbrMaterialTextures.emissive->getFilePath() : ""),
			    cereal::make_nvp("albedoColour", materialProperties.albedoColor),
			    cereal::make_nvp("roughnessColour", materialProperties.roughnessColor),
			    cereal::make_nvp("metallicColour", materialProperties.metallicColor),
			    cereal::make_nvp("emissiveColour", materialProperties.emissiveColor),
			    cereal::make_nvp("usingAlbedoMap", materialProperties.usingAlbedoMap),
			    cereal::make_nvp("usingMetallicMap", materialProperties.usingMetallicMap),
			    cereal::make_nvp("usingRoughnessMap", materialProperties.usingRoughnessMap),
			    cereal::make_nvp("usingNormalMap", materialProperties.usingNormalMap),
			    cereal::make_nvp("usingAOMap", materialProperties.usingAOMap),
			    cereal::make_nvp("usingEmissiveMap", materialProperties.usingEmissiveMap),
			    cereal::make_nvp("workflow", materialProperties.workflow),
			    cereal::make_nvp("shader", getShaderPath()),
			    cereal::make_nvp("renderFlags", renderFlags));
		}

		template <typename Archive>
		inline auto load(Archive &archive) -> void
		{
			std::string albedoFilePath;
			std::string normalFilePath;
			std::string roughnessFilePath;
			std::string metallicFilePath;
			std::string emissiveFilePath;
			std::string aoFilePath;
			std::string shaderFilePath;

			archive(cereal::make_nvp("Albedo", albedoFilePath),
			        cereal::make_nvp("Normal", normalFilePath),
			        cereal::make_nvp("Metallic", metallicFilePath),
			        cereal::make_nvp("Roughness", roughnessFilePath),
			        cereal::make_nvp("AO", aoFilePath),
			        cereal::make_nvp("Emissive", emissiveFilePath),
			        cereal::make_nvp("albedoColour", materialProperties.albedoColor),
			        cereal::make_nvp("roughnessColour", materialProperties.roughnessColor),
			        cereal::make_nvp("metallicColour", materialProperties.metallicColor),
			        cereal::make_nvp("emissiveColour", materialProperties.emissiveColor),
			        cereal::make_nvp("usingAlbedoMap", materialProperties.usingAlbedoMap),
			        cereal::make_nvp("usingMetallicMap", materialProperties.usingMetallicMap),
			        cereal::make_nvp("usingRoughnessMap", materialProperties.usingRoughnessMap),
			        cereal::make_nvp("usingNormalMap", materialProperties.usingNormalMap),
			        cereal::make_nvp("usingAOMap", materialProperties.usingAOMap),
			        cereal::make_nvp("usingEmissiveMap", materialProperties.usingEmissiveMap),
			        cereal::make_nvp("workflow", materialProperties.workflow),
			        cereal::make_nvp("shader", shaderFilePath),
			        cereal::make_nvp("renderFlags", renderFlags));

			if (!shaderFilePath.empty())
				setShader(shaderFilePath);

			if (!albedoFilePath.empty())
				pbrMaterialTextures.albedo = Texture2D::create("albedo", albedoFilePath);
			if (!normalFilePath.empty())
				pbrMaterialTextures.normal = Texture2D::create("roughness", normalFilePath);
			if (!metallicFilePath.empty())
				pbrMaterialTextures.metallic = Texture2D::create("metallic", metallicFilePath);
			if (!roughnessFilePath.empty())
				pbrMaterialTextures.roughness = Texture2D::create("roughness", roughnessFilePath);
			if (!emissiveFilePath.empty())
				pbrMaterialTextures.emissive = Texture2D::create("emissive", emissiveFilePath);
			if (!aoFilePath.empty())
				pbrMaterialTextures.ao = Texture2D::create("ao", aoFilePath);
		}

		auto getShaderPath() const -> std::string;

		inline auto getResourceType() const -> FileType
		{
			return FileType::Material;
		}

		inline auto getPath() const -> std::string
		{
			return materialId;
		}

		static auto create(const std::string &materialId) -> std::shared_ptr<Material>;
		Material(const std::string &materialId);

	  private:
		PBRMataterialTextures pbrMaterialTextures;
		MaterialProperties    materialProperties;

		std::shared_ptr<Shader>        shader;
		std::shared_ptr<UniformBuffer> materialPropertiesBuffer;
		std::string                    name;
		std::string                    materialId;
		std::shared_ptr<DescriptorSet> descriptorSet;

		bool texturesUpdated = false;
	};
}        // namespace maple
