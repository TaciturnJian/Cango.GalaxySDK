#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK {
	struct DeviceIdentifier final {
		enum IdentifierType : std::uint8_t {
			SerialNumber,
			IPAddress,
			MACAddress,
			DeviceIndex,
			UserDefinedID
		};

		IdentifierType Type{};
		std::string Content{};

		friend std::ostream& operator<<(std::ostream& stream, IdentifierType type) noexcept;

		std::ostream& Format(std::ostream& stream) const noexcept;

		explicit operator GX_OPEN_PARAM() const noexcept;
	};
}
