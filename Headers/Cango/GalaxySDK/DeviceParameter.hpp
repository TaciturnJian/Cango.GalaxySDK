#pragma once

#include "Utils.hpp"

namespace Cango::GalaxySDK {
	template <typename TValue>
	using GxGetFuncPtr = GX_STATUS (*)(GX_DEV_HANDLE, GX_FEATURE_ID_CMD, TValue*);

	template <typename TValue>
	using GxSetFuncPtr = GX_STATUS (*)(GX_DEV_HANDLE, GX_FEATURE_ID_CMD, TValue);

	template <
		typename TValue,
		GxGetFuncPtr<TValue> TGet,
		GxSetFuncPtr<TValue> TSet,
		GX_FEATURE_ID_CMD TFeature>
	struct DeviceParameter {
		TValue Value{};

		bool Get(spdlog::logger& logger, const GX_DEV_HANDLE device) noexcept {
			if (TGet(device, TFeature, &Value) == GX_STATUS_SUCCESS) return true;
			logger.error("无法获取相机参数({}): {}", TFeature, GetErrorMessage(logger));
			return false;
		}

		bool Set(spdlog::logger& logger, const GX_DEV_HANDLE device) const noexcept {
			if (TSet(device, TFeature, Value) == GX_STATUS_SUCCESS) return true;
			logger.error("无法设置相机参数({}): {}", TFeature, GetErrorMessage(logger));
			return false;
		}

		static bool GetIsImplemented(spdlog::logger& logger, const GX_DEV_HANDLE device, bool& isImplemented) noexcept {
			if (GXIsImplemented(device, TFeature, &isImplemented) == GX_STATUS_SUCCESS) return true;
			logger.error("无法获取相机参数({})是否实现：{}", TFeature, GetErrorMessage(logger));
			return false;
		}
	};

	template <GX_FEATURE_ID_CMD TFeature>
	using DeviceIntParameter = DeviceParameter<std::int64_t, GXGetInt, GXSetInt, TFeature>;

	template <GX_FEATURE_ID_CMD TFeature>
	using DeviceFloatParameter = DeviceParameter<double, GXGetFloat, GXSetFloat, TFeature>;

	template <GX_FEATURE_ID_CMD TFeature>
	using DeviceBoolParameter = DeviceParameter<bool, GXGetBool, GXSetBool, TFeature>;

	template <GX_FEATURE_ID_CMD TFeature, typename TEnum>
	struct DeviceEnumParameter {
		TEnum Value;

		bool Get(spdlog::logger& logger, const GX_DEV_HANDLE device) noexcept {
			std::int64_t value{};
			if (GXGetEnum(device, TFeature, &value) == GX_STATUS_SUCCESS) {
				Value = static_cast<TEnum>(value);
				return true;
			}
			logger.error("无法获取相机参数({}): {}", TFeature, GetErrorMessage(logger));
			return false;
		}

		bool Set(spdlog::logger& logger, const GX_DEV_HANDLE device) noexcept {
			if (const auto value = static_cast<std::int64_t>(Value);
				GXSetEnum(device, TFeature, value) == GX_STATUS_SUCCESS)
				return true;
			logger.error("无法设置相机参数({}): {}", TFeature, GetErrorMessage(logger));
			return false;
		}

		static bool GetIsImplemented(spdlog::logger& logger, const GX_DEV_HANDLE device, bool& isImplemented) noexcept {
			if (GXIsImplemented(device, TFeature, &isImplemented) == GX_STATUS_SUCCESS) return true;
			logger.error("无法获取相机参数({})是否实现：{}", TFeature, GetErrorMessage(logger));
			return false;
		}
	};

	namespace Parameters {
		using PayLoadSize = DeviceIntParameter<GX_INT_PAYLOAD_SIZE>;

		using Gain = DeviceFloatParameter<GX_FLOAT_GAIN>;
		using AutoGainMax = DeviceFloatParameter<GX_FLOAT_AUTO_GAIN_MAX>;
		using AutoGainMin = DeviceFloatParameter<GX_FLOAT_AUTO_GAIN_MIN>;
		using AutoGain = DeviceEnumParameter<GX_ENUM_GAIN_AUTO, GX_GAIN_AUTO_ENTRY>;

		using ExposureTime = DeviceFloatParameter<GX_FLOAT_EXPOSURE_TIME>;
		using AutoExposureTimeMax = DeviceFloatParameter<GX_FLOAT_AUTO_EXPOSURE_TIME_MAX>;
		using AutoExposureTimeMin = DeviceFloatParameter<GX_FLOAT_AUTO_EXPOSURE_TIME_MIN>;
		using AutoExposure = DeviceEnumParameter<GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_ENTRY>;

		using BalanceRatioSelector = DeviceEnumParameter<
			GX_ENUM_BALANCE_RATIO_SELECTOR,
			GX_BALANCE_RATIO_SELECTOR_ENTRY>;
		using BalanceRatio = DeviceFloatParameter<GX_FLOAT_BALANCE_RATIO>;
		using AutoBalance = DeviceEnumParameter<GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_ENTRY>;
	}
}
