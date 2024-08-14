#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <opencv2/core.hpp>
#include <GxIAPI.h>

#include <Cango/TaskDesign/ObjectOwnership.hpp>
#include <Cango/TaskDesign/DeliveryTask.hpp>
#include <Cango/CommonUtils/AsyncItemPool.hpp>

namespace Cango::GalaxySDK {
	inline namespace Utils {
		bool GetErrorInfo(spdlog::logger& logger, GX_STATUS& error, std::size_t& size) noexcept;

		bool GetErrorContent(
			spdlog::logger& logger,
			const GX_STATUS& error,
			const std::size_t& size,
			std::span<char> buffer) noexcept;

		bool GetErrorMessage(spdlog::logger& logger, std::string& message) noexcept;

		std::string GetErrorMessage(spdlog::logger& logger) noexcept;
	}

	inline namespace LibraryLifeTime {
		bool Initialize(spdlog::logger& logger) noexcept;

		bool Close(spdlog::logger& logger) noexcept;

		void CloseWhatever(spdlog::logger& logger) noexcept;
	}

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

	inline namespace QueryDevices {
		bool RefreshDeviceInfoList(spdlog::logger& logger, std::uint32_t& deviceCount) noexcept;

		bool FillDeviceInfoList(spdlog::logger& logger, std::span<GX_DEVICE_BASE_INFO> list,
		                        std::size_t& filledCount) noexcept;

		bool GetDeviceInfoList(spdlog::logger& logger, std::vector<GX_DEVICE_BASE_INFO>& list) noexcept;

		bool GetDeviceByOrder(spdlog::logger& logger, std::uint32_t order, GX_DEV_HANDLE& deviceHandle) noexcept;

		bool GetDeviceByIdentifier(
			spdlog::logger& logger,
			const DeviceIdentifier& id,
			GX_DEV_HANDLE& deviceHandle) noexcept;
	}

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
	class DeviceEnumParameter {
		std::int64_t RecordValue{};

	public:
		TEnum Value;

		bool Get(spdlog::logger& logger, const GX_DEV_HANDLE device) noexcept {
			if (GXGetEnum(device, TFeature, &RecordValue) == GX_STATUS_SUCCESS) {
				Value = static_cast<TEnum>(RecordValue);
				return true;
			}
			logger.error("无法获取相机参数({}): {}", TFeature, GetErrorMessage(logger));
			return false;
		}

		bool Set(spdlog::logger& logger, const GX_DEV_HANDLE device) noexcept {
			RecordValue = static_cast<std::int64_t>(Value);
			if (GXSetEnum(device, TFeature, RecordValue) == GX_STATUS_SUCCESS) return true;
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

	inline namespace DeviceControls {
		bool CloseDevice(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept;

		bool SendCommand(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle,
		                 const GX_FEATURE_ID& command) noexcept;

		bool StartCapture(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept;

		bool StopCapture(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept;

		bool GetImage(
			spdlog::logger& logger, 
			const GX_DEV_HANDLE& deviceHandle, 
			cv::Mat& image,
			void* imageBuffer = nullptr) noexcept;
	}

	struct GxCamera {
		std::unique_ptr<GX_DEV_HANDLE> DeviceHandle{};
		ObjectUser<spdlog::logger> Logger{};
		std::vector<std::uint8_t> ImageBuffer{};

		using ItemType = cv::Mat;
		bool GetItem(cv::Mat& image) noexcept;

		~GxCamera() noexcept {
			if (DeviceHandle == nullptr || *DeviceHandle == nullptr) return;
			auto& logger = Logger.IsValid() ? *Logger : *spdlog::default_logger();
			const auto& device = *DeviceHandle;
			(void)StopCapture(logger, device);
			(void)CloseDevice(logger, device);
		}
	};

	using CameraCaptureTask = DeliveryTask<GxCamera, TripleItemPool<cv::Mat>, EasyDeliveryTaskMonitor>;

	struct GxCameraOpenParameter {
		bool UseOrder{false};
		std::uint32_t Order{1};
		DeviceIdentifier Identifier{};
	};

	struct GxCameraConfigureParameter {
		Parameters::Gain Gain{};
		Parameters::AutoGainMax AutoGainMax{};
		Parameters::AutoGainMin AutoGainMin{};
		Parameters::AutoGain AutoGain{};

		Parameters::ExposureTime ExposureTime{};
		Parameters::AutoExposureTimeMax AutoExposureTimeMax{};
		Parameters::AutoExposureTimeMin AutoExposureTimeMin{};
		Parameters::AutoExposure AutoExposure{};

		Parameters::BalanceRatio RedBalanceRatio{};
		Parameters::BalanceRatio GreenBalanceRatio{};
		Parameters::BalanceRatio BlueBalanceRatio{};

		bool Apply(spdlog::logger& logger, const GX_DEV_HANDLE device) {
			AutoGain.Set(logger, device);
			if (AutoGain.Value != GX_GAIN_AUTO_OFF) {
				if (!AutoGainMax.Set(logger, device)) return false;
				if (!AutoGainMin.Set(logger, device)) return false;
			}
			else { if (!Gain.Set(logger, device)) return false; }

			AutoExposure.Set(logger, device);
			if (AutoExposure.Value != GX_EXPOSURE_AUTO_OFF) {
				if (!AutoExposureTimeMax.Set(logger, device)) return false;
				if (!AutoExposureTimeMin.Set(logger, device)) return false;
			}
			else { if (!ExposureTime.Set(logger, device)) return false; }

			Parameters::AutoBalance auto_balance{};
			auto_balance.Value = GX_BALANCE_WHITE_AUTO_OFF;
			if (!auto_balance.Set(logger, device)) return false;

			Parameters::BalanceRatioSelector selector{};
			selector.Value = GX_BALANCE_RATIO_SELECTOR_RED;
			if (!selector.Set(logger, device)) return false;
			if (!RedBalanceRatio.Set(logger, device)) return false;

			selector.Value = GX_BALANCE_RATIO_SELECTOR_GREEN;
			if (!selector.Set(logger, device)) return false;
			if (!GreenBalanceRatio.Set(logger, device)) return false;

			selector.Value = GX_BALANCE_RATIO_SELECTOR_BLUE;
			if (!selector.Set(logger, device)) return false;
			if (!BlueBalanceRatio.Set(logger, device)) return false;
			return true;
		}
	};

	class GxCameraProvider {
		ObjectUser<spdlog::logger> Logger{};
		ObjectUser<spdlog::logger> CameraLogger{};

		GxCameraOpenParameter OpenParameter{};
		GxCameraConfigureParameter ConfigureParameter{};

		struct Configurations {
			struct ActorsType {
				ObjectUser<spdlog::logger>& Logger;
				ObjectUser<spdlog::logger>& CameraLogger;
			} Actors;

			struct OptionsType {
				GxCameraOpenParameter& OpenParameter;
				GxCameraConfigureParameter& ConfigureParameter;
			} Options;
		};

	public:
		Configurations Configure() noexcept;

		[[nodiscard]] bool IsFunctional() const noexcept;

		using ItemType = ObjectOwner<GxCamera>;
		bool GetItem(ObjectOwner<GxCamera>& camera) noexcept;
	};

	class GxCameraConsumer {
		CameraCaptureTask Task{};

	public:
		auto Configure() noexcept { return Task.Configure(); }

		[[nodiscard]] bool IsFunctional() noexcept {
			auto&& config = Configure();
			const auto actors = config.Actors;
			return Validate(actors.Monitor, actors.ItemDestination);
		}

		using ItemType = ObjectOwner<GxCamera>;

		void SetItem(const ObjectOwner<GxCamera>& camera) noexcept {
			auto&& config = Configure();
			if (!camera.Authorize(config.Actors.ItemSource)) return;
			config.Actors.Monitor.AcquireUser()->Reset();
			Task.Execute();
		}
	};

	using CameraDeliveryTask = DeliveryTask<GxCameraProvider, GxCameraConsumer, EasyDeliveryTaskMonitor>;

	struct GxCameraCheatsheet {
		ObjectOwner<TripleItemPool<cv::Mat>> ImagePool{};
		ObjectOwner<GxCameraProvider> Provider{};
		ObjectOwner<GxCameraConsumer> Consumer{};
		ObjectOwner<EasyDeliveryTaskMonitor> DeliveryMonitor{};
		ObjectOwner<EasyDeliveryTaskMonitor> CaptureMonitor{};
		CameraDeliveryTask Task{};

		GxCameraCheatsheet();
	};

	struct TimedPicture {
		std::chrono::steady_clock::time_point Time;
		cv::Mat Frame;
	};

	struct TimedGxCamera {
		ObjectOwner<GxCamera> InnerCamera{};

		using ItemType = TimedPicture;

		bool GetItem(TimedPicture& picture) const noexcept {
			if (!InnerCamera->GetItem(picture.Frame)) return false;
			picture.Time = std::chrono::steady_clock::now();
			return true;
		}
	};

	using TimedCameraCaptureTask = DeliveryTask<TimedGxCamera, TripleItemPool<TimedPicture>, EasyDeliveryTaskMonitor>;

	class TimedGxCameraProvider {
		GxCameraProvider Provider{};

	public:
		auto Configure() noexcept { return Provider.Configure(); }

		bool IsFunctional() const noexcept { return Provider.IsFunctional(); }

		using ItemType = ObjectOwner<TimedGxCamera>;

		bool GetItem(ObjectOwner<TimedGxCamera>& camera) noexcept {
			ObjectOwner<TimedGxCamera> new_camera{};
			if (!Provider.GetItem(new_camera->InnerCamera)) return false;
			camera = std::move(new_camera);
			return true;
		}
	};

	class TimedGxCameraConsumer {
		
	};
}
