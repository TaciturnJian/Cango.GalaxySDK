#pragma once

#include <memory>
#include <opencv2/core.hpp>

#include <Cango/CommonUtils/ObjectOwnership.hpp>
#include <Cango/TaskDesign/DeliveryTask.hpp>
#include <Cango/CommonUtils/AsyncItemPool.hpp>

#include "GalaxySDK/Utils.hpp"
#include "GalaxySDK/LibraryLifeTime.hpp"
#include "GalaxySDK/DeviceIdentifier.hpp"
#include "GalaxySDK/QueryDevices.hpp"
#include "GalaxySDK/DeviceParameter.hpp"
#include "GalaxySDK/DeviceControls.hpp"
#include "GalaxySDK/GxCamera.hpp"

namespace Cango::GalaxySDK {
	struct GxCameraOpenParameter {
		bool UseOrder{false};
		std::uint32_t Order{1};
		DeviceIdentifier Identifier{};
	};

	struct GxCameraConfigureParameter {
		Parameters::Gain Gain{12};
		Parameters::AutoGainMax AutoGainMax{12};
		Parameters::AutoGainMin AutoGainMin{2};
		Parameters::AutoGain AutoGain{GX_GAIN_AUTO_OFF};

		Parameters::ExposureTime ExposureTime{4000};
		Parameters::AutoExposureTimeMax AutoExposureTimeMax{10000};
		Parameters::AutoExposureTimeMin AutoExposureTimeMin{1000};
		Parameters::AutoExposure AutoExposure{GX_EXPOSURE_AUTO_OFF};

		Parameters::BalanceRatio RedBalanceRatio{1.00f};
		Parameters::BalanceRatio GreenBalanceRatio{1.00f};
		Parameters::BalanceRatio BlueBalanceRatio{1.00f};

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

		using ItemType = Owner<GxCamera>;
		bool GetItem(Owner<GxCamera>& camera) noexcept;
	};

	using CameraCaptureTask = DeliveryTask<GxCamera, TripleItemPool<cv::Mat>, EasyDeliveryTaskMonitor>;

	class GxCameraConsumer {
		CameraCaptureTask Task{};

	public:
		auto Configure() noexcept { return Task.Configure(); }

		[[nodiscard]] bool IsFunctional() noexcept {
			const auto actors = Task.Configure().Actors;
			return !actors.Monitor.expired() && !actors.ItemDestination.expired();
		}

		using ItemType = Owner<GxCamera>;

		void SetItem(const Owner<GxCamera>& camera) noexcept {
			const auto actors = Task.Configure().Actors;

			// 参数中的 camera 是一个指向所有者的引用，这里获取一个 user 防止资源在 Task.Execute 时被释放
			const ObjectUser<GxCamera> camera_user{camera};
			const auto monitor_user = actors.Monitor.lock();

			if (!camera_user || !monitor_user) return;
			
			actors.ItemSource = camera_user;
			monitor_user->Reset();

			Task.Execute();
		}
	};

	using CameraDeliveryTask = DeliveryTask<GxCameraProvider, GxCameraConsumer, EasyDeliveryTaskMonitor>;

	struct GxCameraCheatsheet {
		Owner<TripleItemPool<cv::Mat>> ImagePool{};
		Owner<GxCameraProvider> Provider{};
		Owner<GxCameraConsumer> Consumer{};
		Owner<EasyDeliveryTaskMonitor> DeliveryMonitor{};
		Owner<EasyDeliveryTaskMonitor> CaptureMonitor{};
		CameraDeliveryTask Task{};

		GxCameraCheatsheet();
	};

	struct TimedPicture {
		std::chrono::steady_clock::time_point Time;
		cv::Mat Frame;
	};

	struct TimedGxCamera {
		Owner<GxCamera> InnerCamera{};

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

		[[nodiscard]] bool IsFunctional() const noexcept { return Provider.IsFunctional(); }

		using ItemType = Owner<TimedGxCamera>;

		bool GetItem(Owner<TimedGxCamera>& camera) noexcept {
			Owner<TimedGxCamera> new_camera{};
			if (!Provider.GetItem(new_camera->InnerCamera)) return false;
			camera = std::move(new_camera);
			return true;
		}
	};

	class TimedGxCameraConsumer {};
}
