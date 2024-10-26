#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK {
	struct GxCamera {
		std::unique_ptr<GX_DEV_HANDLE> DeviceHandle{std::make_unique<GX_DEV_HANDLE>()};
		ObjectUser<spdlog::logger> Logger{};
		std::vector<std::uint8_t> ImageBuffer{};

		using ItemType = cv::Mat;
		bool GetItem(cv::Mat& image) noexcept;

		~GxCamera() noexcept {
			if (DeviceHandle == nullptr || *DeviceHandle == nullptr) return;
			auto& logger = Logger ? *Logger : *spdlog::default_logger();
			const auto& device = *DeviceHandle;
			(void)StopCapture(logger, device);
			(void)CloseDevice(logger, device);
		}
	};
}