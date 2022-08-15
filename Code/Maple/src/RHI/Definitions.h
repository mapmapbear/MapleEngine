//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Others/HashCode.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Shader;
	class RenderPass;
	class CommandBuffer;
	class DescriptorSet;
	class Pipeline;
	struct VertexInputDescription;
	struct DescriptorLayoutInfo;
	struct DescriptorPoolInfo;

	class Texture;
	class Texture2D;
	class Texture3D;
	class TextureCube;
	class TextureDepth;
	class TextureDepthArray;
	class FrameBuffer;
	class Mesh;
	class Material;
	class Skeleton;

	enum class TextureType : int32_t;
	enum class TextureFormat;
	enum class TextureType;
	class RenderPass;

	constexpr uint16_t SHADOWMAP_SiZE_MAX = 4096;
	constexpr uint8_t  MAX_RENDER_TARGETS = 8;
	constexpr uint8_t  SHADOWMAP_MAX      = 4;
	constexpr uint8_t  MAX_MIPS           = 32;
	constexpr uint8_t  MAX_TEXTURES       = 16;

	enum class CullMode : int32_t
	{
		Front,
		Back,
		FrontAndBack,
		None
	};

	enum class PolygonMode : int32_t
	{
		Fill,
		Line,
		Point
	};

	enum class BlendMode : int32_t
	{
		None = 0,
		OneZero,
		ZeroSrcColor,
		SrcAlphaOneMinusSrcAlpha,
	};

	enum class TextureWrap : int32_t
	{
		None,
		Repeat,
		Clamp,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder
	};

	enum class TextureFilter : int32_t
	{
		None,
		Linear,
		Nearest
	};

	enum class TextureFormat : int32_t
	{
		NONE,
		R8,
		R16,
		R16F,
		R32F,
		R32I,
		R32UI,
		RG8,
		RG16F,
		RGB8,
		RGBA8,
		RGB16,
		RGBA16,
		RGB32,
		RGBA32,
		RGB,
		RGBA,
		DEPTH,
		STENCIL,
		DEPTH_STENCIL,
		SCREEN,
		LENGTH
	};

	enum class TextureType : int32_t
	{
		Color,
		Color3D,
		Depth,
		DepthArray,
		Cube,
		Color2DArray,
		Other
	};

	struct RenderPassInfo
	{
		std::vector<std::shared_ptr<Texture>> attachments;

		bool clear = true;
	};

	enum class SubPassContents
	{
		Inline,
		Secondary
	};

	struct TextureParameters
	{
		TextureFormat format;
		TextureFilter minFilter;
		TextureFilter magFilter;
		TextureWrap   wrap;
		TextureWrap   wrapT;

		constexpr TextureParameters() :
		    format(TextureFormat::RGBA8),
		    minFilter(TextureFilter::Linear),
		    magFilter(TextureFilter::Linear),
		    wrap(TextureWrap::Repeat),
		    wrapT(TextureWrap::Repeat)
		{
		}

		constexpr TextureParameters(TextureFormat format, TextureFilter filter, TextureWrap wrap) :
		    format(format),
		    minFilter(filter),
		    magFilter(filter),
		    wrap(wrap),
		    wrapT(wrap)
		{
		}

		constexpr TextureParameters(TextureFormat format, TextureFilter filter) :
		    TextureParameters(format, filter, TextureWrap::Repeat)
		{}

		constexpr TextureParameters(TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap) :
		    format(format),
		    minFilter(minFilter),
		    magFilter(magFilter),
		    wrap(wrap),
		    wrapT(TextureWrap::Repeat)
		{
		}

		constexpr TextureParameters(TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrapS, TextureWrap wrapT) :
		    format(TextureFormat::RGBA8),
		    minFilter(minFilter),
		    magFilter(magFilter),
		    wrap(wrapS),
		    wrapT(wrapT)
		{
		}

		constexpr TextureParameters(TextureFilter minFilter, TextureFilter magFilter) :
		    format(TextureFormat::RGBA8),
		    minFilter(minFilter),
		    magFilter(magFilter),
		    wrap(TextureWrap::Clamp),
		    wrapT(TextureWrap::Clamp)
		{
		}

		constexpr TextureParameters(TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap) :
		    format(TextureFormat::RGBA8),
		    minFilter(minFilter),
		    magFilter(magFilter),
		    wrap(wrap),
		    wrapT(wrap)
		{
		}

		constexpr TextureParameters(TextureWrap wrap) :
		    format(TextureFormat::RGBA8),
		    minFilter(TextureFilter::Linear),
		    magFilter(TextureFilter::Linear),
		    wrap(wrap),
		    wrapT(wrap)
		{
		}

		constexpr TextureParameters(TextureFormat format) :
		    format(format),
		    minFilter(TextureFilter::Linear),
		    magFilter(TextureFilter::Linear),
		    wrap(TextureWrap::Clamp),
		    wrapT(TextureWrap::Clamp)
		{
		}

		constexpr TextureParameters(TextureFormat format, TextureWrap wrap) :
		    format(format),
		    minFilter(TextureFilter::Linear),
		    magFilter(TextureFilter::Linear),
		    wrap(wrap),
		    wrapT(wrap)
		{
		}
	};

	struct TextureLoadOptions
	{
		bool flipX;
		bool flipY;
		bool generateMipMaps;
		bool mutableFormat;        // used in vulkan.

		constexpr TextureLoadOptions() :
		    TextureLoadOptions(false, true, false)
		{
		}

		constexpr TextureLoadOptions(bool flipX, bool flipY, bool genMips = false, bool mutableFormat = false) :
		    flipX(flipX),
		    flipY(flipY),
		    generateMipMaps(genMips),
		    mutableFormat(mutableFormat)
		{
		}
	};

	enum class ImageLayout
	{
		Undefined,
		General,
		Color_Attachment_Optimal,
		Depth_Stencil_Attachment_Optimal,
		Depth_Stencil_Read_Only_Optimal,
		Shader_Read_Only_Optimal,
		Transfer_Dst_Optimal,
		Present_Src
	};

	enum RendererBufferType
	{
		RendererBufferNone    = 0,
		RendererBufferColor   = BIT(0),
		RendererBufferDepth   = BIT(1),
		RendererBufferStencil = BIT(2)
	};

	enum class DrawType : int32_t
	{
		Point,
		Triangle,
		Lines,
		TriangleStrip
	};

	enum class StencilType : int32_t
	{
		Equal,
		Notequal,
		Always,
		Never,
		Less,
		LessOrEqual,
		Greater,
		GreaterOrEqual,

		Keep,
		Replace,
		Zero,
	};

	enum class PixelPackType : int32_t
	{
		Pack,
		UnPack
	};

	enum class RendererBlendFunction : int32_t
	{
		None,
		Zero,
		One,
		SourceAlpha,
		DestinationAlpha,
		OneMinusSourceAlpha
	};

	enum class RendererBlendEquation : int32_t
	{
		None,
		Add,
		Subtract
	};

	enum class RenderMode : int32_t
	{
		Fill,
		Wireframe
	};

	enum class DataType : int32_t
	{
		Float,
		UnsignedInt,
		UnsignedByte
	};

	struct PipelineInfo
	{
		std::shared_ptr<Shader> shader;

		bool transparencyEnabled = true;
		bool depthBiasEnabled    = false;
		bool swapChainTarget     = false;
		bool clearTargets        = false;
		bool stencilTest         = false;
		bool depthTest           = true;

		uint32_t    stencilMask = 0x00;
		StencilType stencilFunc = StencilType::Always;

		StencilType stencilFail      = StencilType::Keep;
		StencilType stencilDepthFail = StencilType::Keep;
		StencilType stencilDepthPass = StencilType::Replace;

		CullMode    cullMode    = CullMode::Back;
		PolygonMode polygonMode = PolygonMode::Fill;
		DrawType    drawType    = DrawType::Triangle;
		BlendMode   blendMode   = BlendMode::None;
		glm::vec4   clearColor  = {0.2f, 0.2f, 0.2f, 1.0};

		std::shared_ptr<Texture> depthTarget      = nullptr;
		std::shared_ptr<Texture> depthArrayTarget = nullptr;

		std::array<std::shared_ptr<Texture>, MAX_RENDER_TARGETS> colorTargets = {};

		uint32_t groupCountX = 1;
		uint32_t groupCountY = 1;
		uint32_t groupCountZ = 1;
		uint32_t maxRayRecursionDepth = 2;
		std::string pipelineName;
	};

	struct RenderCommand
	{
		Mesh *    mesh     = nullptr;
		Material *material = nullptr;

		std::shared_ptr<glm::mat4[]> boneTransforms;

		PipelineInfo pipelineInfo;
		PipelineInfo stencilPipelineInfo;

		glm::mat4 transform;
	};

	enum MemoryBarrierFlags : int32_t
	{
		None                        = BIT(0),
		Shader_Image_Access_Barrier = BIT(1),
		Shader_Storage_Barrier      = BIT(2),
		Texture_Fetch_Barrier       = BIT(3),
		General                     = BIT(4)        // mainly for Vulkan
	};
}        // namespace maple

namespace std
{
	template <>
	struct hash<maple::TextureParameters>
	{
		size_t operator()(const maple::TextureParameters &param) const
		{
			size_t seed = 0;
			maple::HashCode::hashCode(seed, param.format, param.magFilter, param.minFilter, param.wrap);
			return seed;
		}
	};

	template <>
	struct hash<maple::TextureLoadOptions>
	{
		size_t operator()(const maple::TextureLoadOptions &param) const
		{
			size_t seed = 0;
			maple::HashCode::hashCode(seed, param.flipX, param.flipY, param.generateMipMaps);
			return seed;
		}
	};
}        // namespace std
