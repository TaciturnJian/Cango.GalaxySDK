#pragma once

#include <GxIAPI.h>
#include <span>
#include <string>
#include <spdlog/spdlog.h>

namespace Cango::GalaxySDK:: inline Utils {
	bool GetErrorInfo(spdlog::logger& logger, GX_STATUS& error, std::size_t& size) noexcept;

	bool GetErrorContent(
		spdlog::logger& logger,
		const GX_STATUS& error,
		const std::size_t& size,
		std::span<char> buffer) noexcept;

	bool GetErrorMessage(spdlog::logger& logger, std::string& message) noexcept;

	std::string GetErrorMessage(spdlog::logger& logger) noexcept;
}
