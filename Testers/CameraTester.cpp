#ifdef _WINDOWS
#undef max
#undef min
#endif
#include <opencv2/opencv.hpp>

#include <GalaxySDK.hpp>

#include <Cango/CommonUtils/CallRateCounterX.hpp>

using namespace Cango;
using namespace std::chrono_literals;

int main() {
	const auto logger_user = spdlog::default_logger();
	auto& logger = *logger_user;

	if (!GalaxySDK::Initialize(logger)) return 1;

	std::uint32_t device_count{};
	if (!GalaxySDK::RefreshDeviceInfoList(logger, device_count)) return 2;
	if (device_count < 1) return 3;

	GalaxySDK::GxCameraCheatsheet cheatsheet{};
	{
		{
			auto&& config = cheatsheet.Provider->Configure();
			const auto& actors = config.Actors;
			actors.Logger = logger_user;
			actors.CameraLogger = logger_user;
			const auto& options = config.Options;
			auto& open_params = options.OpenParameter;
			open_params.UseOrder = true;
			open_params.Order = 1;
			auto& cam_params = options.ConfigureParameter;
			cam_params.AutoExposure.Value = false;
			cam_params.ExposureTime.Value = 6000;
			cam_params.AutoGain.Value = false;
			cam_params.Gain.Value = 16.0f;
			cam_params.RedBalanceRatio.Value = 1.5352f;
			cam_params.GreenBalanceRatio.Value = 1;
			cam_params.BlueBalanceRatio.Value = 1.5664;
		}
		{
			auto&& config = cheatsheet.Consumer->Configure();
			config.Options.MinInterval = 0ms;
		}
	}

	auto consumer = [&cheatsheet] {
		CallRateCounterX<std::uint32_t> counter{};
		IntervalSleeper sleeper{6ms};
		cv::Mat image{};

		auto& pool = cheatsheet.ImagePool;
		auto begin = std::chrono::steady_clock::now();
		for (int i = 0; i < 10000; i ++) {
			sleeper.Sleep();
			if (!pool->GetItem(image)) continue;

			const auto now = std::chrono::steady_clock::now();
			auto fps = counter.Call(now);
			/*cv::imshow("sample", image);
			cv::waitKey(1);*/
			if (now - begin > 1s) {
				begin = now;
				spdlog::info("FPS: {}", fps);
			}
		}

		cheatsheet.DeliveryMonitor->Interrupt();
		cheatsheet.CaptureMonitor->Interrupt();
	};

	ThreadList threads{};
	threads << cheatsheet.Task << consumer;
	JoinThreads(threads);

	GalaxySDK::Close(logger);
}
