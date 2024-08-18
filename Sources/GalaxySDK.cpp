#include <sstream>
#include <DxImageProc.h>
#include <Cango/GalaxySDK.hpp>

namespace {
	void ConvertRaw8ToRGB24(const auto& raw8Image, cv::Mat& rgbImage) {
		rgbImage.create(raw8Image.nHeight, raw8Image.nWidth, CV_8UC3);
		(void)DxRaw8toRGB24Ex(
			raw8Image.pImgBuf,
			rgbImage.data,
			raw8Image.nWidth,
			raw8Image.nHeight,
			RAW2RGB_NEIGHBOUR,
			BAYERBG,
			false,
			DX_ORDER_RGB
		); //todo 检查这里的错误
	}
}

namespace Cango::GalaxySDK::inline Utils {
	bool GetErrorInfo(spdlog::logger& logger, GX_STATUS& error, std::size_t& size) noexcept {
		if (GXGetLastError(&error, nullptr, &size) == GX_STATUS_SUCCESS) return true;
		logger.error("无法获取最新错误的信息");
		return false;
	}

	bool GetErrorContent(
		spdlog::logger& logger,
		const GX_STATUS& error,
		const std::size_t& size,
		std::span<char> buffer) noexcept {
		GX_STATUS error_code{};
		if (
			GXGetLastError(
				&error_code,
				buffer.data(),
				const_cast<std::size_t*>(&size)
			) == GX_STATUS_SUCCESS) {
			if (error_code != error)
				logger.warn(
					"获取到最新的错误消息，但是错误代码不匹配({} != {})",
					static_cast<int>(error),
					error_code);
			return true;
		}
		logger.error("无法获取最新错误的内容");
		return false;
	}

	bool GetErrorMessage(spdlog::logger& logger, std::string& message) noexcept {
		GX_STATUS error_code{};
		std::size_t error_size{};
		if (!GetErrorInfo(logger, error_code, error_size) || error_size == 0) return false;

		std::vector<char> error_content(error_size);
		if (!GetErrorContent(logger, error_code, error_size, error_content)) return false;

		// make sure the message is null-terminated and copied from the unsafe memory
		message = std::ostringstream{std::string{error_content.data()}}.str();
		return true;
	}

	std::string GetErrorMessage(spdlog::logger& logger) noexcept {
		std::string message{};
		return GetErrorMessage(logger, message) ? message : "failed";
	}
}

namespace Cango::GalaxySDK::inline LibraryLifeTime {
	bool Initialize(spdlog::logger& logger) noexcept {
		if (GXInitLib() == GX_STATUS_SUCCESS) return true;
		logger.error("初始化时发生错误：{}", GetErrorMessage(logger));
		return false;
	}

	bool Close(spdlog::logger& logger) noexcept {
		if (GXCloseLib() == GX_STATUS_SUCCESS) return true;
		logger.error("关闭时发生错误：{}", GetErrorMessage(logger));
		return false;
	}

	void CloseWhatever(spdlog::logger& logger) noexcept { (void)Close(logger); }
}

namespace Cango::GalaxySDK:: inline QueryDevices {
	bool RefreshDeviceInfoList(spdlog::logger& logger, std::uint32_t& deviceCount) noexcept {
		static constexpr auto timeout_ms = 1000;

		if (GXUpdateDeviceList(&deviceCount, timeout_ms) != GX_STATUS_SUCCESS) {
			logger.error("无法更新设备列表：{}", GetErrorMessage(logger));
			return false;
		}

		if (deviceCount == 0) {
			logger.warn("没有找到任何设备");
			return false;
		}

		return true;
	}

	bool FillDeviceInfoList(
		spdlog::logger& logger,
		std::span<GX_DEVICE_BASE_INFO> list,
		std::size_t& filledCount) noexcept {
		filledCount = list.size();
		if (GXGetAllDeviceBaseInfo(list.data(), &filledCount)) return true;
		logger.error("无法获取设备基本信息：{}", GetErrorMessage(logger));
		return false;
	}

	bool GetDeviceInfoList(spdlog::logger& logger, std::vector<GX_DEVICE_BASE_INFO>& list) noexcept {
		std::uint32_t deviceCount{};
		if (!RefreshDeviceInfoList(logger, deviceCount)) return false;

		list.resize(deviceCount);
		std::size_t filledCount{};
		if (!FillDeviceInfoList(logger, list, filledCount)) return false;

		list.resize(filledCount);
		return true;
	}

	bool GetDeviceByOrder(spdlog::logger& logger, const std::uint32_t order, GX_DEV_HANDLE& deviceHandle) noexcept {
		std::uint32_t count{};
		if (!RefreshDeviceInfoList(logger, count)) return false;

		if (order > count) {
			logger.error("要打开的设备序号({})超过了当前的设备数量({})", order, count);
			return false;
		}

		if (GXOpenDeviceByIndex(order, &deviceHandle) == GX_STATUS_SUCCESS) return true;
		logger.error("无法打开设备({})：{}", order, GetErrorMessage(logger));
		return false;
	}

	bool GetDeviceByIdentifier(
		spdlog::logger& logger,
		const DeviceIdentifier& id,
		GX_DEV_HANDLE& deviceHandle) noexcept {
		if (std::uint32_t count{}; !RefreshDeviceInfoList(logger, count)) return false;
		auto param = static_cast<GX_OPEN_PARAM>(id);
		if (GXOpenDevice(&param, &deviceHandle) == GX_STATUS_SUCCESS) return true;
		logger.error("无法打开设备({})：{}", id.Content, GetErrorMessage(logger));
		return false;
	}
}

namespace Cango::GalaxySDK:: inline DeviceControls {
	bool CloseDevice(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept {
		if (GXCloseDevice(deviceHandle) == GX_STATUS_SUCCESS) return true;
		logger.error("无法关闭设备：{}", GetErrorMessage(logger));
		return false;
	}

	bool SendCommand(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle, const GX_FEATURE_ID& command) noexcept {
		if (GXSendCommand(deviceHandle, command) == GX_STATUS_SUCCESS) return true;
		logger.error("无法发送命令：{}", GetErrorMessage(logger));
		return false;
	}

	bool StartCapture(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept {
#ifndef _LINUX
		if (SendCommand(logger, deviceHandle, GX_COMMAND_ACQUISITION_START)) return true;
#else
		if (GXStreamOn(deviceHandle) == GX_STATUS_SUCCESS) return true;
#endif
		logger.error("无法开始采集：{}", GetErrorMessage(logger));
		return false;
	}

	bool StopCapture(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle) noexcept {
#ifndef _LINUX
		if (SendCommand(logger, deviceHandle, GX_COMMAND_ACQUISITION_STOP)) return true;
#else
		if (GXStreamOff(deviceHandle) == GX_STATUS_SUCCESS) return true;
#endif
		logger.error("无法停止采集：{}", GetErrorMessage(logger));
		return true;
	}

	bool GetImage(spdlog::logger& logger, const GX_DEV_HANDLE& deviceHandle, cv::Mat& image,
	              void* imageBuffer) noexcept {
#ifndef _LINUX
		GX_FRAME_DATA frame{.pImgBuf = imageBuffer};
		if (GXGetImage(deviceHandle, &frame, 100) != GX_STATUS_SUCCESS) {
			logger.error("无法获取图像：{}", GetErrorMessage(logger));
			return false;
		}
#else
		GX_FRAME_BUFFER* frame_buffer{nullptr};
		if (GXDQBuf(deviceHandle, &frame_buffer, 100) != GX_STATUS_SUCCESS) {
			logger.error("无法获取图像：{}", GetErrorMessage(logger));
			return false;
		}

		if (frame_buffer == nullptr) {
			logger.error("图像缓冲区为空");
			return false;
		}

		auto& frame = *frame_buffer;
#endif

		if (frame.pImgBuf == nullptr) {
			logger.error("图像数据为空");
			return false;
		}
		image.create(frame.nHeight, frame.nWidth, CV_8UC3);
		ConvertRaw8ToRGB24(frame, image);
#ifdef _LINUX
		if (GXQBuf(deviceHandle, frame_buffer) != GX_STATUS_SUCCESS) {
			logger.error("归还缓冲区失败：{}", GetErrorMessage(logger));
		}
#endif

		return true;
	}
}

namespace Cango::GalaxySDK {
	std::ostream& operator<<(std::ostream& stream, const DeviceIdentifier::IdentifierType type) noexcept {
		switch (type) {
		case DeviceIdentifier::SerialNumber:
			return stream << "SerialNumber";
		case DeviceIdentifier::IPAddress:
			return stream << "IPAddress";
		case DeviceIdentifier::MACAddress:
			return stream << "MACAddress";
		case DeviceIdentifier::DeviceIndex:
			return stream << "DeviceIndex";
		case DeviceIdentifier::UserDefinedID:
			return stream << "UserDefinedID";
		}
		return stream << "ErrorType";
	}

	std::ostream& DeviceIdentifier::Format(std::ostream& stream) const noexcept {
		return stream << Type << '(' << Content << ')';
	}

	DeviceIdentifier::operator GX_OPEN_PARAM() const noexcept {
		return {
			.pszContent = const_cast<char*>(Content.data()),
			.openMode = Type,
			.accessMode = GX_ACCESS_EXCLUSIVE
		};
	}

	// ReSharper disable once CppMemberFunctionMayBeConst
	bool GxCamera::GetItem(cv::Mat& image) noexcept {
#ifndef _LINUX
		return GetImage(*Logger, *DeviceHandle, image, ImageBuffer.data());
#else
		return GetImage(*Logger, *DeviceHandle, image);
#endif
	}

	GxCameraProvider::Configurations GxCameraProvider::Configure() noexcept {
		return Configurations{
			.Actors = {Logger, CameraLogger},
			.Options = {OpenParameter, ConfigureParameter}
		};
	}

	bool GxCameraProvider::IsFunctional() const noexcept { return Logger && CameraLogger; }

	bool GxCameraProvider::GetItem(Owner<GxCamera>& camera) noexcept {
		Owner<GxCamera> new_camera{};
		auto& handle = *new_camera->DeviceHandle;
		auto& logger = *Logger;
		if (!(OpenParameter.UseOrder
			      ? GetDeviceByOrder(logger, OpenParameter.Order, handle)
			      : GetDeviceByIdentifier(logger, OpenParameter.Identifier, handle)
		))
			return false;
		if (!ConfigureParameter.Apply(logger, handle)) return false;

#ifndef _LINUX
		Parameters::PayLoadSize payload{};
		if (!payload.Get(*Logger, handle)) return false;
		camera->ImageBuffer.reserve(payload.Value);
#endif

		if (!StartCapture(logger, handle)) return false;
		camera = std::move(new_camera);
		return true;
	}

	GxCameraCheatsheet::GxCameraCheatsheet() {
		auto&& provider_config = Provider->Configure();
		{
			const auto actors = provider_config.Actors;
			actors.Logger = spdlog::default_logger();
			actors.CameraLogger = spdlog::default_logger();
		}

		auto&& consumer_config = Consumer->Configure();
		{
			const auto actors = consumer_config.Actors;
			actors.Monitor = CaptureMonitor;
			actors.ItemDestination = ImagePool;
		}

		auto&& task_config = Task.Configure();
		{
			const auto actors = task_config.Actors;
			actors.Monitor = DeliveryMonitor;
			actors.ItemSource = Provider;
			actors.ItemDestination = Consumer;
		}
	}
}
