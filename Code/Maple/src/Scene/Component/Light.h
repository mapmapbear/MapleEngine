//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Component.h"
#include <glm/glm.hpp>
namespace maple
{
	enum class LightType
	{
		DirectionalLight = 0,
		SpotLight        = 1,
		PointLight       = 2,
	};

	struct LightData
	{
		glm::vec4 color;        // 16
		glm::vec4 position;
		glm::vec4 direction;
		//align to 16 bytes
		float intensity;
		float radius;
		float type;
		float angle;
	};

	class MAPLE_EXPORT Light : public Component
	{
	  public:
		constexpr static char *ICON = ICON_MDI_LIGHTBULB;
		// Construct.
		Light(const glm::vec3 &direction = glm::vec3(0.0f), const glm::vec4 &color = glm::vec4(1.0f), float intensity = 1.0f, const LightType &type = LightType::DirectionalLight, const glm::vec3 &position = glm::vec3(), float radius = 10.0f, float angle = 0.0f);
		auto onImGui() -> void;

		LightData lightData;
		bool      showFrustum = false;

		template <class Archive>
		inline auto serialize(Archive &archive) -> void
		{
			archive(
			    lightData.color,
			    lightData.position,
			    lightData.direction,
			    lightData.radius,
			    lightData.intensity,
			    lightData.type,
			    lightData.angle,
			    entity);
		}
	};
};        // namespace maple
