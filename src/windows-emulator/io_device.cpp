#include "io_device.hpp"

namespace
{
	struct dummy_device : stateless_device
	{
		void read() override
		{
		}

		void write() override
		{
		}
	};
}

std::unique_ptr<io_device> create_device(const std::wstring_view device)
{
	if (device == L"CNG"
		|| device == L"KsecDD"
		|| device == L"DeviceApi\\CMApi"
		|| device == L"ConDrv\\Server"
		|| device == L"Afd\\Endpoint")
	{
		return std::make_unique<dummy_device>();
	}

	throw std::runtime_error("Unsupported device: " + std::string(device.begin(), device.end()));
}