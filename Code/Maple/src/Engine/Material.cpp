
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Material.h"
#include "RHI/DescriptorSet.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/Texture.h"
#include "RHI/UniformBuffer.h"

#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"
#include "Others/Serialization.h"
#include "Others/StringUtils.h"

#include "Application.h"


namespace maple
{
	Material::Material(const std::shared_ptr<Shader> &shader, const MaterialProperties &properties, const PBRMataterialTextures &textures) :
	    pbrMaterialTextures(textures), shader(shader)
	{
		PROFILE_FUNCTION();

		setRenderFlag(RenderFlags::DepthTest);
		setRenderFlag(RenderFlags::DeferredRender);
		setMaterialProperites(properties);
	}

	Material::Material()
	{
		PROFILE_FUNCTION();
		setRenderFlag(RenderFlags::DepthTest);
		setRenderFlag(RenderFlags::DeferredRender);
	}

	Material::Material(const std::string &materialId) :
	    materialId(materialId)
	{
		PROFILE_FUNCTION();
		Serialization::loadMaterial(this, materialId);
		setRenderFlag(RenderFlags::DepthTest);
	}

	Material::~Material()
	{
	}

	auto Material::loadPBRMaterial(const std::string &name, const std::string &path, const std::string &extension) -> void
	{
		PROFILE_FUNCTION();
	}

	auto Material::loadMaterial(const std::string &name, const std::string &path) -> void
	{
		this->name                    = name;
		pbrMaterialTextures.albedo    = Texture2D::create(name, path);
		pbrMaterialTextures.normal    = nullptr;
		pbrMaterialTextures.roughness = nullptr;
		pbrMaterialTextures.metallic  = nullptr;
		pbrMaterialTextures.ao        = nullptr;
		pbrMaterialTextures.emissive  = nullptr;
	}

	auto Material::createDescriptorSet(int32_t layoutID, bool pbr) -> void
	{
		PROFILE_FUNCTION();

	
		if (shader == nullptr)
		{
			if (isFlagOf(RenderFlags::ForwardRender))
			{
				shader = Shader::create("shaders/ForwardPBR.shader");
			}

			if (isFlagOf(RenderFlags::DeferredRender))
			{
				shader = Shader::create("shaders/DeferredColor.shader");
			}

			if (isFlagOf(RenderFlags::ForwardPreviewRender))
			{
				shader = Shader::create("shaders/ForwardPreview.shader");
			}
		}

		DescriptorInfo descriptorDesc;
		descriptorDesc.layoutIndex = layoutID;
		descriptorDesc.shader      = shader.get();

		descriptorSet = DescriptorSet::create(descriptorDesc);
		updateDescriptorSet();
	}

	auto Material::updateDescriptorSet() -> void
	{
		PROFILE_FUNCTION();
		descriptorSet->setTexture("uAlbedoMap", pbrMaterialTextures.albedo);
		descriptorSet->setTexture("uMetallicMap", pbrMaterialTextures.albedo);
		descriptorSet->setTexture("uRoughnessMap", pbrMaterialTextures.albedo);
		descriptorSet->setTexture("uNormalMap", pbrMaterialTextures.albedo);
		descriptorSet->setTexture("uAOMap", pbrMaterialTextures.albedo);
		descriptorSet->setTexture("uEmissiveMap", pbrMaterialTextures.albedo);

		if (pbrMaterialTextures.albedo != nullptr)
		{
			descriptorSet->setTexture("uAlbedoMap", pbrMaterialTextures.albedo);
		}
		else
		{
			descriptorSet->setTexture("uAlbedoMap", Texture2D::getDefaultTexture());
			materialProperties.usingAlbedoMap = 0.0f;
		}

		if (pbrMaterialTextures.metallic)
		{
			descriptorSet->setTexture("uMetallicMap", pbrMaterialTextures.metallic);
		}
		else
		{
			materialProperties.usingMetallicMap = 0.0f;
			descriptorSet->setTexture("uMetallicMap", Texture2D::getDefaultTexture());
		}

		if (pbrMaterialTextures.roughness)
		{
			descriptorSet->setTexture("uRoughnessMap", pbrMaterialTextures.roughness);
		}
		else
		{
			descriptorSet->setTexture("uRoughnessMap", Texture2D::getDefaultTexture());
			materialProperties.usingRoughnessMap = 0.0f;
		}

		if (pbrMaterialTextures.normal != nullptr)
		{
			descriptorSet->setTexture("uNormalMap", pbrMaterialTextures.normal);
		}
		else
		{
			descriptorSet->setTexture("uNormalMap", Texture2D::getDefaultTexture());
			materialProperties.usingNormalMap = 0.0f;
		}

		if (pbrMaterialTextures.ao != nullptr)
		{
			descriptorSet->setTexture("uAOMap", pbrMaterialTextures.ao);
		}
		else
		{
			descriptorSet->setTexture("uAOMap", Texture2D::getDefaultTexture());
			materialProperties.usingAOMap = 0.0f;
		}

		if (pbrMaterialTextures.emissive != nullptr)
		{
			descriptorSet->setTexture("uEmissiveMap", pbrMaterialTextures.emissive);
		}
		else
		{
			descriptorSet->setTexture("uEmissiveMap", Texture2D::getDefaultTexture());
			materialProperties.usingEmissiveMap = 0.0f;
		}

		updateUniformBuffer();
	}

	auto Material::updateUniformBuffer() -> void
	{
		PROFILE_FUNCTION();
		if (!descriptorSet)
			return;

		descriptorSet->setUniformBufferData("UniformMaterialData", &materialProperties);
	}

	auto Material::setMaterialProperites(const MaterialProperties &properties) -> void
	{
		PROFILE_FUNCTION();
		materialProperties = properties;
		updateUniformBuffer();
	}

	auto Material::setTextures(const PBRMataterialTextures &textures) -> void
	{
		PROFILE_FUNCTION();
		pbrMaterialTextures = textures;
	}

	auto Material::setAlbedoTexture(const std::string &path) -> void
	{
		setAlbedo(Texture2D::create(path, path));
	}

	auto Material::setAlbedo(const std::shared_ptr<Texture2D> &texture) -> void
	{
		PROFILE_FUNCTION();
		if (texture)
		{
			pbrMaterialTextures.albedo        = texture;
			materialProperties.usingAlbedoMap = 1.0;
			texturesUpdated                   = true;
		}
	}

	auto Material::setNormalTexture(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		auto tex = Texture2D::create(path, path);
		if (tex)
		{
			pbrMaterialTextures.normal        = tex;
			materialProperties.usingNormalMap = 1.0;
			texturesUpdated                   = true;
		}
	}

	auto Material::setRoughnessTexture(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		auto tex = Texture2D::create(path, path);
		if (tex)
		{
			pbrMaterialTextures.roughness        = tex;
			materialProperties.usingRoughnessMap = 1.0;

			texturesUpdated = true;
		}
	}

	auto Material::setMetallicTexture(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		auto tex = Texture2D::create(path, path);
		if (tex)
		{
			pbrMaterialTextures.metallic        = tex;
			materialProperties.usingMetallicMap = 1.0;

			texturesUpdated = true;
		}
	}

	auto Material::setAOTexture(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		auto tex = Texture2D::create(path, path);
		if (tex)
		{
			pbrMaterialTextures.ao        = tex;
			materialProperties.usingAOMap = 1.0;
			texturesUpdated               = true;
		}
	}

	auto Material::setEmissiveTexture(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		auto tex = Texture2D::create(path, path);
		if (tex)
		{
			pbrMaterialTextures.emissive        = tex;
			materialProperties.usingEmissiveMap = 1.0;
			texturesUpdated                     = true;
		}
	}

	auto Material::bind() -> void
	{
		PROFILE_FUNCTION();

		if (descriptorSet == nullptr)
		{
			createDescriptorSet();
			setTexturesUpdated(false);
		}
		if (texturesUpdated)
		{
			updateDescriptorSet();
		}
		descriptorSet->update();
	}

	auto Material::setShader(const std::string &path) -> void
	{
		PROFILE_FUNCTION();
		if (!StringUtils::endWith(path, "ForwardPreview.shader"))
		{
			shader = Shader::create(path);
		}
	}

	auto Material::setShader(const std::shared_ptr<Shader> &shader) -> void
	{
		PROFILE_FUNCTION();
		this->shader = shader;
	}

	auto Material::getShaderPath() const -> std::string
	{
		return shader ? shader->getFilePath() : "";
	}

	auto Material::create(const std::string &materialId) -> std::shared_ptr<Material>
	{
		return Application::getCache()->emplace<Material>(materialId);
	}

};        // namespace maple
