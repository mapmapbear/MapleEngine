//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "FBXLoader.h"
#include "FileSystem/Skeleton.h"
#include "FileSystem/MeshResource.h"

#include "Animation/Animation.h"
#include "Others/StringUtils.h"
#include "Others/Console.h"
#include "Engine/Core.h"
#include "Engine/Material.h"
#include "Scene/Component/Transform.h"
#include "Math/BoundingBox.h"
#include "FileSystem/File.h"
#include "Math/MathUtils.h"

#include <vector>
#include <mio.hpp>
#include <ofbx.h>

namespace maple
{
	enum class Orientation
	{
		Y_UP,
		Z_UP,
		Z_MINUS_UP,
		X_MINUS_UP,
		X_UP
	};

	namespace
	{
		static auto operator-(const ofbx::Vec3& a, const ofbx::Vec3& b) -> ofbx::Vec3
		{
			return { a.x - b.x, a.y - b.y, a.z - b.z };
		}

		static auto operator-(const ofbx::Vec2& a, const ofbx::Vec2& b) -> ofbx::Vec2
		{
			return { a.x - b.x, a.y - b.y };
		}

		inline auto getBonePath(const ofbx::Object* bone)->std::string
		{
			if (bone == nullptr) {
				return "";
			}
			return getBonePath(bone->getParent()) + "/" + bone->name;
		}


		inline auto toGlm(const ofbx::Vec2& vec)
		{
			return glm::vec2(float(vec.x), float(vec.y));
		}

		inline auto toGlm(const ofbx::Vec3& vec)
		{
			return glm::vec3(float(vec.x), float(vec.y), float(vec.z));
		}

		inline auto toGlm(const ofbx::Vec4& vec)
		{
			return glm::vec4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
		}

		inline auto toGlm(const ofbx::Color& vec)
		{
			return glm::vec4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
		}

		inline auto fixOrientation(const glm::vec3& v, Orientation orientation) -> glm::vec3
		{
			switch (orientation)
			{
			case Orientation::Y_UP:
				return glm::vec3(v.x, v.y, v.z);
			case Orientation::Z_UP:
				return glm::vec3(v.x, v.z, -v.y);
			case Orientation::Z_MINUS_UP:
				return glm::vec3(v.x, -v.z, v.y);
			case Orientation::X_MINUS_UP:
				return glm::vec3(v.y, -v.x, v.z);
			case Orientation::X_UP:
				return glm::vec3(-v.y, v.x, v.z);
			}
			return v;
		}

		inline auto fixOrientation(const glm::quat& v, Orientation orientation) -> glm::quat
		{
			switch (orientation)
			{
			case Orientation::Y_UP:
				return { v.w,v.x, v.y, v.z };
			case Orientation::Z_UP:
				return { v.w,v.x,-v.y, v.z };
			case Orientation::Z_MINUS_UP:
				return { v.w,v.x, -v.z, v.y };
			case Orientation::X_MINUS_UP:
				return { v.w, -v.x, v.z, v.y };
			case Orientation::X_UP:
				return { -v.y, v.x, v.z, v.w };
			}
			return v;
		}

		inline auto computeTangents(ofbx::Vec3* out, int32_t vertexCount, const ofbx::Vec3* vertices, const ofbx::Vec3* normals, const ofbx::Vec2* uvs)
		{
			for (int i = 0; i < vertexCount; i += 3)
			{
				const auto& v0 = vertices[i + 0];
				const auto& v1 = vertices[i + 1];
				const auto& v2 = vertices[i + 2];
				const auto& uv0 = uvs[i + 0];
				const auto& uv1 = uvs[i + 1];
				const auto& uv2 = uvs[i + 2];

				const ofbx::Vec3 dv10 = v1 - v0;
				const ofbx::Vec3 dv20 = v2 - v0;
				const ofbx::Vec2 duv10 = uv1 - uv0;
				const ofbx::Vec2 duv20 = uv2 - uv0;

				const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
				ofbx::Vec3 tangent;
				tangent.x = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
				tangent.y = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
				tangent.z = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
				const float l = 1 / sqrtf(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
				tangent.x *= l;
				tangent.y *= l;
				tangent.z *= l;
				out[i + 0] = tangent;
				out[i + 1] = tangent;
				out[i + 2] = tangent;
			}
		}

		inline auto toMatrix(const ofbx::Matrix& mat) 
		{
			glm::mat4  result;
			for (int32_t i = 0; i < 4; i++)
				for (int32_t j = 0; j < 4; j++)
					result[i][j] = (float)mat.m[i * 4 + j];
			return result;
		}

		inline auto getTransform(const ofbx::Object* object, Orientation orientation) -> component::Transform
		{
			component::Transform transform;
			ofbx::Vec3 p = object->getLocalTranslation();
			transform.setLocalPosition(fixOrientation({ static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z) }, orientation));
			ofbx::Vec3 r = object->getLocalRotation();
			transform.setLocalOrientation(fixOrientation(glm::vec3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)), orientation));
			ofbx::Vec3 s = object->getLocalScaling();
			transform.setLocalScale({ static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z) });

			if (object->getParent()) 
			{
				transform.setWorldMatrix(getTransform(object->getParent(), orientation).getWorldMatrix());
			}
			else 
			{
				transform.setWorldMatrix(glm::mat4(1));
			}
			return transform;
		}

		inline auto loadTexture(const ofbx::Material* material, ofbx::Texture::TextureType type) -> std::shared_ptr<Texture2D>
		{
			const ofbx::Texture* ofbxTexture = material->getTexture(type);
			std::shared_ptr<Texture2D> texture2D;
			if (ofbxTexture)
			{
				ofbx::DataView filename = ofbxTexture->getRelativeFileName();
				if (filename == "")
					filename = ofbxTexture->getFileName();

				char filePath[256];
				filename.toString(filePath);

				if (File::fileExists(filePath))
				{
					texture2D = Texture2D::create(filePath, filePath);
				}
				else 
				{
					LOGW("file {0} did not find", filePath);
				}
			}

			return texture2D;
		}

		inline auto loadMaterial(const ofbx::Material* material, bool animated) 
		{
			auto pbrMaterial = std::make_shared<Material>();

			PBRMataterialTextures textures;
			MaterialProperties properties;

			properties.albedoColor = toGlm(material->getDiffuseColor());
			properties.metallicColor = toGlm(material->getSpecularColor());

			float roughness = 1.0f - std::sqrt(float(material->getShininess()) / 100.0f);
			properties.roughnessColor = glm::vec4(roughness);

			textures.albedo = loadTexture(material, ofbx::Texture::TextureType::DIFFUSE);
			textures.normal = loadTexture(material, ofbx::Texture::TextureType::NORMAL);
			textures.metallic = loadTexture(material, ofbx::Texture::TextureType::SPECULAR);
			textures.roughness = loadTexture(material, ofbx::Texture::TextureType::SHININESS);
			textures.emissive = loadTexture(material, ofbx::Texture::TextureType::EMISSIVE);
			textures.ao = loadTexture(material, ofbx::Texture::TextureType::AMBIENT);

			if (!textures.albedo)
				properties.usingAlbedoMap = 0.0f;
			if (!textures.normal)
				properties.usingNormalMap = 0.0f;
			if (!textures.metallic)
				properties.usingMetallicMap = 0.0f;
			if (!textures.roughness)
				properties.usingRoughnessMap = 0.0f;
			if (!textures.emissive)
				properties.usingEmissiveMap = 0.0f;
			if (!textures.ao)
				properties.usingAOMap = 0.0f;

			pbrMaterial->setTextures(textures);
			pbrMaterial->setMaterialProperites(properties);

			return pbrMaterial;
		}

		inline auto getOffsetMatrix(const ofbx::Mesh* mesh, const ofbx::Object* node) -> glm::mat4
		{
			auto* skin = mesh ? mesh->getGeometry()->getSkin() : nullptr;
			if (skin)
			{
				for (int i = 0, c = skin->getClusterCount(); i < c; i++)
				{
					const ofbx::Cluster* cluster = skin->getCluster(i);
					if (cluster->getLink() == node)
					{
						return toMatrix(cluster->getTransformLinkMatrix());
					}
				}
			}
			return toMatrix(node->getGlobalTransform());
		}

		inline auto loadBones(const ofbx::Object* parent, Skeleton * skeleton, int32_t parentId, std::vector<const ofbx::Object*> & sceneBones) -> void
		{
			int32_t i = 0;
			while (const ofbx::Object* child = parent->resolveObjectLink(i++))
			{
				if (child->getType() == ofbx::Object::Type::LIMB_NODE)
				{
					auto boneIndex = skeleton->getBoneIndex(child->name);//create a bone if missing
					if (boneIndex == -1)
					{
						auto& newBone = skeleton->createBone();
						newBone.name = child->name;
						newBone.offsetMatrix = toMatrix(child->getLocalTransform());
						boneIndex = newBone.id;
						newBone.parentIdx = parentId;
						auto& parent = skeleton->getBones()[parentId];
						parent.children.emplace_back(boneIndex);
						sceneBones.emplace_back(child);
						loadBones(child, skeleton, boneIndex, sceneBones);
					}
				}
			}
		}

		inline auto loadWeight(const ofbx::Skin* skin, Mesh* mesh, Skeleton * skeleton) -> void
		{
			if(skeleton != nullptr)
			{
				for (int32_t clusterIndex = 0; clusterIndex < skin->getClusterCount(); clusterIndex++)
				{
					const ofbx::Cluster* cluster = skin->getCluster(clusterIndex);
					if (cluster->getIndicesCount() == 0)
						continue;

					const auto link = cluster->getLink();

					const int32_t boneIndex = skeleton->getBoneIndex(link->name);
					if (boneIndex == -1)
					{
						LOGC("Missing bone");
						return;
					}
					const int* clusterIndices = cluster->getIndices();
					const double* clusterWeights = cluster->getWeights();
					for (int32_t j = 0; j < cluster->getIndicesCount(); j++)
					{
						int32_t vtxIndex = clusterIndices[j];
						float vtxWeight = (float)clusterWeights[j];

						if (vtxWeight <= 0 || vtxIndex < 0)
							continue;

						auto& indices = mesh->getBlendIndices(vtxIndex);
						auto& weights = mesh->getBlendWeights(vtxIndex);

						for (int32_t k = 0; k < 4; k++)
						{
							if (vtxWeight >= weights[k])
							{
								for (int32_t l = 2; l >= k; l--)
								{
									indices[l + 1] = indices[l];
									weights[l + 1] = weights[l];
								}

								indices[k] = boneIndex;
								weights[k] = vtxWeight;
								break;
							}
						}
					}
				}
			}
		}

		inline auto print(Skeleton * skeleton,const Bone& bone,int32_t level) -> void
		{
			std::string str;
			for (auto i = 0;i<level;i++)
			{
				str += "--";
			}
			LOGW("{0} ,{1}",str,bone.name);
			for (auto child : bone.children)
			{
				print(skeleton, skeleton->getBones()[child], level + 1);
			}
		}

		inline auto getCurveData(AnimationCurveProperty& curve, const ofbx::AnimationCurve* node)
		{
			if (node != nullptr)
			{
				auto times = node->getKeyTime();
				for (auto i = 0; i < node->getKeyCount(); i++)
				{
					curve.curve.addKey(ofbx::fbxTimeToSeconds(times[i]), node->getKeyValue()[i], 1, 1);
				}
			}
		}


		inline auto loadClip(const ofbx::IScene* scene, int32_t index, float frameRate, const std::vector<const ofbx::Object*>& sceneBones)  -> std::shared_ptr<AnimationClip>
		{
			const ofbx::AnimationStack* stack = scene->getAnimationStack(index);
			const ofbx::AnimationLayer* layer = stack->getLayer(0);
			const ofbx::TakeInfo* takeInfo = scene->getTakeInfo(stack->name);
			if (takeInfo == nullptr)
				return nullptr;

			const double localDuration = takeInfo->local_time_to - takeInfo->local_time_from;
			if (localDuration <= MathUtils::EPS)
				return nullptr;
			auto animationDuration = (int32_t)(localDuration * frameRate + 0.5f);

			auto clip = std::make_shared<AnimationClip>();

			clip->wrapMode = AnimationWrapMode::Loop;
			clip->length = localDuration;//animationDuration;

			char name[256];
			takeInfo->name.toString(name);
			clip->name = name;
			
			// Import curves
			for (int32_t i = 0; i < sceneBones.size(); i++)
			{
				const ofbx::AnimationCurveNode* translationNode = layer->getCurveNode(*sceneBones[i], "Lcl Translation");
				const ofbx::AnimationCurveNode* rotationNode = layer->getCurveNode(*sceneBones[i], "Lcl Rotation");

				if (rotationNode)
				{
					auto& curve0 = clip->curves.emplace_back();
					curve0.path = getBonePath(sceneBones[i]);
					auto& cur0 = curve0.properties.emplace_back();
					getCurveData(cur0, rotationNode->getCurve(0));
					cur0.type = AnimationCurvePropertyType::LocalRotationX;
					auto& cur1 = curve0.properties.emplace_back();
					getCurveData(cur1, rotationNode->getCurve(1));
					cur1.type = AnimationCurvePropertyType::LocalRotationY;
					auto& cur2 = curve0.properties.emplace_back();
					getCurveData(cur2, rotationNode->getCurve(2));
					cur2.type = AnimationCurvePropertyType::LocalRotationZ;
				}

				if (translationNode)
				{
					auto& curve = clip->curves.emplace_back();
					curve.path = getBonePath(sceneBones[i]);
					auto& cur0 = curve.properties.emplace_back();
					getCurveData(cur0, translationNode->getCurve(0));
					cur0.type = AnimationCurvePropertyType::LocalPositionX;
					auto& cur1 = curve.properties.emplace_back();
					getCurveData(cur1, translationNode->getCurve(1));
					cur1.type = AnimationCurvePropertyType::LocalPositionY;
					auto& cur2 = curve.properties.emplace_back();
					getCurveData(cur2, translationNode->getCurve(2));
					cur2.type = AnimationCurvePropertyType::LocalPositionZ;
				}
			}
			return clip;
		}

		inline auto loadAnimation(const std::string& fileName, const ofbx::IScene* scene, const std::vector<const ofbx::Object*> & sceneBones, std::vector<std::shared_ptr<IResource>>& outRes)
		{
			float frameRate = scene->getSceneFrameRate();
			if (frameRate <= 0)
				frameRate = 30.0f;

			auto animation = std::make_shared<Animation>(fileName);

			const int32_t animCount = scene->getAnimationStackCount();
			for (int32_t animIndex = 0; animIndex < animCount; animIndex++)
			{
				auto clip = loadClip(scene, animIndex, frameRate, sceneBones);
				if (clip != nullptr) 
				{
					animation->addClip(clip);
				}
			}
			if (animCount > 0) 
			{
				outRes.emplace_back(animation);
			}
		}
	}

	auto FBXLoader::load(const std::string& fileName, const std::string& extension, std::vector<std::shared_ptr<IResource>>& outRes) -> void
	{
		mio::mmap_source mmap(fileName);
		MAPLE_ASSERT(mmap.is_open(), "open fbx file failed");

		constexpr bool ignoreGeometry = false;
		const uint64_t flags = ignoreGeometry ? (uint64_t)ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t)ofbx::LoadFlags::TRIANGULATE;
		auto scene = ofbx::load((uint8_t*)mmap.data(), mmap.size(), flags);

		const ofbx::GlobalSettings* settings = scene->getGlobalSettings();

		Orientation orientation = Orientation::Y_UP;

		switch (settings->UpAxis)
		{
		case ofbx::UpVector_AxisX:
			orientation = Orientation::X_UP;
			break;
		case ofbx::UpVector_AxisY:
			orientation = Orientation::Y_UP;
			break;
		case ofbx::UpVector_AxisZ:
			orientation = Orientation::Z_UP;
			break;
		}

		int32_t i = 0;
		std::vector<const ofbx::Object*> sceneBone;
		auto skeleton = std::make_shared<Skeleton>(fileName);
		while (const ofbx::Object* bone = scene->getRoot()->resolveObjectLink(i++))
		{
			if (bone->getType() == ofbx::Object::Type::LIMB_NODE)
			{
				auto boneIndex = skeleton->getBoneIndex(bone->name);//create a bone if missing
				if (boneIndex == -1)
				{
					auto& newBone = skeleton->createBone();
					newBone.name = bone->name;
					newBone.offsetMatrix = toMatrix(bone->getLocalTransform());
					boneIndex = newBone.id;
					sceneBone.emplace_back(bone);
				}
				loadBones(bone, skeleton.get(), boneIndex, sceneBone);
			}
		}

		loadAnimation(fileName,scene, sceneBone, outRes);

		if (!skeleton->getBones().empty())
		{
			outRes.emplace_back(skeleton);
		}

		auto meshes = std::make_shared<MeshResource>(fileName);
		outRes.emplace_back(meshes);
		for (int32_t i = 0; i < scene->getMeshCount(); ++i)
		{
			const auto fbxMesh = (const ofbx::Mesh*)scene->getMesh(i);
			const auto geom = fbxMesh->getGeometry();
			const auto numIndices = geom->getIndexCount();
			const auto vertexCount = geom->getVertexCount();
			const auto vertices = geom->getVertices();
			const auto normals = geom->getNormals();
			const auto tangents = geom->getTangents();
			const auto colors = geom->getColors();
			const auto uvs = geom->getUVs();
			const auto materials = geom->getMaterials();


			std::vector<std::shared_ptr<Material>> pbrMaterials;
			std::vector<Vertex> tempVertices;
			tempVertices.resize(vertexCount);
			std::vector<uint32_t> indicesArray;
			indicesArray.resize(numIndices);
			auto boundingBox = std::make_shared<BoundingBox>();

			const auto indices = geom->getFaceIndices();

			ofbx::Vec3* generatedTangents = nullptr;
			if (!tangents && normals && uvs)
			{
				generatedTangents = new ofbx::Vec3[vertexCount];
				computeTangents(generatedTangents, vertexCount, vertices, normals, uvs);
			}

			auto transform = getTransform(fbxMesh,orientation);

			for (int32_t i = 0; i < vertexCount; ++i)
			{
				const ofbx::Vec3& cp = vertices[i];
				auto& vertex = tempVertices[i];
				vertex.pos = transform.getWorldMatrix() * glm::vec4(float(cp.x), float(cp.y), float(cp.z),1.0);
				fixOrientation(vertex.pos,orientation);
				boundingBox->merge(vertex.pos);

				if (normals)
				{
					glm::mat3 matrix(transform.getWorldMatrix());
					vertex.normal = glm::transpose(glm::inverse(matrix)) * glm::normalize(glm::vec3{ float(normals[i].x), float(normals[i].y), float(normals[i].z) });
				}
				if (uvs)
					vertex.texCoord = { float(uvs[i].x), 1.0f - float(uvs[i].y) };
				if (colors)
					vertex.color = { float(colors[i].x), float(colors[i].y), float(colors[i].z), float(colors[i].w) };
				else 
					vertex.color = { 1,1,1,1 };
				if (tangents)
					vertex.tangent = transform.getWorldMatrix() * glm::vec4(float(tangents[i].x), float(tangents[i].y), float(tangents[i].z),1.0);

				fixOrientation(vertex.normal, orientation);
				fixOrientation(vertex.tangent, orientation);
			}

			for (int32_t i = 0; i < numIndices; i++)
			{
				int32_t index = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];
				indicesArray[i] = index;
			}

			
			auto mesh = std::make_shared<Mesh>(indicesArray, tempVertices);

			for (auto i = 0;i< fbxMesh->getMaterialCount();i++)
			{
				const ofbx::Material* material = fbxMesh->getMaterial(i);
				pbrMaterials.emplace_back(loadMaterial(material, false));
			}

			mesh->setMaterial(pbrMaterials);
		
			const auto trianglesCount = vertexCount / 3;

			std::vector<uint32_t> subMeshIdx;

			if (fbxMesh->getMaterialCount() > 1) 
			{
				int32_t rangeStart = 0;
				int32_t rangeStartVal = materials[rangeStart];
				for (int32_t triangleIndex = 1; triangleIndex < trianglesCount; triangleIndex++)
				{
					if (rangeStartVal != materials[triangleIndex])
					{
						rangeStartVal = materials[triangleIndex];
						subMeshIdx.emplace_back(materials[triangleIndex] * 3);
					}
				}

				mesh->setSubMeshIndex(subMeshIdx);
				mesh->setSubMeshCount(subMeshIdx.size());

				MAPLE_ASSERT(subMeshIdx.size() == pbrMaterials.size(), "size is not same");
			}
			
			if (geom->getSkin() != nullptr)
			{
				loadWeight(geom->getSkin(), mesh.get(), skeleton.get());
			}

			std::string name = fbxMesh->name;

			MAPLE_ASSERT(name != "", "name should not be null");

			mesh->setName(name);
			meshes->addMesh(name, mesh);

			if (generatedTangents)
				delete[] generatedTangents;
		}
	}
};            // namespace maple
