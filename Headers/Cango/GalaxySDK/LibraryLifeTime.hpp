#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK:: inline LibraryLifeTime {
	bool Initialize(spdlog::logger& logger) noexcept;

	bool Close(spdlog::logger& logger) noexcept;

	void CloseWhatever(spdlog::logger& logger) noexcept;
}
