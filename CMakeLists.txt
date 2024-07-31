project(GalaxySDK)

if(CMAKE_HOST_UNIX)
	add_library(GxIAPI SHARED IMPORTED GLOBAL)
	set_property(TARGET GxIAPI PROPERTY IMPORTED_SONAME libgxiapi.so)
	set_property(TARGET GxIAPI PROPERTY IMPORTED_LOCATION "/usr/lib/libgxiapi.so")
	set(GalaxySDK_Headers ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Linux_x64/inc)
	set(GalaxySDK_LIBRARIES GxIAPI)
elseif(CMAKE_HOST_WIN32)
	add_library(GxIAPI STATIC IMPORTED GLOBAL)
	set_property(TARGET GxIAPI PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows_x64/lib/GxIAPI.lib)
	set_property(TARGET GxIAPI PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows_x64/lib/GxIAPI.lib)
	add_library(DxImageProc STATIC IMPORTED GLOBAL)
	set_property(TARGET DxImageProc PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows_x64/lib/DxImageProc.lib)
	set_property(TARGET DxImageProc PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows_x64/lib/DxImageProc.lib)
	set(GalaxySDK_Headers ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Linux_x64/inc ${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows_x64/inc)
	set(GalaxySDK_LIBRARIES GxIAPI DxImageProc)
else()
	message(FATAL_ERROR "不受支持的平台")
endif()

AddCXXModule(
	NAME "GalaxySDK"
	NAMESPACE "Cango"
	LINKS
		${GalaxySDK_LIBRARIES}
		fmt::fmt
		spdlog::spdlog
		${OpenCV_LIBRARIES}
		Cango::TaskDesign
		Cango::CommonUtils
)
target_include_directories(Cango_GalaxySDK BEFORE PUBLIC ${GalaxySDK_Headers})

