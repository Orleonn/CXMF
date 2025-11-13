#include "CXMF.hpp"

#include <fstream>
#include <sstream>
#include <format>
#include <optional>
#include <filesystem>
#include <limits>
#include <list>
#include <unordered_map>

#ifdef CXMF_INCLUDE_IMPORTER
	#include "assimp/Importer.hpp"
	#include "assimp/scene.h"
	#include "assimp/material.h"
	#include "assimp/GltfMaterial.h"
	#include "assimp/postprocess.h"
	#include "assimp/Logger.hpp"
	#include "assimp/DefaultLogger.hpp"
	#include "assimp/LogStream.hpp"
	#include "assimp/commonMetaData.h"

	#ifndef AI_MATKEY_LIGHTMAP_FACTOR
		// An undeclared macro for obtaining the ambient occlusion factor
		#define AI_MATKEY_LIGHTMAP_FACTOR "$tex.file.strength", aiTextureType_LIGHTMAP, 0
	#endif

	#define GLM_ENABLE_EXPERIMENTAL
	#include "glm/ext.hpp"

	#include "meshoptimizer.h"
#endif

#include "zlib.h"

#undef min
#undef max



// Read/Write model stuff

#define WRITE_PARAM(Param, Size) stream.write(reinterpret_cast<const char*>(Param), Size)
#define READ_PARAM(Param, Size) stream.read(reinterpret_cast<char*>(Param), Size)



// Vertex

static std::ostream& operator<<(std::ostream& stream, const cxmf::Vertex& vertex)
{
	WRITE_PARAM(&vertex.position[0], sizeof(vertex.position));
	WRITE_PARAM(&vertex.normal[0], sizeof(vertex.normal));
	WRITE_PARAM(&vertex.uv[0], sizeof(vertex.uv));
	WRITE_PARAM(&vertex.tangent[0], sizeof(vertex.tangent));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Vertex& vertex)
{
	READ_PARAM(&vertex.position[0], sizeof(vertex.position));
	READ_PARAM(&vertex.normal[0], sizeof(vertex.normal));
	READ_PARAM(&vertex.uv[0], sizeof(vertex.uv));
	READ_PARAM(&vertex.tangent[0], sizeof(vertex.tangent));
	return stream;
}



// WeightedVertex

static std::ostream& operator<<(std::ostream& stream, const cxmf::WeightedVertex& vertex)
{
	WRITE_PARAM(&vertex.position[0], sizeof(vertex.position));
	WRITE_PARAM(&vertex.normal[0], sizeof(vertex.normal));
	WRITE_PARAM(&vertex.uv[0], sizeof(vertex.uv));
	WRITE_PARAM(&vertex.tangent[0], sizeof(vertex.tangent));
	WRITE_PARAM(&vertex.boneID[0], sizeof(vertex.boneID));
	WRITE_PARAM(&vertex.weight[0], sizeof(vertex.weight));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::WeightedVertex& vertex)
{
	READ_PARAM(&vertex.position[0], sizeof(vertex.position));
	READ_PARAM(&vertex.normal[0], sizeof(vertex.normal));
	READ_PARAM(&vertex.uv[0], sizeof(vertex.uv));
	READ_PARAM(&vertex.tangent[0], sizeof(vertex.tangent));
	READ_PARAM(&vertex.boneID[0], sizeof(vertex.boneID));
	READ_PARAM(&vertex.weight[0], sizeof(vertex.weight));
	return stream;
}



// Texture

static std::ostream& operator<<(std::ostream& stream, const cxmf::Texture& tex)
{
	const uint32_t pathLen = static_cast<uint32_t>(tex.path.length());
	WRITE_PARAM(&pathLen, sizeof(pathLen));
	WRITE_PARAM(tex.path.c_str(), pathLen);
	WRITE_PARAM(&tex.samplerIndex, sizeof(tex.samplerIndex));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Texture& tex)
{
	uint32_t pathLen = 0;
	READ_PARAM(&pathLen, sizeof(pathLen));
	tex.path.resize(pathLen);
	READ_PARAM(tex.path.data(), pathLen);
	READ_PARAM(&tex.samplerIndex, sizeof(tex.samplerIndex));
	return stream;
}



// Sampler

static std::ostream& operator<<(std::ostream& stream, const cxmf::Sampler& sampler)
{
	const uint32_t nameLen = static_cast<uint32_t>(sampler.name.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(sampler.name.c_str(), nameLen);
	WRITE_PARAM(&sampler.magFilter, sizeof(sampler.magFilter));
	WRITE_PARAM(&sampler.minFilter, sizeof(sampler.minFilter));
	WRITE_PARAM(&sampler.mipmapMode, sizeof(sampler.mipmapMode));
	WRITE_PARAM(&sampler.addressModeU, sizeof(sampler.addressModeU));
	WRITE_PARAM(&sampler.addressModeV, sizeof(sampler.addressModeV));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Sampler& sampler)
{
	uint32_t nameLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	sampler.name.resize(nameLen);
	READ_PARAM(sampler.name.data(), nameLen);
	READ_PARAM(&sampler.magFilter, sizeof(sampler.magFilter));
	READ_PARAM(&sampler.minFilter, sizeof(sampler.minFilter));
	READ_PARAM(&sampler.mipmapMode, sizeof(sampler.mipmapMode));
	READ_PARAM(&sampler.addressModeU, sizeof(sampler.addressModeU));
	READ_PARAM(&sampler.addressModeV, sizeof(sampler.addressModeV));
	return stream;
}



// Material

static std::ostream& operator<<(std::ostream& stream, const cxmf::Material& material)
{
	const uint32_t nameLen = static_cast<uint32_t>(material.name.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(material.name.c_str(), nameLen);
	WRITE_PARAM(&material.baseColorFactor[0], sizeof(material.baseColorFactor));
	WRITE_PARAM(&material.roughnessFactor, sizeof(material.roughnessFactor));
	WRITE_PARAM(&material.metallicFactor, sizeof(material.metallicFactor));
	WRITE_PARAM(&material.ambientOcclusionFactor, sizeof(material.ambientOcclusionFactor));
	WRITE_PARAM(&material.emissiveFactor[0], sizeof(material.emissiveFactor));
	WRITE_PARAM(&material.textureIndex, sizeof(material.textureIndex));
	WRITE_PARAM(&material.alphaMode, sizeof(material.alphaMode));
	WRITE_PARAM(&material.alphaCutoff, sizeof(material.alphaCutoff));
	WRITE_PARAM(&material.doubleSided, sizeof(material.doubleSided));
	WRITE_PARAM(&material.shadeless, sizeof(material.shadeless));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Material& material)
{
	uint32_t nameLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	material.name.resize(nameLen);
	READ_PARAM(material.name.data(), nameLen);
	READ_PARAM(&material.baseColorFactor[0], sizeof(material.baseColorFactor));
	READ_PARAM(&material.roughnessFactor, sizeof(material.roughnessFactor));
	READ_PARAM(&material.metallicFactor, sizeof(material.metallicFactor));
	READ_PARAM(&material.ambientOcclusionFactor, sizeof(material.ambientOcclusionFactor));
	READ_PARAM(&material.emissiveFactor[0], sizeof(material.emissiveFactor));
	READ_PARAM(&material.textureIndex, sizeof(material.textureIndex));
	READ_PARAM(&material.alphaMode, sizeof(material.alphaMode));
	READ_PARAM(&material.alphaCutoff, sizeof(material.alphaCutoff));
	READ_PARAM(&material.doubleSided, sizeof(material.doubleSided));
	READ_PARAM(&material.shadeless, sizeof(material.shadeless));
	return stream;
}



// Meshlet

static std::ostream& operator<<(std::ostream& stream, const cxmf::Meshlet& meshlet)
{
	WRITE_PARAM(&meshlet.bounds.center[0], sizeof(meshlet.bounds.center));
	WRITE_PARAM(&meshlet.bounds.radius, sizeof(meshlet.bounds.radius));
	WRITE_PARAM(&meshlet.vertexOffset, sizeof(meshlet.vertexOffset));
	WRITE_PARAM(&meshlet.triangleOffset, sizeof(meshlet.triangleOffset));
	WRITE_PARAM(&meshlet.vertexCount, sizeof(meshlet.vertexCount));
	WRITE_PARAM(&meshlet.triangleCount, sizeof(meshlet.triangleCount));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Meshlet& meshlet)
{
	READ_PARAM(&meshlet.bounds.center[0], sizeof(meshlet.bounds.center));
	READ_PARAM(&meshlet.bounds.radius, sizeof(meshlet.bounds.radius));
	READ_PARAM(&meshlet.vertexOffset, sizeof(meshlet.vertexOffset));
	READ_PARAM(&meshlet.triangleOffset, sizeof(meshlet.triangleOffset));
	READ_PARAM(&meshlet.vertexCount, sizeof(meshlet.vertexCount));
	READ_PARAM(&meshlet.triangleCount, sizeof(meshlet.triangleCount));
	return stream;
}



// Mesh

static std::ostream& operator<<(std::ostream& stream, const cxmf::Mesh& mesh)
{
	const uint32_t nameLen = static_cast<uint32_t>(mesh.name.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(mesh.name.c_str(), nameLen);
	WRITE_PARAM(&mesh.bounds.center[0], sizeof(mesh.bounds.center));
	WRITE_PARAM(&mesh.bounds.radius, sizeof(mesh.bounds.radius));
	WRITE_PARAM(&mesh.vertexOffset, sizeof(mesh.vertexOffset));
	WRITE_PARAM(&mesh.vertexCount, sizeof(mesh.vertexCount));
	WRITE_PARAM(&mesh.meshletOffset, sizeof(mesh.meshletOffset));
	WRITE_PARAM(&mesh.meshletCount, sizeof(mesh.meshletCount));
	WRITE_PARAM(&mesh.materialIndex, sizeof(mesh.materialIndex));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Mesh& mesh)
{
	uint32_t nameLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	mesh.name.resize(nameLen);
	READ_PARAM(mesh.name.data(), nameLen);
	READ_PARAM(&mesh.bounds.center[0], sizeof(mesh.bounds.center));
	READ_PARAM(&mesh.bounds.radius, sizeof(mesh.bounds.radius));
	READ_PARAM(&mesh.vertexOffset, sizeof(mesh.vertexOffset));
	READ_PARAM(&mesh.vertexCount, sizeof(mesh.vertexCount));
	READ_PARAM(&mesh.meshletOffset, sizeof(mesh.meshletOffset));
	READ_PARAM(&mesh.meshletCount, sizeof(mesh.meshletCount));
	READ_PARAM(&mesh.materialIndex, sizeof(mesh.materialIndex));
	return stream;
}



// MeshHierarchy

static std::ostream& operator<<(std::ostream& stream, const cxmf::MeshHierarchy& mhi)
{
	const uint32_t nameLen = static_cast<uint32_t>(mhi.name.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(mhi.name.c_str(), nameLen);
	WRITE_PARAM(mhi.localTransform.Data(), sizeof(mhi.localTransform));
	WRITE_PARAM(&mhi.meshIndex, sizeof(mhi.meshIndex));
	WRITE_PARAM(&mhi.parentIndex, sizeof(mhi.parentIndex));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::MeshHierarchy& mhi)
{
	uint32_t nameLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	mhi.name.resize(nameLen);
	READ_PARAM(mhi.name.data(), nameLen);
	READ_PARAM(mhi.localTransform.Data(), sizeof(mhi.localTransform));
	READ_PARAM(&mhi.meshIndex, sizeof(mhi.meshIndex));
	READ_PARAM(&mhi.parentIndex, sizeof(mhi.parentIndex));
	return stream;
}



// Bone

static std::ostream& operator<<(std::ostream& stream, const cxmf::Bone& bone)
{
	const uint32_t nameLen = static_cast<uint32_t>(bone.name.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(bone.name.c_str(), nameLen);
	WRITE_PARAM(bone.inverseBindTransform.Data(), sizeof(bone.inverseBindTransform));
	WRITE_PARAM(bone.offsetMatrix.Data(), sizeof(bone.offsetMatrix));
	WRITE_PARAM(&bone.parentIndex, sizeof(bone.parentIndex));
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::Bone& bone)
{
	uint32_t nameLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	bone.name.resize(nameLen);
	READ_PARAM(bone.name.data(), nameLen);
	READ_PARAM(bone.inverseBindTransform.Data(), sizeof(bone.inverseBindTransform));
	READ_PARAM(bone.offsetMatrix.Data(), sizeof(bone.offsetMatrix));
	READ_PARAM(&bone.parentIndex, sizeof(bone.parentIndex));
	return stream;
}



// Model

static void writeGenericModelToStream(std::ostream& stream, const cxmf::Model& model)
{
	const uint32_t nameLen = static_cast<uint32_t>(model.name.length());
	const uint32_t texturesCount = static_cast<uint32_t>(model.textures.size());
	const uint32_t samplersCount = static_cast<uint32_t>(model.samplers.size());
	const uint32_t materialsCount = static_cast<uint32_t>(model.materials.size());
	const uint32_t meshesCount = static_cast<uint32_t>(model.meshes.size());
	const uint32_t meshNodesCount = static_cast<uint32_t>(model.meshNodes.size());
	const uint32_t meshletVerticesCount = static_cast<uint32_t>(model.meshletVertices.size());
	const uint32_t meshletTrianglesCount = static_cast<uint32_t>(model.meshletTriangles.size());
	const uint32_t meshletsCount = static_cast<uint32_t>(model.meshlets.size());
	const uint32_t copyrightLen = static_cast<uint32_t>(model.copyright.length());
	const uint32_t generatorLen = static_cast<uint32_t>(model.generator.length());
	WRITE_PARAM(&nameLen, sizeof(nameLen));
	WRITE_PARAM(&texturesCount, sizeof(texturesCount));
	WRITE_PARAM(&samplersCount, sizeof(samplersCount));
	WRITE_PARAM(&materialsCount, sizeof(materialsCount));
	WRITE_PARAM(&meshesCount, sizeof(meshesCount));
	WRITE_PARAM(&meshNodesCount, sizeof(meshNodesCount));
	WRITE_PARAM(&meshletVerticesCount, sizeof(meshletVerticesCount));
	WRITE_PARAM(&meshletTrianglesCount, sizeof(meshletTrianglesCount));
	WRITE_PARAM(&meshletsCount, sizeof(meshletsCount));
	WRITE_PARAM(&copyrightLen, sizeof(copyrightLen));
	WRITE_PARAM(&generatorLen, sizeof(generatorLen));
	WRITE_PARAM(model.name.c_str(), nameLen);
	for (uint32_t i = 0; i < texturesCount; ++i)
		stream << model.textures[i];
	for (uint32_t i = 0; i < samplersCount; ++i)
		stream << model.samplers[i];
	for (uint32_t i = 0; i < materialsCount; ++i)
		stream << model.materials[i];
	for (uint32_t i = 0; i < meshesCount; ++i)
		stream << model.meshes[i];
	for (uint32_t i = 0; i < meshNodesCount; ++i)
		stream << model.meshNodes[i];
	WRITE_PARAM(model.meshletVertices.data(), sizeof(uint32_t) * meshletVerticesCount);
	WRITE_PARAM(model.meshletTriangles.data(), sizeof(uint8_t) * meshletTrianglesCount);
	for (uint32_t i = 0; i < meshletsCount; ++i)
		stream << model.meshlets[i];
	WRITE_PARAM(&model.bounds.center[0], sizeof(model.bounds.center));
	WRITE_PARAM(&model.bounds.radius, sizeof(model.bounds.radius));
	WRITE_PARAM(model.copyright.c_str(), copyrightLen);
	WRITE_PARAM(model.generator.c_str(), generatorLen);
}

static void readGenericModelFromStream(std::istream& stream, cxmf::Model& model)
{
	uint32_t nameLen = 0;
	uint32_t texturesCount = 0;
	uint32_t samplersCount = 0;
	uint32_t materialsCount = 0;
	uint32_t meshesCount = 0;
	uint32_t meshNodesCount = 0;
	uint32_t meshletVerticesCount = 0;
	uint32_t meshletTrianglesCount = 0;
	uint32_t meshletsCount = 0;
	uint32_t copyrightLen = 0;
	uint32_t generatorLen = 0;
	READ_PARAM(&nameLen, sizeof(nameLen));
	READ_PARAM(&texturesCount, sizeof(texturesCount));
	READ_PARAM(&samplersCount, sizeof(samplersCount));
	READ_PARAM(&materialsCount, sizeof(materialsCount));
	READ_PARAM(&meshesCount, sizeof(meshesCount));
	READ_PARAM(&meshNodesCount, sizeof(meshNodesCount));
	READ_PARAM(&meshletVerticesCount, sizeof(meshletVerticesCount));
	READ_PARAM(&meshletTrianglesCount, sizeof(meshletTrianglesCount));
	READ_PARAM(&meshletsCount, sizeof(meshletsCount));
	READ_PARAM(&copyrightLen, sizeof(copyrightLen));
	READ_PARAM(&generatorLen, sizeof(generatorLen));
	model.name.resize(nameLen);
	model.textures.resize(texturesCount);
	model.samplers.resize(samplersCount);
	model.materials.resize(materialsCount);
	model.meshes.resize(meshesCount);
	model.meshNodes.resize(meshNodesCount);
	model.meshletVertices.resize(meshletVerticesCount);
	model.meshletTriangles.resize(meshletTrianglesCount);
	model.meshlets.resize(meshletsCount);
	model.copyright.resize(copyrightLen);
	model.generator.resize(generatorLen);
	READ_PARAM(model.name.data(), nameLen);
	for (uint32_t i = 0; i < texturesCount; ++i)
		stream >> model.textures[i];
	for (uint32_t i = 0; i < samplersCount; ++i)
		stream >> model.samplers[i];
	for (uint32_t i = 0; i < materialsCount; ++i)
		stream >> model.materials[i];
	for (uint32_t i = 0; i < meshesCount; ++i)
		stream >> model.meshes[i];
	for (uint32_t i = 0; i < meshNodesCount; ++i)
		stream >> model.meshNodes[i];
	READ_PARAM(model.meshletVertices.data(), sizeof(uint32_t) * meshletVerticesCount);
	READ_PARAM(model.meshletTriangles.data(), sizeof(uint8_t) * meshletTrianglesCount);
	for (uint32_t i = 0; i < meshletsCount; ++i)
		stream >> model.meshlets[i];
	READ_PARAM(&model.bounds.center[0], sizeof(model.bounds.center));
	READ_PARAM(&model.bounds.radius, sizeof(model.bounds.radius));
	READ_PARAM(model.copyright.data(), copyrightLen);
	READ_PARAM(model.generator.data(), generatorLen);
}



// StaticModel

static std::ostream& operator<<(std::ostream& stream, const cxmf::StaticModel& model)
{
	writeGenericModelToStream(stream, model);

	const uint32_t vertexCount = static_cast<uint32_t>(model.vertices.size());
	WRITE_PARAM(&vertexCount, sizeof(vertexCount));
	for (uint32_t i = 0; i < vertexCount; ++i)
		stream << model.vertices[i];
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::StaticModel& model)
{
	readGenericModelFromStream(stream, model);

	uint32_t vertexCount = 0;
	READ_PARAM(&vertexCount, sizeof(vertexCount));
	model.vertices.resize(vertexCount);
	for (uint32_t i = 0; i < vertexCount; ++i)
		stream >> model.vertices[i];
	return stream;
}



// SkinnedModel

static std::ostream& operator<<(std::ostream& stream, const cxmf::SkinnedModel& model)
{
	writeGenericModelToStream(stream, model);

	const uint32_t vertexCount = static_cast<uint32_t>(model.vertices.size());
	const uint32_t bonesCount = static_cast<uint32_t>(model.bones.size());
	WRITE_PARAM(&vertexCount, sizeof(vertexCount));
	WRITE_PARAM(&bonesCount, sizeof(vertexCount));
	for (uint32_t i = 0; i < vertexCount; ++i)
		stream << model.vertices[i];
	for (uint32_t i = 0; i < bonesCount; ++i)
		stream << model.bones[i];
	return stream;
}
static std::istream& operator>>(std::istream& stream, cxmf::SkinnedModel& model)
{
	readGenericModelFromStream(stream, model);

	uint32_t vertexCount = 0;
	uint32_t bonesCount = 0;
	READ_PARAM(&vertexCount, sizeof(vertexCount));
	READ_PARAM(&bonesCount, sizeof(vertexCount));
	model.vertices.resize(vertexCount);
	model.bones.resize(bonesCount);
	for (uint32_t i = 0; i < vertexCount; ++i)
		stream >> model.vertices[i];
	for (uint32_t i = 0; i < bonesCount; ++i)
		stream >> model.bones[i];
	return stream;
}

#undef WRITE_PARAM
#undef READ_PARAM



namespace cxmf
{

constexpr inline uint32_t MAGIC = ((static_cast<uint32_t>('F') << 24) |	 //
								   (static_cast<uint32_t>('M') << 16) |	 //
								   (static_cast<uint32_t>('X') << 8) |	 //
								   static_cast<uint32_t>('C'));

struct HEADER
{
	uint32_t magic;
	uint32_t version;
	uint32_t compressedSize;
	uint32_t baseSize;
	uint32_t flags;
};

static void send_log_message(Logger* logger, const std::string& txt)
{
	if (logger && !txt.empty())
	{
		logger->write(txt.c_str());
	}
}

#define CXMF_LOG(LogHandler, ...) \
send_log_message(LogHandler, std::format(__VA_ARGS__))



static constexpr uint32_t make_version(int32_t major, int32_t minor, int32_t patch)
{
	return (static_cast<uint32_t>(major) << 24) |  //
		   (static_cast<uint32_t>(minor) << 16) |  //
		   static_cast<uint32_t>(patch);
}

uint32_t GetVersion()
{
	return make_version(CXMF_VERSION_MAJOR, CXMF_VERSION_MINOR, CXMF_VERSION_PATCH);
}

void DecodeVersion(uint32_t version, uint32_t& major, uint32_t& minor, uint32_t& patch)
{
	major = (version >> 24) & 0xFF;
	minor = (version >> 16) & 0xFF;
	patch = version & 0xFFFF;
}

bool HasImporter()
{
#ifdef CXMF_INCLUDE_IMPORTER
	return true;
#else
	return false;
#endif
}

static constexpr size_t aligned_size(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

static std::optional<std::string> load_file_content(const char* filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) return std::nullopt;

	file.seekg(0, std::ios::end);
	const size_t fileSize = static_cast<size_t>(file.tellg());
	if (fileSize == 0) return std::string();

	file.seekg(0, std::ios::beg);
	std::string content(fileSize, '\0');
	file.read(content.data(), fileSize);
	return content;
}

#ifdef CXMF_INCLUDE_IMPORTER

class CXMFAssimpScopeLogStream final : public Assimp::LogStream
{
private:
	static constexpr uint32_t ErrFlags = Assimp::Logger::ErrorSeverity::Warn | Assimp::Logger::ErrorSeverity::Err;

private:
	Logger* m_Logger;

public:
	CXMFAssimpScopeLogStream() = delete;

	explicit CXMFAssimpScopeLogStream(Logger* logger)
		: m_Logger(logger)
	{
		if (m_Logger) Assimp::DefaultLogger::get()->attachStream(this, ErrFlags);
	}

	~CXMFAssimpScopeLogStream() override
	{
		if (m_Logger) Assimp::DefaultLogger::get()->detachStream(this, ErrFlags);
	}

	void write(const char* message) override
	{
		if (m_Logger) m_Logger->write(message);
	}
};



enum class SamplerMagFilter : unsigned int
{
	SamplerMagFilter_Nearest = 9728,
	SamplerMagFilter_Linear = 9729
};

enum class SamplerMinFilter : unsigned int
{
	SamplerMinFilter_Nearest = 9728,
	SamplerMinFilter_Linear = 9729,
	SamplerMinFilter_Nearest_Mipmap_Nearest = 9984,
	SamplerMinFilter_Linear_Mipmap_Nearest = 9985,
	SamplerMinFilter_Nearest_Mipmap_Linear = 9986,
	SamplerMinFilter_Linear_Mipmap_Linear = 9987
};



struct BoundingBox
{
	glm::vec3 min;
	glm::vec3 max;

	BoundingBox()
		: min(std::numeric_limits<float>::max()), max(-std::numeric_limits<float>::max())
	{}

	BoundingSphere getSphere() const
	{
		const glm::vec3 center = (min + max) * 0.5F;
		const float radius = glm::length((max - min) * 0.5F);

		BoundingSphere sphere;
		sphere.center[0] = center[0];
		sphere.center[1] = center[1];
		sphere.center[2] = center[2];
		sphere.radius = radius;
		return sphere;
	}
};

static void convertAssimpMatrixToGLM(glm::mat4& mat, const aiMatrix4x4& assimpMat)
{
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			mat[col][row] = assimpMat[row][col];
		}
	}
}

static void convertAssimpMatrixToCXMF(Mat4x4& mat, const aiMatrix4x4& assimpMat)
{
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			mat[col * 4 + row] = assimpMat[row][col];
		}
	}
}

static void convertGLMMatrixToCXMF(Mat4x4& mat, const glm::mat4& glmMat)
{
	for (int col = 0; col < 4; ++col)
	{
		for (int row = 0; row < 4; ++row)
		{
			mat[col * 4 + row] = glmMat[col][row];
		}
	}
}

static glm::vec2 assimpVectorToGLM(const aiVector2D& vec)
{
	return glm::vec2(vec.x, vec.y);
}

static glm::vec3 assimpVectorToGLM(const aiVector3D& vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}

struct ImportContext
{
	struct IntermediateVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec3 tangent;
		uint32_t boneID[4];
		float weight[4];
	};

	struct IntermediateMesh
	{
		std::string name;
		std::vector<IntermediateVertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint32_t> meshletVertices;
		std::vector<uint8_t> meshletTriangles;
		std::vector<Meshlet> meshlets;
		BoundingBox aabb;
		uint32_t materialIndex;
	};

	Logger* logger;
	BoundingBox modelAABB;
	std::string modelName;
	std::string modelCopyright;
	std::string modelGenerator;
	std::vector<IntermediateMesh> meshes;
	std::vector<MeshHierarchy> nodes;
	std::vector<Texture> textures;
	std::vector<Sampler> samplers;
	std::vector<Material> materials;
	std::list<Bone> bones;

	std::vector<aiNode*> importedNodes;
	std::unordered_map<aiMesh*, uint32_t> importedMeshes;
	std::unordered_map<aiMaterial*, uint32_t> importedMaterials;
	std::unordered_map<std::string, uint32_t> importedTextures;
	std::unordered_map<std::string, uint32_t> importedBones;
	uint32_t idCounter;

	bool hasNode(aiNode* node) const
	{
		const std::vector<aiNode*>::const_iterator _End = importedNodes.cend();
		return std::find(importedNodes.cbegin(), _End, node) != _End;
	}
	uint32_t getMeshIndex(aiMesh* mesh) const
	{
		const auto _It = importedMeshes.find(mesh);
		if (_It != importedMeshes.end())  //
			return _It->second;
		return INVALID_INDEX;
	}
	uint32_t getMaterialIndex(aiMaterial* material) const
	{
		const auto _It = importedMaterials.find(material);
		if (_It != importedMaterials.end())	 //
			return _It->second;
		return INVALID_INDEX;
	}
	uint32_t getBoneIndex(const std::string& boneName) const
	{
		const auto _It = importedBones.find(boneName);
		if (_It != importedBones.end())	 //
			return _It->second;
		return INVALID_INDEX;
	}
	uint32_t getTextureIndex(const std::string& path) const
	{
		const auto _It = importedTextures.find(path);
		if (_It != importedTextures.end())	//
			return _It->second;
		return INVALID_INDEX;
	}
	uint32_t getSamplerIndex(const Sampler& s) const
	{
		const uint32_t _Sz = static_cast<uint32_t>(samplers.size());
		for (uint32_t i = 0; i < _Sz; ++i)
		{
			const Sampler& v = samplers[i];
			if (v.magFilter == s.magFilter &&		 //
				v.minFilter == s.minFilter &&		 //
				v.mipmapMode == s.mipmapMode &&		 //
				v.addressModeU == s.addressModeU &&	 //
				v.addressModeV == s.addressModeV)
			{
				return i;
			}
		}
		return INVALID_INDEX;
	}
	bool hasBones() const
	{
		return !bones.empty();
	}

	ImportContext()
		: logger(nullptr),
		  modelAABB(),
		  modelName(),
		  modelCopyright(),
		  modelGenerator(),
		  meshes(),
		  nodes(),
		  textures(),
		  samplers(),
		  materials(),
		  bones(),
		  importedNodes(),
		  importedMeshes(),
		  importedMaterials(),
		  importedTextures(),
		  importedBones(),
		  idCounter(0)
	{}

	std::string genDummyName()
	{
		return std::string("unnamed") + std::to_string(idCounter++);
	}
};



static void optimizeMesh(ImportContext::IntermediateMesh& mesh)
{
	using vertex_t = ImportContext::IntermediateVertex;

	const size_t index_count = mesh.indices.size();
	const size_t unindexed_vertex_count = mesh.vertices.size();
	std::vector<uint32_t> remap(unindexed_vertex_count);
	const size_t vertex_count = meshopt_generateVertexRemap(remap.data(), mesh.indices.data(),	//
															index_count, mesh.vertices.data(),	//
															unindexed_vertex_count, sizeof(vertex_t));

	std::vector<uint32_t> newIndexBuffer(index_count);
	std::vector<vertex_t> newVertexBuffer(vertex_count);
	meshopt_remapIndexBuffer(newIndexBuffer.data(), mesh.indices.data(), index_count, remap.data());
	meshopt_remapVertexBuffer(newVertexBuffer.data(), mesh.vertices.data(), unindexed_vertex_count,	 //
							  sizeof(vertex_t), remap.data());

	{
		std::vector<uint32_t> tmpIndices(index_count);
		std::vector<vertex_t> tmpVertices(vertex_count);
		meshopt_optimizeVertexCache(tmpIndices.data(), newIndexBuffer.data(), index_count, vertex_count);

		meshopt_optimizeVertexFetch(tmpVertices.data(), tmpIndices.data(), index_count,	 //
									newVertexBuffer.data(), vertex_count, sizeof(vertex_t));

		newIndexBuffer = std::move(tmpIndices);
		newVertexBuffer = std::move(tmpVertices);
	}

	const size_t max_meshlets = meshopt_buildMeshletsBound(index_count, CXMF_MAX_MESHLET_VERTICES, CXMF_MAX_MESHLET_TRIANGLES);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<uint32_t> meshlet_vertices(index_count);
	std::vector<uint8_t> meshlet_triangles(index_count + max_meshlets * 3);
	const size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(),						 //
													   meshlet_triangles.data(), newIndexBuffer.data(), index_count,	 //
													   &newVertexBuffer[0].position[0], vertex_count, sizeof(vertex_t),	 //
													   CXMF_MAX_MESHLET_VERTICES, CXMF_MAX_MESHLET_TRIANGLES, 0.0F);
	{
		const meshopt_Meshlet& last = meshlets[meshlet_count - 1];
		meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
		meshlet_triangles.resize(last.triangle_offset + last.triangle_count * 3);
		meshlets.resize(meshlet_count);
	}

	mesh.meshlets.reserve(meshlet_count);
	for (const meshopt_Meshlet& m : meshlets)
	{
		uint32_t* const m_vertices = meshlet_vertices.data() + m.vertex_offset;
		uint8_t* const m_triangles = meshlet_triangles.data() + m.triangle_offset;
		meshopt_optimizeMeshlet(m_vertices, m_triangles, m.triangle_count, m.vertex_count);

		const meshopt_Bounds bounds = meshopt_computeMeshletBounds(m_vertices, m_triangles, m.triangle_count,	   //
																   &newVertexBuffer[0].position[0], vertex_count,  //
																   sizeof(vertex_t));

		Meshlet& newMeshlet = mesh.meshlets.emplace_back();
		newMeshlet.bounds.center[0] = bounds.center[0];
		newMeshlet.bounds.center[1] = bounds.center[1];
		newMeshlet.bounds.center[2] = bounds.center[2];
		newMeshlet.bounds.radius = bounds.radius;
		newMeshlet.vertexOffset = m.vertex_offset;
		newMeshlet.triangleOffset = m.triangle_offset;
		newMeshlet.vertexCount = m.vertex_count;
		newMeshlet.triangleCount = m.triangle_count;
	}

	mesh.vertices = std::move(newVertexBuffer);
	mesh.indices.clear();  // Unused
	mesh.meshletVertices = std::move(meshlet_vertices);
	mesh.meshletTriangles = std::move(meshlet_triangles);
}



static uint32_t parseAssimpBone(ImportContext& ctx, const aiScene& scene, const aiBone& assimpBone)
{
	const std::string boneName = assimpBone.mName.C_Str();
	uint32_t boneIndex = ctx.getBoneIndex(boneName);
	if (boneIndex != INVALID_INDEX)	 //
		return boneIndex;

	const aiNode& boneNode = *assimpBone.mNode;

	boneIndex = static_cast<uint32_t>(ctx.bones.size());
	ctx.importedBones.insert({boneName, boneIndex});
	Bone& bone = ctx.bones.emplace_back();
	bone.name = boneName;
	convertAssimpMatrixToCXMF(bone.inverseBindTransform, boneNode.mTransformation);
	convertAssimpMatrixToCXMF(bone.offsetMatrix, assimpBone.mOffsetMatrix);
	if (boneNode.mParent == nullptr)
	{
		bone.parentIndex = INVALID_INDEX;
	}
	else
	{
		const aiBone* const parentBone = scene.findBone(boneNode.mParent->mName);
		if (parentBone == nullptr)
		{
			bone.parentIndex = INVALID_INDEX;
		}
		else
		{
			bone.parentIndex = parseAssimpBone(ctx, scene, *parentBone);
			if (bone.parentIndex == boneIndex)
			{
				bone.parentIndex = INVALID_INDEX;
			}
		}
	}
	return boneIndex;
}

static uint32_t parseAssimpTexture(ImportContext& ctx, aiMaterial* assimpMaterial)
{
	aiTextureType textureType = aiTextureType_BASE_COLOR;
	aiString str;
	aiReturn result = assimpMaterial->Get(AI_MATKEY_TEXTURE(textureType, 0), str);
	if (result != aiReturn_SUCCESS || str.length == 0)
	{
		textureType = aiTextureType_DIFFUSE;
		result = assimpMaterial->Get(AI_MATKEY_TEXTURE(textureType, 0), str);
	}

	if (result != aiReturn_SUCCESS || str.length == 0)	//
		return INVALID_INDEX;

	const std::string texName = str.C_Str();
	uint32_t textureIndex = ctx.getTextureIndex(texName);
	if (textureIndex != INVALID_INDEX)	//
		return textureIndex;

	textureIndex = static_cast<uint32_t>(ctx.textures.size());
	ctx.importedTextures.insert({texName, textureIndex});

	Texture& tex = ctx.textures.emplace_back();
	tex.path = texName;
	tex.samplerIndex = INVALID_INDEX;

	Sampler sampler;
	aiTextureMapMode mapU, mapV;
	int filterMag, filterMin;

	const auto convAddressMode = [](aiTextureMapMode mode) -> Sampler::AddressMode
	{
		switch (mode)
		{
			case aiTextureMapMode_Clamp:
				return Sampler::AddressMode::CLAMP_TO_EDGE;
			case aiTextureMapMode_Mirror:
				return Sampler::AddressMode::MIRRORED_REPEAT;
			default:
				return Sampler::AddressMode::REPEAT;
		}
	};

	result = assimpMaterial->Get(AI_MATKEY_MAPPINGMODE_U(textureType, 0), mapU);
	if (result == aiReturn_SUCCESS)
	{
		sampler.addressModeU = convAddressMode(mapU);
	}
	else
	{
		sampler.addressModeU = Sampler::AddressMode::REPEAT;
	}

	result = assimpMaterial->Get(AI_MATKEY_MAPPINGMODE_V(textureType, 0), mapV);
	if (result == aiReturn_SUCCESS)
	{
		sampler.addressModeV = convAddressMode(mapV);
	}
	else
	{
		sampler.addressModeV = Sampler::AddressMode::REPEAT;
	}

	result = assimpMaterial->Get(AI_MATKEY_GLTF_MAPPINGFILTER_MAG(textureType, 0), filterMag);
	if (result == aiReturn_SUCCESS)
	{
		if (static_cast<SamplerMagFilter>(filterMag) == SamplerMagFilter::SamplerMagFilter_Linear)
			sampler.magFilter = Sampler::Filter::LINEAR;
		else
			sampler.magFilter = Sampler::Filter::NEAREST;
	}
	else
	{
		sampler.magFilter = Sampler::Filter::NEAREST;
	}

	result = assimpMaterial->Get(AI_MATKEY_GLTF_MAPPINGFILTER_MIN(textureType, 0), filterMin);
	if (result == aiReturn_SUCCESS)
	{
		switch (static_cast<SamplerMinFilter>(filterMin))
		{
			case SamplerMinFilter::SamplerMinFilter_Linear:
			{
				sampler.minFilter = Sampler::Filter::LINEAR;
				sampler.mipmapMode = Sampler::MipmapMode::NONE;
				break;
			}
			case SamplerMinFilter::SamplerMinFilter_Nearest_Mipmap_Nearest:
			{
				sampler.minFilter = Sampler::Filter::NEAREST;
				sampler.mipmapMode = Sampler::MipmapMode::NEAREST;
				break;
			}
			case SamplerMinFilter::SamplerMinFilter_Linear_Mipmap_Nearest:
			{
				sampler.minFilter = Sampler::Filter::LINEAR;
				sampler.mipmapMode = Sampler::MipmapMode::NEAREST;
				break;
			}
			case SamplerMinFilter::SamplerMinFilter_Nearest_Mipmap_Linear:
			{
				sampler.minFilter = Sampler::Filter::NEAREST;
				sampler.mipmapMode = Sampler::MipmapMode::LINEAR;
				break;
			}
			case SamplerMinFilter::SamplerMinFilter_Linear_Mipmap_Linear:
			{
				sampler.minFilter = Sampler::Filter::LINEAR;
				sampler.mipmapMode = Sampler::MipmapMode::LINEAR;
				break;
			}
			default:
			{
				sampler.minFilter = Sampler::Filter::NEAREST;
				sampler.mipmapMode = Sampler::MipmapMode::NONE;
				break;
			}
		}
	}
	else
	{
		sampler.minFilter = Sampler::Filter::NEAREST;
		sampler.mipmapMode = Sampler::MipmapMode::NONE;
	}



	tex.samplerIndex = ctx.getSamplerIndex(sampler);
	if (tex.samplerIndex != INVALID_INDEX)	//
		return textureIndex;

	aiString samplerName;
	result = assimpMaterial->Get(AI_MATKEY_GLTF_MAPPINGNAME(textureType, 0), samplerName);
	if (result == aiReturn_SUCCESS && samplerName.length > 0)
	{
		aiString samplerID;
		result = assimpMaterial->Get(AI_MATKEY_GLTF_MAPPINGID(textureType, 0), samplerID);
		if (result == aiReturn_SUCCESS && samplerID.length > 0)
		{
			sampler.name = std::string(samplerName.C_Str()) + std::string(samplerID.C_Str());
		}
		else
		{
			sampler.name = std::string(samplerName.C_Str()) + std::to_string(ctx.idCounter++);
		}
	}
	else
	{
		sampler.name = ctx.genDummyName();
	}

	tex.samplerIndex = static_cast<uint32_t>(ctx.samplers.size());
	ctx.samplers.push_back(std::move(sampler));
	return textureIndex;
}

static uint32_t parseAssimpMaterial(ImportContext& ctx, aiMaterial* assimpMaterial)
{
	uint32_t materialIndex = ctx.getMaterialIndex(assimpMaterial);
	if (materialIndex != INVALID_INDEX)	 //
		return materialIndex;

	const aiString materialName = assimpMaterial->GetName();

	materialIndex = static_cast<uint32_t>(ctx.materials.size());
	ctx.importedMaterials.insert({assimpMaterial, materialIndex});

	Material& mat = ctx.materials.emplace_back();
	mat.name = materialName.C_Str();
	if (mat.name.empty()) mat.name = ctx.genDummyName();
	mat.textureIndex = parseAssimpTexture(ctx, assimpMaterial);

	aiColor4D baseColorFactor;
	aiReturn result = assimpMaterial->Get(AI_MATKEY_BASE_COLOR, baseColorFactor);
	if (result != aiReturn_SUCCESS)
	{
		result = assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, baseColorFactor);
	}
	if (result == aiReturn_SUCCESS)
	{
		mat.baseColorFactor[0] = glm::clamp<float>(baseColorFactor[0], 0.0F, 1.0F);
		mat.baseColorFactor[1] = glm::clamp<float>(baseColorFactor[1], 0.0F, 1.0F);
		mat.baseColorFactor[2] = glm::clamp<float>(baseColorFactor[2], 0.0F, 1.0F);
		mat.baseColorFactor[3] = glm::clamp<float>(baseColorFactor[3], 0.0F, 1.0F);
	}
	else
	{
		mat.baseColorFactor[0] = 1.0F;
		mat.baseColorFactor[1] = 1.0F;
		mat.baseColorFactor[2] = 1.0F;
		mat.baseColorFactor[3] = 1.0F;
	}

	float roughnessFactor;
	result = assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor);
	if (result == aiReturn_SUCCESS)
	{
		mat.roughnessFactor = glm::clamp<float>(roughnessFactor, 0.0F, 1.0F);
	}
	else
	{
		mat.roughnessFactor = 1.0F;
	}

	float metallicFactor;
	result = assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor);
	if (result == aiReturn_SUCCESS)
	{
		mat.metallicFactor = glm::clamp<float>(metallicFactor, 0.0F, 1.0F);
	}
	else
	{
		mat.metallicFactor = 1.0F;
	}

	float ambientOcclusionFactor;
	result = assimpMaterial->Get(AI_MATKEY_LIGHTMAP_FACTOR, ambientOcclusionFactor);
	if (result == aiReturn_SUCCESS)
	{
		mat.ambientOcclusionFactor = glm::clamp<float>(ambientOcclusionFactor, 0.0F, 1.0F);
	}
	else
	{
		mat.ambientOcclusionFactor = 1.0F;
	}

	aiColor3D emissiveFactor;
	result = assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveFactor);
	if (result == aiReturn_SUCCESS)
	{
		mat.emissiveFactor[0] = glm::clamp<float>(emissiveFactor[0], 0.0F, 1.0F);
		mat.emissiveFactor[1] = glm::clamp<float>(emissiveFactor[1], 0.0F, 1.0F);
		mat.emissiveFactor[2] = glm::clamp<float>(emissiveFactor[2], 0.0F, 1.0F);
	}
	else
	{
		mat.emissiveFactor[0] = 0.0F;
		mat.emissiveFactor[1] = 0.0F;
		mat.emissiveFactor[2] = 0.0F;
	}

	aiString alphaMode;
	result = assimpMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
	if (result == aiReturn_SUCCESS)
	{
		if (std::strcmp(alphaMode.C_Str(), "OPAQUE") == 0)
		{
			mat.alphaMode = Material::AlphaMode::OPAQUE;
			mat.alphaCutoff = 0.0F;
		}
		else if (std::strcmp(alphaMode.C_Str(), "MASK") == 0)
		{
			mat.alphaMode = Material::AlphaMode::MASK;
			float alphaCutoff;
			result = assimpMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff);
			if (result == aiReturn_SUCCESS)
			{
				mat.alphaCutoff = glm::clamp<float>(alphaCutoff, 0.0F, 1.0F);
			}
			else
			{
				mat.alphaCutoff = 0.5F;
			}
		}
		else if (std::strcmp(alphaMode.C_Str(), "BLEND") == 0)
		{
			mat.alphaMode = Material::AlphaMode::BLEND;
			mat.alphaCutoff = 0.0F;
		}
		else
		{
			mat.alphaMode = Material::AlphaMode::OPAQUE;
			mat.alphaCutoff = 0.0F;
		}
	}
	else
	{
		mat.alphaMode = Material::AlphaMode::OPAQUE;
		mat.alphaCutoff = 0.0F;
	}

	bool doubleSided;
	result = assimpMaterial->Get(AI_MATKEY_TWOSIDED, doubleSided);
	if (result == aiReturn_SUCCESS)
	{
		mat.doubleSided = doubleSided;
	}
	else
	{
		mat.doubleSided = false;
	}

	aiShadingMode shadingMode;
	result = assimpMaterial->Get(AI_MATKEY_SHADING_MODEL, shadingMode);
	if (result == aiReturn_SUCCESS)
	{
		mat.shadeless = (shadingMode == aiShadingMode_Unlit);
	}
	else
	{
		mat.shadeless = false;
	}
	return materialIndex;
}

static uint32_t parseAssimpMesh(ImportContext& ctx, const aiScene& scene, aiMesh* assimpMesh)
{
	uint32_t meshIndex = ctx.getMeshIndex(assimpMesh);
	if (meshIndex != INVALID_INDEX)	 //
		return meshIndex;

	using vertex_t = ImportContext::IntermediateVertex;
	using mesh_t = ImportContext::IntermediateMesh;

	const bool hasTextureCoords = assimpMesh->HasTextureCoords(0);

	meshIndex = static_cast<uint32_t>(ctx.meshes.size());
	ctx.importedMeshes.insert({assimpMesh, meshIndex});
	mesh_t& mesh = ctx.meshes.emplace_back();
	mesh.name = assimpMesh->mName.C_Str();
	if (mesh.name.empty()) mesh.name = ctx.genDummyName();

	mesh.aabb.min = assimpVectorToGLM(assimpMesh->mAABB.mMin);
	mesh.aabb.max = assimpVectorToGLM(assimpMesh->mAABB.mMax);
	ctx.modelAABB.min = glm::min(ctx.modelAABB.min, mesh.aabb.min);
	ctx.modelAABB.max = glm::max(ctx.modelAABB.max, mesh.aabb.max);

	// Vertices
	mesh.vertices.resize(assimpMesh->mNumVertices);
	for (uint32_t i_vertex = 0; i_vertex < assimpMesh->mNumVertices; ++i_vertex)
	{
		vertex_t& vert = mesh.vertices[i_vertex];
		vert.position = assimpVectorToGLM(assimpMesh->mVertices[i_vertex]);
		vert.normal = assimpVectorToGLM(assimpMesh->mNormals[i_vertex]);
		vert.tangent = assimpVectorToGLM(assimpMesh->mTangents[i_vertex]);

		if (hasTextureCoords)
		{
			const aiVector3D& uv = assimpMesh->mTextureCoords[0][i_vertex];
			vert.uv[0] = uv[0];
			vert.uv[1] = uv[1];
		}
		else
		{
			vert.uv[0] = 0.0F;
			vert.uv[1] = 0.0F;
		}

		for (int i = 0; i < 4; ++i)
		{
			vert.boneID[i] = INVALID_INDEX;
			vert.weight[i] = 0.0F;
		}
	}

	// Indices
	mesh.indices.resize(static_cast<size_t>(assimpMesh->mNumFaces) * 3);
	for (uint32_t i_face = 0; i_face < assimpMesh->mNumFaces; ++i_face)
	{
		const aiFace& face = assimpMesh->mFaces[i_face];
		const uint32_t offset = i_face * 3;
		mesh.indices[offset + 0] = face.mIndices[0];
		mesh.indices[offset + 1] = face.mIndices[1];
		mesh.indices[offset + 2] = face.mIndices[2];
	}

	// Bones
	if (assimpMesh->mNumBones > 0)
	{
		for (uint32_t i_bone = 0; i_bone < assimpMesh->mNumBones; ++i_bone)
		{
			parseAssimpBone(ctx, scene, *assimpMesh->mBones[i_bone]);
		}

		std::vector<uint32_t> influenceOnVerticies(mesh.vertices.size(), 0);
		std::string boneName;
		for (uint32_t i_bone = 0; i_bone < assimpMesh->mNumBones; ++i_bone)
		{
			const aiBone& bone = *assimpMesh->mBones[i_bone];
			boneName = bone.mName.C_Str();
			const uint32_t boneIndex = ctx.importedBones[boneName];

			for (uint32_t i_weight = 0; i_weight < bone.mNumWeights; ++i_weight)
			{
				const aiVertexWeight& assimpWeight = bone.mWeights[i_weight];
				vertex_t& vert = mesh.vertices[assimpWeight.mVertexId];
				uint32_t& coord = influenceOnVerticies[assimpWeight.mVertexId];
				if (coord < 4)
				{
					vert.boneID[coord] = boneIndex;
					vert.weight[coord] = assimpWeight.mWeight;
					++coord;
				}
			}
		}

		// Normalize weights
		float goodWeights[4];
		int maxGoodWeights;
		for (uint32_t i_vertex = 0; i_vertex < assimpMesh->mNumVertices; ++i_vertex)
		{
			maxGoodWeights = 0;
			vertex_t& vert = mesh.vertices[i_vertex];
			for (int i = 0; i < 4; ++i)
			{
				if (vert.boneID[i] == INVALID_INDEX)  //
					break;

				goodWeights[maxGoodWeights++] = vert.weight[i];
			}

			if (maxGoodWeights > 0)
			{
				float mag = 0.0F;
				for (int i = 0; i < maxGoodWeights; ++i)
					mag += goodWeights[i];

				if (mag > 0.00001F)
				{
					for (int i = 0; i < maxGoodWeights; ++i)
						vert.weight[i] = goodWeights[i] / mag;
				}
			}
		}
	}

	// Material
	if (assimpMesh->mMaterialIndex < scene.mNumMaterials)
	{
		mesh.materialIndex = parseAssimpMaterial(ctx, scene.mMaterials[assimpMesh->mMaterialIndex]);
	}
	else
	{
		mesh.materialIndex = INVALID_INDEX;
	}
	return meshIndex;
}

static void parseAssimpMeshNode(ImportContext& ctx, const aiScene& scene, aiNode* root, uint32_t parentIndex)
{
	if (!root || ctx.hasNode(root))	 //
		return;

	ctx.importedNodes.push_back(root);

	if (root->mNumMeshes == 0)
	{
		for (uint32_t i_node = 0; i_node < root->mNumChildren; ++i_node)
		{
			parseAssimpMeshNode(ctx, scene, root->mChildren[i_node], parentIndex);
		}
		return;
	}

	if (root->mNumMeshes != 1)
	{
		CXMF_LOG(ctx.logger, "FATAL ERROR: Node '{}' has more than one mesh!", root->mName.C_Str());
		std::abort();
	}

	const uint32_t currentIndex = static_cast<uint32_t>(ctx.nodes.size());
	MeshHierarchy& node = ctx.nodes.emplace_back();

	node.name = root->mName.C_Str();
	if (node.name.empty()) node.name = ctx.genDummyName();
	convertAssimpMatrixToCXMF(node.localTransform, root->mTransformation);
	node.parentIndex = parentIndex;

	aiMesh* const assimpMesh = scene.mMeshes[root->mMeshes[0]];
	node.meshIndex = parseAssimpMesh(ctx, scene, assimpMesh);

	for (uint32_t i_node = 0; i_node < root->mNumChildren; ++i_node)
	{
		parseAssimpMeshNode(ctx, scene, root->mChildren[i_node], currentIndex);
	}
}

static void parseAssimpMetaData(ImportContext& ctx, const aiScene& scene)
{
	ctx.modelCopyright.clear();
	ctx.modelGenerator.clear();

	bool hasCopyright = false;
	bool hasGenerator = false;
	for (uint32_t i = 0; i < scene.mMetaData->mNumProperties; ++i)
	{
		const aiString& key = scene.mMetaData->mKeys[i];
		const aiMetadataEntry& entry = scene.mMetaData->mValues[i];
		if (std::strcmp(key.C_Str(), AI_METADATA_SOURCE_COPYRIGHT) == 0)
		{
			hasCopyright = true;
			if (entry.mType == AI_AISTRING)
			{
				const aiString& copyright = *static_cast<aiString*>(entry.mData);
				if (copyright.length > 0)
				{
					ctx.modelCopyright = copyright.C_Str();
				}
			}
		}
		else if (std::strcmp(key.C_Str(), AI_METADATA_SOURCE_GENERATOR) == 0)
		{
			hasGenerator = true;
			if (entry.mType == AI_AISTRING)
			{
				const aiString& generator = *static_cast<aiString*>(entry.mData);
				if (generator.length > 0)
				{
					ctx.modelGenerator = generator.C_Str();
				}
			}
		}

		if (hasCopyright && hasGenerator)  //
			break;
	}
}

static bool parseAssimp(const char* filename, ImportContext& ctx)
{
	if (Assimp::DefaultLogger::isNullLogger())
	{
		Assimp::DefaultLogger::create(nullptr, Assimp::Logger::LogSeverity::NORMAL, 0, nullptr);
	}

	CXMFAssimpScopeLogStream logStream(ctx.logger);
	Assimp::Importer importer;
	importer.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
	constexpr uint32_t importFlags = aiProcess_CalcTangentSpace |		//
									 aiProcess_JoinIdenticalVertices |	//
									 aiProcess_Triangulate |			//
									 aiProcess_GenNormals |				//
									 aiProcess_LimitBoneWeights |		//
									 aiProcess_ValidateDataStructure |	//
									 // aiProcess_ImproveCacheLocality |		 //
									 aiProcess_RemoveRedundantMaterials |  //
									 aiProcess_PopulateArmatureData |	   //
									 aiProcess_SortByPType |			   //
									 // aiProcess_OptimizeMeshes |			   //
									 // aiProcess_OptimizeGraph |			   //
									 aiProcess_FlipUVs |  //
									 aiProcess_GenBoundingBoxes;

	const aiScene* const scene = importer.ReadFile(filename, importFlags);
	if (!scene)
	{
		CXMF_LOG(ctx.logger, "Failed to import '{}' | {}", filename, importer.GetErrorString());
		return false;
	}

	if (!scene->HasMeshes())
	{
		CXMF_LOG(ctx.logger, "Scene no meshes '{}'", filename);
		return false;
	}

	parseAssimpMetaData(ctx, *scene);

	if (scene->mName.length > 0)
	{
		ctx.modelName = scene->mName.C_Str();
	}
	else
	{
		ctx.modelName = ctx.genDummyName();
	}

	parseAssimpMeshNode(ctx, *scene, scene->mRootNode, INVALID_INDEX);
	return true;
}

static uint32_t makeCXMFGeneral(Model& model, ImportContext& ctx)
{
	model.name = ctx.modelName;
	model.bounds = ctx.modelAABB.getSphere();
	model.copyright = ctx.modelCopyright;
	model.generator = ctx.modelGenerator;
	model.flags = 0;
	model.version = GetVersion();

	model.textures = std::move(ctx.textures);
	model.samplers = std::move(ctx.samplers);
	model.materials = std::move(ctx.materials);
	model.meshNodes = std::move(ctx.nodes);

	size_t totalMeshlets = 0;
	size_t totalMeshletVertices = 0;
	size_t totalMeshletTriangles = 0;
	model.meshes.reserve(ctx.meshes.size());
	for (const ImportContext::IntermediateMesh& m : ctx.meshes)
	{
		Mesh& mesh = model.meshes.emplace_back();
		mesh.name = m.name;
		mesh.bounds = m.aabb.getSphere();
		mesh.meshletOffset = static_cast<uint32_t>(totalMeshlets);
		mesh.meshletCount = static_cast<uint32_t>(m.meshlets.size());
		mesh.materialIndex = m.materialIndex;

		totalMeshlets += m.meshlets.size();
		totalMeshletVertices += m.meshletVertices.size();
		totalMeshletTriangles += m.meshletTriangles.size();
	}

	model.meshlets.reserve(totalMeshlets);
	model.meshletVertices.reserve(totalMeshletVertices);
	model.meshletTriangles.reserve(totalMeshletTriangles);
	uint32_t totalVertices = 0;
	uint32_t meshletVerticesOffset = 0;
	uint32_t meshletTrianglesOffset = 0;
	std::vector<uint32_t> tempMeshletVertices;
	const uint32_t meshesCount = static_cast<uint32_t>(ctx.meshes.size());
	for (uint32_t i_mesh = 0; i_mesh < meshesCount; ++i_mesh)
	{
		const ImportContext::IntermediateMesh& m = ctx.meshes[i_mesh];
		model.meshes[i_mesh].vertexOffset = totalVertices;
		model.meshes[i_mesh].vertexCount = static_cast<uint32_t>(m.vertices.size());

		tempMeshletVertices = m.meshletVertices;
		for (uint32_t& v : tempMeshletVertices)
		{
			v += totalVertices;
		}

		model.meshletVertices.insert(model.meshletVertices.end(), tempMeshletVertices.begin(), tempMeshletVertices.end());
		model.meshletTriangles.insert(model.meshletTriangles.end(), m.meshletTriangles.begin(), m.meshletTriangles.end());
		for (const Meshlet& mt : m.meshlets)
		{
			Meshlet& meshlet = model.meshlets.emplace_back();
			meshlet.bounds = mt.bounds;
			meshlet.vertexOffset = meshletVerticesOffset + mt.vertexOffset;
			meshlet.triangleOffset = meshletTrianglesOffset + mt.triangleOffset;
			meshlet.vertexCount = mt.vertexCount;
			meshlet.triangleCount = mt.triangleCount;
		}

		totalVertices += static_cast<uint32_t>(m.vertices.size());
		meshletVerticesOffset += static_cast<uint32_t>(m.meshletVertices.size());
		meshletTrianglesOffset += static_cast<uint32_t>(m.meshletTriangles.size());
	}
	return totalVertices;
}

template <typename _VertTy>
requires (std::is_same_v<_VertTy, Vertex> || std::is_same_v<_VertTy, WeightedVertex>)
static void makeCXMFVertices(std::vector<_VertTy>& outVertices,	 //
							 const std::vector<ImportContext::IntermediateMesh>& ctxMeshes)
{
	uint32_t vertexOffset = 0;
	for (const ImportContext::IntermediateMesh& m : ctxMeshes)
	{
		const uint32_t meshVertexCount = static_cast<uint32_t>(m.vertices.size());
		for (uint32_t i = 0; i < meshVertexCount; ++i)
		{
			const ImportContext::IntermediateVertex& inV = m.vertices[i];
			_VertTy& outV = outVertices[vertexOffset + i];
			for (int i = 0; i < 3; ++i)
			{
				outV.position[i] = inV.position[i];
				outV.normal[i] = inV.normal[i];
				outV.tangent[i] = inV.tangent[i];
			}
			for (int i = 0; i < 2; ++i)
			{
				outV.uv[i] = inV.uv[i];
			}

			if constexpr (std::is_same_v<_VertTy, WeightedVertex>)
			{
				for (int i = 0; i < 4; ++i)
				{
					outV.boneID[i] = inV.boneID[i];
					outV.weight[i] = inV.weight[i];
				}
			}
		}
		vertexOffset += meshVertexCount;
	}
}

static SkinnedModel* makeCXMFSkinned(ImportContext& ctx)
{
	SkinnedModel* const model = new SkinnedModel();
	const uint32_t totalVertices = makeCXMFGeneral(*model, ctx);
	constexpr uint32_t maxVerticesLimit = std::numeric_limits<uint32_t>::max() / aligned_size(sizeof(WeightedVertex), 16);
	if (totalVertices >= maxVerticesLimit)
	{
		CXMF_LOG(ctx.logger, "Overflow of the maximum number of vertices in model!");
		delete model;
		return nullptr;
	}

	model->vertices.resize(totalVertices);
	makeCXMFVertices(model->vertices, ctx.meshes);

	model->bones.reserve(ctx.bones.size());
	for (Bone& bone : ctx.bones)
	{
		model->bones.push_back(std::move(bone));
	}
	return model;
}

static StaticModel* makeCXMFStatic(ImportContext& ctx)
{
	StaticModel* const model = new StaticModel();
	const uint32_t totalVertices = makeCXMFGeneral(*model, ctx);
	constexpr uint32_t maxVerticesLimit = std::numeric_limits<uint32_t>::max() / aligned_size(sizeof(Vertex), 16);
	if (totalVertices >= maxVerticesLimit)
	{
		CXMF_LOG(ctx.logger, "Overflow of the maximum number of vertices in model!");
		delete model;
		return nullptr;
	}

	model->vertices.resize(totalVertices);
	makeCXMFVertices(model->vertices, ctx.meshes);
	return model;
}

static Model* importModel(const char* filename, Logger* logger)
{
	ImportContext ctx;
	ctx.logger = logger;
	if (!parseAssimp(filename, ctx))  //
		return nullptr;

	for (ImportContext::IntermediateMesh& mesh : ctx.meshes)
	{
		optimizeMesh(mesh);
	}

	if (ctx.hasBones())
	{
		return makeCXMFSkinned(ctx);
	}
	else
	{
		return makeCXMFStatic(ctx);
	}
}

#endif	// CXMF_INCLUDE_IMPORTER

Model* LoadFromFile(const char* filePath, Logger* logger)
{
	if (!filePath) return nullptr;

	std::string str = filePath;
	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
		str.pop_back();

	if (str.empty()) return nullptr;

	if (str.ends_with(".cxmf"))
	{
		const std::optional<std::string> content = load_file_content(str.c_str());
		if (!content.has_value())
		{
			CXMF_LOG(logger, "Can't open '{}'", str.c_str());
			return nullptr;
		}
		const std::string& buf = content.value();
		return LoadFromMemory(buf.c_str(), buf.length(), logger);
	}
#ifdef CXMF_INCLUDE_IMPORTER
	else if (str.ends_with(".gltf") || str.ends_with(".glb"))
	{
		return importModel(str.c_str(), logger);
	}
#endif
	else
	{
		CXMF_LOG(logger, "Invalid input file extension name '{}'", str.c_str());
		return nullptr;
	}
}

Model* LoadFromMemory(const void* data, size_t dataSize, Logger* logger)
{
	if (!data || dataSize <= (sizeof(HEADER) + 1))	//
		return nullptr;

	const uint8_t* pointer = static_cast<const uint8_t*>(data);
	const HEADER& header = *reinterpret_cast<const HEADER*>(pointer);
	pointer += sizeof(HEADER);

	if (header.magic != MAGIC)
	{
		CXMF_LOG(logger, "Invalid model magic!");
		return nullptr;
	}

	if (header.baseSize == 0 ||		   //
		header.compressedSize == 0 ||  //
		static_cast<size_t>(header.compressedSize) > (dataSize - (sizeof(HEADER) + 1)))
	{
		CXMF_LOG(logger, "Invalid model size!");
		return nullptr;
	}

	uint32_t major, minor, patch;
	DecodeVersion(header.version, major, minor, patch);
	if (major != CXMF_VERSION_MAJOR || minor != CXMF_VERSION_MINOR)
	{
		CXMF_LOG(logger, "Incorrect model version {}.{}.{} | Supported: {}.{}.X",  //
				 major, minor, patch, CXMF_VERSION_MAJOR, CXMF_VERSION_MINOR);
		return nullptr;
	}

	const ModelType modelTyp = static_cast<ModelType>(*pointer);
	pointer += 1;

	switch (modelTyp)
	{
		case ModelType::STATIC:
		case ModelType::SKINNED:
			break;
		default:
		{
			CXMF_LOG(logger, "Invalid model type!");
			return nullptr;
		}
	}

	z_stream zlibStream;
	std::memset(&zlibStream, 0, sizeof(z_stream));
	int err = inflateInit(&zlibStream);
	if (err != Z_OK)
	{
		CXMF_LOG(logger, "ERROR: inflateInit ({})", err);
		return nullptr;
	}

	std::string modelContent(header.baseSize, '\0');

	zlibStream.next_in = reinterpret_cast<Bytef*>(const_cast<uint8_t*>(pointer));
	zlibStream.avail_in = header.compressedSize;
	zlibStream.next_out = reinterpret_cast<Bytef*>(modelContent.data());
	zlibStream.avail_out = header.baseSize;

	do
	{
		err = inflate(&zlibStream, Z_NO_FLUSH);
	} while (err == Z_OK);

	inflateEnd(&zlibStream);

	if (err != Z_STREAM_END)
	{
		CXMF_LOG(logger, "ERROR: inflate ({})", err);
		return nullptr;
	}

	std::stringstream modelStream(std::move(modelContent));

	Model* outModel = nullptr;
	if (modelTyp == ModelType::STATIC)
	{
		StaticModel* const model = new StaticModel();
		modelStream >> *model;
		outModel = model;
	}
	else if (modelTyp == ModelType::SKINNED)
	{
		SkinnedModel* const model = new SkinnedModel();
		modelStream >> *model;
		outModel = model;
	}

	if (outModel)
	{
		outModel->flags = header.flags;
		outModel->version = header.version;
	}
	return outModel;
}



class DefaultOutputStream final : public OutputStream
{
private:
	std::ostream& m_Stream;

public:
	explicit DefaultOutputStream(std::ostream& stream)
		: m_Stream(stream)
	{}

	~DefaultOutputStream() override = default;

	bool write(const void* data, size_t sizeBytes) override
	{
		m_Stream.write(static_cast<const char*>(data), sizeBytes);
		return m_Stream.good();
	}
};

bool SaveToFile(const Model& model, const char* directoryPath, CompressionLevel level, Logger* logger)
{
	std::filesystem::path pathToFile;
	if (!directoryPath || directoryPath[0] == '\0')
		pathToFile = std::filesystem::current_path();
	else
		pathToFile = directoryPath;

	if (!std::filesystem::is_directory(pathToFile))	 //
		std::filesystem::create_directories(pathToFile);

	if (model.name.empty())
		pathToFile /= "unnamed.cxmf";
	else
		pathToFile /= (model.name + ".cxmf");

	std::ofstream file(pathToFile, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		CXMF_LOG(logger, "Can't open '{}'", pathToFile.string());
		return false;
	}

	DefaultOutputStream stream(file);
	return SaveToStream(model, stream, level, logger);
}

bool SaveToStream(const Model& model, OutputStream& stream, CompressionLevel level, Logger* logger)
{
	int compLevel;
	switch (level)
	{
		case CompressionLevel::NONE:
		{
			compLevel = Z_NO_COMPRESSION;
			break;
		}
		case CompressionLevel::SPEED:
		{
			compLevel = Z_BEST_SPEED;
			break;
		}
		case CompressionLevel::MIN_SIZE:
		{
			compLevel = Z_BEST_COMPRESSION;
			break;
		}
		default:
		{
			compLevel = Z_DEFAULT_COMPRESSION;
			break;
		}
	}

	std::string modelContent;
	{
		std::stringstream modelStream;
		switch (model.GetType())
		{
			case ModelType::STATIC:
			{
				modelStream << static_cast<const StaticModel&>(model);
				break;
			}
			case ModelType::SKINNED:
			{
				modelStream << static_cast<const SkinnedModel&>(model);
				break;
			}
			default:
			{
				CXMF_LOG(logger, "Invalid model type!");
				return false;
			}
		}
		modelContent = modelStream.str();
	}

	if (modelContent.length() >= static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
	{
		CXMF_LOG(logger, "Model size is too large!");
		return false;
	}

	z_stream zlibStream;
	std::memset(&zlibStream, 0, sizeof(z_stream));
	int err = deflateInit(&zlibStream, compLevel);
	if (err != Z_OK)
	{
		CXMF_LOG(logger, "ERROR: deflateInit ({})", err);
		return false;
	}

	const uint32_t sourceSize = static_cast<uint32_t>(modelContent.length());
	const uint32_t compressBoundSize = static_cast<uint32_t>(compressBound(sourceSize));

	void* const tempContent = malloc(compressBoundSize);
	if (!tempContent)
	{
		deflateEnd(&zlibStream);
		return false;
	}

	zlibStream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(modelContent.c_str()));
	zlibStream.avail_in = sourceSize;
	zlibStream.next_out = static_cast<Bytef*>(tempContent);
	zlibStream.avail_out = compressBoundSize;

	do
	{
		err = deflate(&zlibStream, Z_FINISH);
	} while (err == Z_OK);

	deflateEnd(&zlibStream);

	if (err != Z_STREAM_END)
	{
		free(tempContent);
		CXMF_LOG(logger, "ERROR: deflate ({})", err);
		return false;
	}

	const uint32_t compressedSize = static_cast<uint32_t>(zlibStream.total_out);
	const uint8_t modelTyp = static_cast<uint8_t>(model.GetType());
	HEADER header;
	header.magic = MAGIC;
	header.version = model.version;
	header.compressedSize = compressedSize;
	header.baseSize = sourceSize;
	header.flags = model.flags;

	const bool writeHeaderResult = stream.write(&header, sizeof(HEADER));
	if (!writeHeaderResult)
	{
		free(tempContent);
		return false;
	}

	const bool writeTypeResult = stream.write(&modelTyp, 1);
	if (!writeTypeResult)
	{
		free(tempContent);
		return false;
	}

	const bool writeContentResult = stream.write(tempContent, compressedSize);
	free(tempContent);
	return writeContentResult;
}





Model::Model(ModelType type)
	: c_type(type),
	  name(),
	  textures(),
	  samplers(),
	  materials(),
	  meshes(),
	  meshNodes(),
	  meshlets(),
	  bounds(),
	  copyright(),
	  generator(),
	  flags(0),
	  version(0)
{
	//
}

StaticModel* Model::StaticModelCast()
{
	if (c_type == ModelType::STATIC)  //
		return static_cast<StaticModel*>(this);
	return nullptr;
}

const StaticModel* Model::StaticModelCast() const
{
	if (c_type == ModelType::STATIC)  //
		return static_cast<const StaticModel*>(this);
	return nullptr;
}

SkinnedModel* Model::SkinnedModelCast()
{
	if (c_type == ModelType::SKINNED)  //
		return static_cast<SkinnedModel*>(this);
	return nullptr;
}

const SkinnedModel* Model::SkinnedModelCast() const
{
	if (c_type == ModelType::SKINNED)  //
		return static_cast<const SkinnedModel*>(this);
	return nullptr;
}



StaticModel::StaticModel()
	: Model(ModelType::STATIC), vertices()
{
	//
}

SkinnedModel::SkinnedModel()
	: Model(ModelType::SKINNED), vertices(), bones()
{
	//
}

}  //namespace cxmf
