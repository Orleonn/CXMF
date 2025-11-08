#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>



#define CXMF_NODISCARD [[nodiscard]]

namespace cxmf
{

CXMF_NODISCARD extern uint32_t GetVersion();
extern void DecodeVersion(uint32_t version, uint32_t& major, uint32_t& minor, uint32_t& patch);
CXMF_NODISCARD extern bool HasImporter();

constexpr inline uint32_t INVALID_INDEX = 0xFFFFFFFFU;



// Column-major matrix
struct Mat4x4
{
	float _data[16];

	constexpr Mat4x4()
		: _data{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}
	{}

	CXMF_NODISCARD float& operator[](size_t idx)
	{
		return _data[idx];
	}
	CXMF_NODISCARD const float& operator[](size_t idx) const
	{
		return _data[idx];
	}

	CXMF_NODISCARD float* Data()
	{
		return &_data[0];
	}
	CXMF_NODISCARD const float* Data() const
	{
		return &_data[0];
	}
};

struct BoundingSphere
{
	float center[3];  // x, y, z
	float radius;
};

struct Vertex
{
	float position[3];	// x, y, z
	float normal[3];	// x, y, z
	float uv[2];		// x, y
	float tangent[3];	// x, y, z
};

struct WeightedVertex
{
	float position[3];	 // x, y, z
	float normal[3];	 // x, y, z
	float uv[2];		 // x, y
	float tangent[3];	 // x, y, z
	uint32_t boneID[4];	 // INVALID_INDEX if no bone
	float weight[4];	 // normalized
};

struct Meshlet
{
	BoundingSphere bounds;
	uint32_t vertexOffset;
	uint32_t triangleOffset;
	uint32_t vertexCount;
	uint32_t triangleCount;
};

struct Sampler
{
	enum class Filter : int8_t
	{
		NEAREST = 0,  // VK_FILTER_NEAREST
		LINEAR = 1	  // VK_FILTER_LINEAR
	};

	enum class MipmapMode : int8_t
	{
		NONE = -1,
		NEAREST = 0,  // VK_SAMPLER_MIPMAP_MODE_NEAREST
		LINEAR = 1	  // VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	enum class AddressMode : int8_t
	{
		REPEAT = 0,			  // VK_SAMPLER_ADDRESS_MODE_REPEAT
		MIRRORED_REPEAT = 1,  // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT
		CLAMP_TO_EDGE = 2,	  // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
	};

	std::string name;
	Filter magFilter;
	Filter minFilter;
	MipmapMode mipmapMode;
	AddressMode addressModeU;
	AddressMode addressModeV;
};

struct Texture
{
	/*
		Path to texture files

		Base color texture:
			Without prefixes (my_awesome_texture.dds)
			Red channel: Red color
			Green channel: Green color
			Blue channel: Blue color
			Alpha channel: Alpha (or 1-bit alpha)

		Normal map texture:
			Must be prefixed with: "_n" (my_awesome_texture_n.dds)
			Red channel: X axis
			Green channel: Y axis
			Blue channel: Z axis

		Ambient occlusion, metallic, roughness texture:
			Must be prefixed with: "_arm" (my_awesome_texture_arm.dds)
			Red channel: Ambient occlusion map
			Green channel: Roughness map
			Blue channel: Metallic map

		Emmisive texture:
			Must be prefixed with: "_emi" (my_awesome_texture_emi.dds)
			Red channel: Red factor
			Green channel: Green factor
			Blue channel: Blue factor
	*/
	std::string path;
	uint32_t samplerIndex;

	CXMF_NODISCARD bool HasPath() const
	{
		return !path.empty();
	}

	CXMF_NODISCARD bool HasSampler() const
	{
		return samplerIndex != INVALID_INDEX;
	}
};

struct Material
{
	enum class AlphaMode : int8_t
	{
		OPAQUE = 0,
		MASK = 1,
		BLEND = 2
	};

	std::string name;
	float baseColorFactor[4];  // r, g, b, a
	float roughnessFactor;
	float metallicFactor;
	float ambientOcclusionFactor;
	float emissiveFactor[3];  // r, g, b
	uint32_t textureIndex;
	AlphaMode alphaMode;
	float alphaCutoff;
	bool doubleSided;
	bool shadeless;

	CXMF_NODISCARD bool HasTexture() const
	{
		return textureIndex != INVALID_INDEX;
	}
};

struct Mesh
{
	std::string name;
	BoundingSphere bounds;
	uint32_t vertexOffset;
	uint32_t vertexCount;
	uint32_t meshletOffset;
	uint32_t meshletCount;
	uint32_t materialIndex;

	CXMF_NODISCARD bool HasMaterial() const
	{
		return materialIndex != INVALID_INDEX;
	}
};

struct MeshHierarchy
{
	std::string name;
	Mat4x4 localTransform;
	Mat4x4 worldTransform;
	std::vector<uint32_t> meshIndices;
	uint32_t parentIndex;

	CXMF_NODISCARD bool HasParent() const
	{
		return parentIndex != INVALID_INDEX;
	}

	CXMF_NODISCARD uint32_t MeshCount() const
	{
		return static_cast<uint32_t>(meshIndices.size());
	}
};

struct Bone
{
	std::string name;
	Mat4x4 inverseBindTransform;
	Mat4x4 offsetMatrix;
	uint32_t parentIndex;

	CXMF_NODISCARD bool HasParent() const
	{
		return parentIndex != INVALID_INDEX;
	}
};



enum class ModelType
{
	STATIC = 0,
	SKINNED = 1
};

struct StaticModel;
struct SkinnedModel;

struct Model
{
private:
	const ModelType c_type;

public:
	std::string name;
	std::vector<Texture> textures;
	std::vector<Sampler> samplers;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<MeshHierarchy> meshNodes;
	std::vector<uint32_t> meshletVertices;
	std::vector<uint8_t> meshletTriangles;
	std::vector<Meshlet> meshlets;
	BoundingSphere bounds;
	std::string copyright;
	std::string generator;
	uint32_t flags;
	uint32_t version;

	Model() = delete;
	virtual ~Model() = default;

protected:
	Model(ModelType type);

public:
	CXMF_NODISCARD ModelType GetType() const
	{
		return c_type;
	}

	CXMF_NODISCARD StaticModel* StaticModelCast();
	CXMF_NODISCARD const StaticModel* StaticModelCast() const;

	CXMF_NODISCARD SkinnedModel* SkinnedModelCast();
	CXMF_NODISCARD const SkinnedModel* SkinnedModelCast() const;
};



struct StaticModel final : public Model
{
	std::vector<Vertex> vertices;

	StaticModel();
	~StaticModel() override = default;
};



struct SkinnedModel final : public Model
{
	std::vector<WeightedVertex> vertices;
	std::vector<Bone> bones;

	SkinnedModel();
	~SkinnedModel() override = default;
};



class Logger
{
public:
	virtual ~Logger() = default;

	virtual void write(const char* message) = 0;
};

/*
	Load a model in glTF 2.0 or FBX format for import (required CXMF_INCLUDE_IMPORTER option)
	or an already imported model in CXMF format.

	@param filePath - path to .gltf/.cxmf model file
	@param logger - optional log handler for outputting errors and warnings

	@return Return a 'cxmf::Model' object if success, otherwise 'nullptr'
*/
CXMF_NODISCARD extern Model* LoadFromFile(const char* filePath, Logger* logger = nullptr);



/*
	Load a CXMF model from memory buffer

	@param data - buffer with content
	@param dataSize - buffer size in bytes
	@param logger - optional log handler for outputting errors and warnings

	@return Return a 'cxmf::Model' object if success, otherwise 'nullptr'
*/
CXMF_NODISCARD extern Model* LoadFromMemory(const void* data, size_t dataSize, Logger* logger = nullptr);



enum class CompressionLevel
{
	NONE,	  // No compression
	DEFAULT,  // Default compression
	SPEED,	  // Max speed compression
	MIN_SIZE  // Max compression
};

/*
	Save the model to file in CXMF format

	@param model - model to save
	@param directoryPath - path to the directory where the model should be saved, with 'model.name'.cxmf format
		if directoryPath is nullptr then model will be saved to current working directory
	@param level - compression level
	@param logger - optional log handler for outputting errors and warnings

	@return Return true if success
*/
extern bool SaveToFile(const Model& model, const char* directoryPath, CompressionLevel level, Logger* logger = nullptr);



class OutputStream
{
public:
	virtual ~OutputStream() = default;

	// Return false if something went wrong during recording
	virtual bool write(const void* data, size_t sizeBytes) = 0;
};

/*
	Save the model content to output stream

	@param model - model to save
	@param stream - output stream
	@param level - compression level
	@param logger - optional log handler for outputting errors and warnings

	@return Return true if success
*/
extern bool SaveToStream(const Model& model, OutputStream& stream, CompressionLevel level, Logger* logger = nullptr);



/*
	Use this for free model object or just use C++ 'delete' keyword
*/
inline void Free(Model* const& model)
{
	if (model)
	{
		delete model;
		const_cast<Model*&>(model) = nullptr;
	}
}

}  //namespace cxmf
