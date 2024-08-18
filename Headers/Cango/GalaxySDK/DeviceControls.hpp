#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK:: inline DeviceControls {
	bool CloseDevice(
		spdlog::logger& logger,
		const GX_DEV_HANDLE& deviceHandle) noexcept;

	bool SendCommand(
		spdlog::logger& logger,
		const GX_DEV_HANDLE& deviceHandle,
		const GX_FEATURE_ID& command) noexcept;

	bool StartCapture(
		spdlog::logger& logger,
		const GX_DEV_HANDLE& deviceHandle) noexcept;

	bool StopCapture(
		spdlog::logger& logger,
		const GX_DEV_HANDLE& deviceHandle) noexcept;

	bool GetImage(
		spdlog::logger& logger,
		const GX_DEV_HANDLE& deviceHandle,
		cv::Mat& image,
		void* imageBuffer = nullptr) noexcept;
}
