#include "CXMF.hpp"

#include <iostream>



class CXMFConsoleLogger final : public cxmf::Logger
{
public:
	void write(const char* message) override
	{
		std::cout << message << std::endl;
	}
};



int main(int /* argc */, char** /* argv */)
{
	CXMFConsoleLogger logger;
	cxmf::Model* const model = cxmf::LoadFromFile("", &logger);
	if (!model) return 1;

	cxmf::Free(model);
	return 0;
}
