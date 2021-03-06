cmake_minimum_required(VERSION 3.1)
project(iscore_plugin_curve LANGUAGES CXX)

iscore_common_setup()

# Packages
find_package(Qt5 5.3 REQUIRED COMPONENTS Core Widgets)

# Files & main target
file(GLOB_RECURSE HDRS
    "${CMAKE_CURRENT_SOURCE_DIR}/Curve/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CurvePlugin.hpp"
)
set(SRCS
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/CurveSegmentModelSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Linear/LinearSegmentSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Power/PowerSegmentSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Sin/SinSegmentSerialization.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/PointArray/PointArraySegmentSerialization.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/CurveModelSerialization.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Process/CurveProcessModel.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Commands/SetSegmentParameters.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Commands/UpdateCurve.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Commands/CurveCommandFactory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Commands/MovePoint.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Point/CurvePointModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Point/CurvePointView.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/CurveSegmentModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/CurveSegmentFactory.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/CurveSegmentView.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Linear/LinearSegment.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Power/PowerSegment.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/Sin/SinSegment.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Segment/PointArray/PointArraySegment.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CommandObjects/MoveSegmentCommandObject.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CommandObjects/CreatePointCommandObject.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CommandObjects/MovePointCommandObject.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CommandObjects/CurveCommandObjectBase.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CommandObjects/SetSegmentParametersCommandObject.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/Tools/CurveTool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/Tools/MoveTool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/Tools/SmartTool.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CurvePalette.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Palette/CurveEditionSettings.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Settings/CurveSettingsModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Settings/CurveSettingsPresenter.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/Settings/CurveSettingsView.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/Curve/CurveModel.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/CurvePresenter.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Curve/CurveView.cpp"

"${CMAKE_CURRENT_SOURCE_DIR}/iscore_plugin_curve.cpp"
)

add_library(${PROJECT_NAME} ${SRCS} ${HDRS})
iscore_generate_command_list_file(${PROJECT_NAME} "${HDRS}")
target_link_libraries(${PROJECT_NAME} PUBLIC
                     Qt5::Core Qt5::Widgets
                     iscore_lib_base iscore_lib_process iscore_lib_state)

setup_iscore_plugin(${PROJECT_NAME})
