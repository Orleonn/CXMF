#include "CXMF.hpp"
#include "cmd.hpp"

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <format>
#include <filesystem>
#include <thread>



#define PAUSE(M) std::this_thread::sleep_for(std::chrono::milliseconds(M))



static void readInput(std::string& input)
{
	std::getline(cmd::cin, input);
}

static void trimStringSpaces(std::string& str)
{
	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
		str.erase(0, 1);

	while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
		str.pop_back();
}

class CXMFConsoleLogger final : public cxmf::Logger
{
public:
	void write(const char* message) override
	{
		cmd::cout << cmd::clr_orange << message << cmd::clr_gray << cmd::endl;
	}
};



class OptionSelector
{
private:
	using option_pair_t = std::pair<std::string, std::function<void()>>;

private:
	std::vector<option_pair_t> m_Options;

private:
	void fill_options() const
	{
		for (size_t i = 0; i < m_Options.size(); ++i)
		{
			cmd::cout << '[' << (i + 1) << "] " << m_Options[i].first << '\n';
		}
		cmd::cout << cmd::flush;
	}

public:
	OptionSelector()
		: m_Options()
	{}

	~OptionSelector() = default;

public:
	void add(const std::string& name, std::function<void()> func)
	{
		m_Options.push_back({name, std::move(func)});
	}

	void reset()
	{
		m_Options.clear();
	}

	void select() const
	{
		if (m_Options.empty()) return;

		fill_options();

		cmd::cout << cmd::save_cpos;
		std::string input;
		int result;
		const int max_options = static_cast<int>(m_Options.size());
		for (;;)
		{
			cmd::cout << "Enter choice (1-" << max_options << "): ";
			readInput(input);

			try
			{
				result = std::stoi(input);
			}
			catch (...)
			{
				result = 0;
			}

			if (result < 1 || result > max_options)
			{
				input.clear();
				cmd::cout << cmd::restore_cpos << cmd::clear_after;
				cmd::cout << "Incorrect option, try again!\n";
			}
			else
			{
				m_Options[result - 1].second();
				break;
			}
		}
	}
};

// Returns -1 if cancelled, or index from 0 to (IdsCount - 1)
static int processSelectIndex(int IdsCount)
{
	if (IdsCount <= 0) return -1;

	cmd::cout << cmd::save_cpos;

	std::string input;
	int index;
	for (;;)
	{
		cmd::cout << "Enter ID:\n" << cmd::clr_green;
		cmd::cout << ">> ";
		readInput(input);
		cmd::cout << cmd::clr_gray;

		if (input.empty())
		{
			index = -1;
			break;
		}
		else
		{
			try
			{
				index = std::stoi(input);
			}
			catch (...)
			{
				index = IdsCount;
			}

			if (index < 0 || index >= IdsCount)
			{
				input.clear();
				cmd::cout << cmd::restore_cpos << cmd::clear_after;
				cmd::cout << "Invalid ID, try again!\n";
			}
			else
			{
				break;
			}
		}
	}
	return index;
}

// Returns empty string if cancelled
static std::string processSelectString(std::string prefix, const std::function<bool(const std::string&)>& validator)
{
	if (prefix.empty()) prefix = "string";

	cmd::cout << cmd::save_cpos;

	std::string input;
	for (;;)
	{
		cmd::cout << "Enter new " << prefix << ":\n" << cmd::clr_green;
		cmd::cout << ">> ";
		readInput(input);
		cmd::cout << cmd::clr_gray;

		trimStringSpaces(input);

		if (input.empty())
		{
			break;
		}
		else if (validator(input))
		{
			break;
		}
		else
		{
			input.clear();
			cmd::cout << cmd::restore_cpos << cmd::clear_after;
			cmd::cout << "Invalid " << prefix << ", try again!\n";
		}
	}
	return input;
}

enum class ModelMenuType
{
	MAIN,
	TEXTURES,
	SAMPLERS,
	MATERIALS,
	MESHES,
	NODES,
	BONES
};



static CXMFConsoleLogger consoleLogger;
static OptionSelector optionControl;
static std::string currentWorkDir;
static std::string currentAssetPath;
static cxmf::Model* currentModel = nullptr;
static ModelMenuType currentModelMenu = ModelMenuType::MAIN;
static bool needToExit = false;
static bool needToClearScreen = true;



static void clearScreen()
{
	if (needToClearScreen)
	{
		cmd::cout << cmd::clear;
		needToClearScreen = false;
	}
}

static void option_Exit()
{
	needToExit = true;
}

static void option_LoadModel()
{
	cmd::cout << cmd::clear;
	cmd::cout << "Enter the path to model ";
#ifdef CXMF_INCLUDE_IMPORTER
	cmd::cout << "(.gltf/.glb/.cxmf):" << cmd::endl;
#else
	cmd::cout << "(.cxmf):" << cmd::endl;
#endif
	cmd::cout << cmd::clr_green << currentWorkDir << "> ";

	std::string modelPath;
	readInput(modelPath);
	if (modelPath.empty())
	{
		needToClearScreen = true;
		return;
	}

	cmd::cout << cmd::clear << cmd::clr_gray;

	currentModel = cxmf::LoadFromFile(modelPath.c_str(), &consoleLogger);
	if (currentModel)
	{
		currentAssetPath = std::filesystem::absolute(modelPath).string();
		currentModelMenu = ModelMenuType::MAIN;
	}
	else
	{
		cmd::cout << cmd::clr_red << "Failed to load: " << modelPath;
		cmd::cout << cmd::endl << cmd::endl;
	}
}

static void option_RenameModel()
{
	const std::function<bool(const std::string&)> validator = [](const std::string& newName) -> bool
	{
		for (char ch : newName)
		{
			if (ch != '-' && ch != '_' && !std::isalnum(static_cast<unsigned char>(ch)))  //
				return false;
		}
		return true;
	};

	const std::string newName = processSelectString("name", validator);
	if (newName.empty()) return;

	currentModel->name = newName;
	cmd::cout << cmd::clr_green << "CHANGED!" << cmd::endl;

	PAUSE(2000);
}

static void option_changeTexturePath()
{
	const int index = processSelectIndex(static_cast<int>(currentModel->textures.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::Texture& tex : currentModel->textures)
		{
			if (tex.path == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("path", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = currentModel->textures[index].path;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_RenameSampler()
{
	const int index = processSelectIndex(static_cast<int>(currentModel->samplers.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::Sampler& sampler : currentModel->samplers)
		{
			if (sampler.name == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("name", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = currentModel->samplers[index].name;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_RenameMaterial()
{
	const int index = processSelectIndex(static_cast<int>(currentModel->materials.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::Material& mat : currentModel->materials)
		{
			if (mat.name == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("name", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = currentModel->materials[index].name;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_RenameMesh()
{
	const int index = processSelectIndex(static_cast<int>(currentModel->meshes.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::Mesh& mesh : currentModel->meshes)
		{
			if (mesh.name == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("name", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = currentModel->meshes[index].name;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_RenameNode()
{
	const int index = processSelectIndex(static_cast<int>(currentModel->meshNodes.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::MeshHierarchy& node : currentModel->meshNodes)
		{
			if (node.name == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("name", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = currentModel->meshNodes[index].name;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_RenameBone()
{
	cxmf::SkinnedModel& model = *currentModel->SkinnedModelCast();

	const int index = processSelectIndex(static_cast<int>(model.bones.size()));
	if (index == -1) return;

	std::function<bool(const std::string&)> pathValidator = [&model](const std::string& newName) -> bool
	{
		bool exists = false;
		for (const cxmf::Bone& bone : model.bones)
		{
			if (bone.name == newName)
			{
				exists = true;
				break;
			}
		}
		return !exists;
	};

	const std::string newName = processSelectString("name", pathValidator);
	if (newName.empty()) return;

	std::string& oldName = model.bones[index].name;
	cmd::cout << cmd::clr_green << std::format("Changed: \"{}\" >> \"{}\"", oldName, newName);
	oldName = newName;

	PAUSE(2000);
}

static void option_showModelTextures()
{
	cmd::cout << cmd::endl;
	for (size_t i = 0; i < currentModel->textures.size(); ++i)
	{
		const cxmf::Texture& tex = currentModel->textures[i];
		cmd::cout << "ID: " << i << ", Path: \"" << tex.path << "\"\n";

		cmd::cout << "\tSampler ID: ";
		if (tex.HasSampler())
		{
			cmd::cout << tex.samplerIndex << " \"" << currentModel->samplers[tex.samplerIndex].name << '\"';
		}
		else
		{
			cmd::cout << "<NO-SAMPLER>";
		}
		cmd::cout << '\n';

		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->textures.empty())
	{
		optionControl.add("Change path", &option_changeTexturePath);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_showModelSamplers()
{
	const auto toStringFilter = [](cxmf::Sampler::Filter mode) -> std::string_view
	{
		switch (mode)
		{
			case cxmf::Sampler::Filter::NEAREST:
				return "NEAREST";
			case cxmf::Sampler::Filter::LINEAR:
				return "LINEAR";
			default:
				return "UNKNOWN";
		}
	};
	const auto toStringMipmapMode = [](cxmf::Sampler::MipmapMode mode) -> std::string_view
	{
		switch (mode)
		{
			case cxmf::Sampler::MipmapMode::NONE:
				return "NONE";
			case cxmf::Sampler::MipmapMode::NEAREST:
				return "NEAREST";
			case cxmf::Sampler::MipmapMode::LINEAR:
				return "LINEAR";
			default:
				return "UNKNOWN";
		}
	};
	const auto toStringAddressMode = [](cxmf::Sampler::AddressMode mode) -> std::string_view
	{
		switch (mode)
		{
			case cxmf::Sampler::AddressMode::REPEAT:
				return "REPEAT";
			case cxmf::Sampler::AddressMode::MIRRORED_REPEAT:
				return "MIRRORED_REPEAT";
			case cxmf::Sampler::AddressMode::CLAMP_TO_EDGE:
				return "CLAMP_TO_EDGE";
			default:
				return "UNKNOWN";
		}
	};

	cmd::cout << cmd::endl;
	for (size_t i = 0; i < currentModel->samplers.size(); ++i)
	{
		const cxmf::Sampler& sampler = currentModel->samplers[i];

		cmd::cout << "ID: " << i << ", Name: \"" << sampler.name << "\"\n";
		cmd::cout << "\tMag filter: " << toStringFilter(sampler.magFilter) << '\n';
		cmd::cout << "\tMin filter: " << toStringFilter(sampler.minFilter) << '\n';
		cmd::cout << "\tMipmap mode: " << toStringMipmapMode(sampler.mipmapMode) << '\n';
		cmd::cout << "\tAddress mode U: " << toStringAddressMode(sampler.addressModeU) << '\n';
		cmd::cout << "\tAddress mode V: " << toStringAddressMode(sampler.addressModeV) << '\n';
		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->samplers.empty())
	{
		optionControl.add("Rename sampler", &option_RenameSampler);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_showModelMaterials()
{
	const auto toStringAlphaMode = [](cxmf::Material::AlphaMode mode) -> std::string_view
	{
		switch (mode)
		{
			case cxmf::Material::AlphaMode::OPAQUE:
				return "OPAQUE";
			case cxmf::Material::AlphaMode::MASK:
				return "MASK";
			case cxmf::Material::AlphaMode::BLEND:
				return "BLEND";
			default:
				return "UNKNOWN";
		}
	};

	cmd::cout << cmd::endl;
	for (size_t i = 0; i < currentModel->materials.size(); ++i)
	{
		const cxmf::Material& mat = currentModel->materials[i];

		cmd::cout << "ID: " << i << ", Name: \"" << mat.name << "\"\n";

		cmd::cout << "\tBase color (RGBA): ";
		cmd::cout << mat.baseColorFactor[0] << ", " << mat.baseColorFactor[1] << ", ";
		cmd::cout << mat.baseColorFactor[2] << ", " << mat.baseColorFactor[3] << '\n';

		cmd::cout << "\tRoughness: " << mat.roughnessFactor << '\n';

		cmd::cout << "\tMetallic: " << mat.metallicFactor << '\n';

		cmd::cout << "\tOcclusion: " << mat.ambientOcclusionFactor << '\n';

		cmd::cout << "\tEmissive (RGB): " << mat.emissiveFactor[0] << ", ";
		cmd::cout << mat.emissiveFactor[1] << ", " << mat.emissiveFactor[2] << '\n';

		cmd::cout << "\tTexture ID: ";
		if (mat.HasTexture())
			cmd::cout << mat.textureIndex << " \"" << currentModel->textures[mat.textureIndex].path << '\"';
		else
			cmd::cout << "<NO-TEXTURE>";
		cmd::cout << '\n';

		cmd::cout << "\tAlpha mode: " << toStringAlphaMode(mat.alphaMode) << '\n';

		if (mat.alphaMode == cxmf::Material::AlphaMode::MASK)
		{
			cmd::cout << "\tAlpha cutoff: " << mat.alphaCutoff << '\n';
		}

		cmd::cout << "\tDouble sided: " << (mat.doubleSided ? "ON" : "OFF") << '\n';

		cmd::cout << "\tShadeless: " << (mat.shadeless ? "ON" : "OFF") << '\n';

		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->materials.empty())
	{
		optionControl.add("Rename material", &option_RenameMaterial);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_showModelMeshes()
{
	cmd::cout << cmd::endl;
	for (size_t i = 0; i < currentModel->meshes.size(); ++i)
	{
		const cxmf::Mesh& mesh = currentModel->meshes[i];

		cmd::cout << "ID: " << i << ", Name: \"" << mesh.name << "\"\n";

		cmd::cout << "\tVertices: " << mesh.vertexCount << '\n';

		cmd::cout << "\tMeshlets: " << mesh.meshletCount << '\n';

		cmd::cout << "\tMaterial ID: ";
		if (mesh.HasMaterial())
			cmd::cout << mesh.materialIndex << " \"" << currentModel->materials[mesh.materialIndex].name << '\"';
		else
			cmd::cout << "<NO-MATERIAL>";
		cmd::cout << '\n';

		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->meshes.empty())
	{
		optionControl.add("Rename mesh", &option_RenameMesh);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_showModelNodes()
{
	cmd::cout << cmd::endl;
	for (size_t i = 0; i < currentModel->meshNodes.size(); ++i)
	{
		const cxmf::MeshHierarchy& node = currentModel->meshNodes[i];

		cmd::cout << "ID: " << i << ", Name: \"" << node.name << "\"\n";

		cmd::cout << "\tParent ID: ";
		if (node.HasParent())
			cmd::cout << node.parentIndex << " \"" << currentModel->meshNodes[node.parentIndex].name << '\"';
		else
			cmd::cout << "<NO-PARENT>";
		cmd::cout << '\n';

		cmd::cout << "\tMesh ID: " << node.meshIndex << " \"" << currentModel->meshes[node.meshIndex].name << "\"\n";

		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->meshNodes.empty())
	{
		optionControl.add("Rename node", &option_RenameNode);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_showModelBones()
{
	const cxmf::SkinnedModel& model = *currentModel->SkinnedModelCast();

	cmd::cout << cmd::endl;
	for (size_t i = 0; i < model.bones.size(); ++i)
	{
		const cxmf::Bone& bone = model.bones[i];

		cmd::cout << "ID: " << i << ", Name: \"" << bone.name << "\"\n";

		cmd::cout << "\tParent ID: ";
		if (bone.HasParent())
			cmd::cout << bone.parentIndex << " \"" << model.bones[bone.parentIndex].name << '\"';
		else
			cmd::cout << "<NO-PARENT>";
		cmd::cout << '\n';

		cmd::cout << cmd::endl;
	}
	cmd::cout << cmd::endl;

	if (!model.bones.empty())
	{
		optionControl.add("Rename bone", &option_RenameBone);
	}
	optionControl.add("Back", []() { currentModelMenu = ModelMenuType::MAIN; });
	optionControl.select();

	needToClearScreen = true;
}

static void option_SaveModel()
{
	cmd::cout << cmd::clear;
	cmd::cout << "Enter directory path:" << cmd::endl;
	cmd::cout << cmd::clr_green << currentWorkDir << "> ";

	std::string saveDir;
	readInput(saveDir);
	trimStringSpaces(saveDir);
	cmd::cout << cmd::clr_gray;

	std::string compMode;
	cmd::cout << "0 - none, 1 - normal, 2 - max speed, 3 - max compression\n";
	cmd::cout << "Enter compression mode (default: 1):\n";
	cmd::cout << cmd::clr_green << ">> ";
	readInput(compMode);

	int mode;
	if (compMode.empty())
	{
		mode = 1;
	}
	else
	{
		try
		{
			mode = std::stoi(compMode);
		}
		catch (...)
		{
			mode = 1;
		}
	}

	cxmf::CompressionLevel compLevel;
	switch (mode)
	{
		case 0:
		{
			compLevel = cxmf::CompressionLevel::NONE;
			break;
		}
		case 2:
		{
			compLevel = cxmf::CompressionLevel::SPEED;
			break;
		}
		case 3:
		{
			compLevel = cxmf::CompressionLevel::MIN_SIZE;
			break;
		}
		default:
		{
			compLevel = cxmf::CompressionLevel::DEFAULT;
			break;
		}
	}
	cmd::cout << cmd::clear;

	if (cxmf::SaveToFile(*currentModel, saveDir.c_str(), compLevel, &consoleLogger))
		cmd::cout << cmd::clr_green << "SAVED!";
	else
		cmd::cout << cmd::clr_red << "FAILED!";

	PAUSE(2000);
}

static void showModelMainMenu()
{
	cmd::cout << cmd::clr_green;
	cmd::cout << currentAssetPath << cmd::endl;
	cmd::cout << "Name: " << currentModel->name << cmd::endl;
	cmd::cout << "Type: ";
	switch (currentModel->GetType())
	{
		case cxmf::ModelType::STATIC:
			cmd::cout << "STATIC";
			break;
		case cxmf::ModelType::SKINNED:
			cmd::cout << "SKINNED";
			break;
		default:
			cmd::cout << "UNKNOWN";
			break;
	}
	cmd::cout << cmd::endl;

	if (!currentModel->copyright.empty())
	{
		cmd::cout << "Copyright: " << currentModel->copyright << cmd::endl;
	}
	if (!currentModel->generator.empty())
	{
		cmd::cout << "Generator: " << currentModel->generator << cmd::endl;
	}

	cmd::cout << cmd::clr_gray;

	optionControl.reset();
	switch (currentModelMenu)
	{
		case ModelMenuType::MAIN:
		{
			optionControl.add("Textures", []() { currentModelMenu = ModelMenuType::TEXTURES; });
			optionControl.add("Samplers", []() { currentModelMenu = ModelMenuType::SAMPLERS; });
			optionControl.add("Materials", []() { currentModelMenu = ModelMenuType::MATERIALS; });
			optionControl.add("Meshes", []() { currentModelMenu = ModelMenuType::MESHES; });
			optionControl.add("Nodes", []() { currentModelMenu = ModelMenuType::NODES; });
			if (currentModel->GetType() == cxmf::ModelType::SKINNED)
			{
				optionControl.add("Bones", []() { currentModelMenu = ModelMenuType::BONES; });
			}
			break;
		}
		case ModelMenuType::TEXTURES:
		{
			option_showModelTextures();
			return;
		}
		case ModelMenuType::SAMPLERS:
		{
			option_showModelSamplers();
			return;
		}
		case ModelMenuType::MATERIALS:
		{
			option_showModelMaterials();
			return;
		}
		case ModelMenuType::MESHES:
		{
			option_showModelMeshes();
			return;
		}
		case ModelMenuType::NODES:
		{
			option_showModelNodes();
			return;
		}
		case ModelMenuType::BONES:
		{
			option_showModelBones();
			return;
		}
		default:
			break;
	}

	optionControl.add("Rename model", &option_RenameModel);
	optionControl.add("Save", &option_SaveModel);
	optionControl.add("Exit", &option_Exit);
	optionControl.select();
	needToClearScreen = true;
}

static void showMainMenu()
{
	cmd::cout << cmd::clr_green;
	cmd::cout << std::format("--- CXMF Editor ver {}.{}.{}, build {} {} ---\n",	 //
							 CXMF_VERSION_MAJOR, CXMF_VERSION_MINOR, CXMF_VERSION_PATCH, __DATE__, __TIME__);
	cmd::cout << cmd::clr_gray;

	optionControl.reset();
	optionControl.add("Load model", &option_LoadModel);
	optionControl.add("Exit", &option_Exit);
	optionControl.select();
}

static void main_loop()
{
	clearScreen();

	if (currentModel)
	{
		showModelMainMenu();
	}
	else
	{
		showMainMenu();
	}
}



int main(int /* argc */, char** /* argv */)
{
	currentWorkDir = std::filesystem::current_path().string();

	while (!needToExit)
	{
		main_loop();
	}

	cxmf::Free(currentModel);
	return 0;
}
