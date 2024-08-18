#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK:: inline QueryDevices {
	bool RefreshDeviceInfoList(
		spdlog::logger& logger,
		std::uint32_t& deviceCount) noexcept;

	bool FillDeviceInfoList(
		spdlog::logger& logger,
		std::span<GX_DEVICE_BASE_INFO> list,
		std::size_t& filledCount) noexcept;

	bool GetDeviceInfoList(
		spdlog::logger& logger,
		std::vector<GX_DEVICE_BASE_INFO>& list) noexcept;

	bool GetDeviceByOrder(
		spdlog::logger& logger,
		std::uint32_t order,
		GX_DEV_HANDLE& deviceHandle) noexcept;

	bool GetDeviceByIdentifier(
		spdlog::logger& logger,
		const DeviceIdentifier& id,
		GX_DEV_HANDLE& deviceHandle) noexcept;
}
