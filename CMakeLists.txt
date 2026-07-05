cmake_minimum_required(VERSION 3.20)

project(DV_Volume_Flow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    IOImage
    IOLegacy
    FiltersCore
    FiltersGeneral
    FiltersSources
    FiltersFlowPaths
    FiltersModeling
    ImagingCore
    RenderingCore
    RenderingOpenGL2
    RenderingVolume
    RenderingVolumeOpenGL2
    RenderingAnnotation
    InteractionStyle
    InteractionWidgets
)

add_executable(Isosurface
    "Volumetric Dataset/Isosurface.cpp"
)

add_executable(RayMarching
    "Volumetric Dataset/RayMarching.cpp"
)

add_executable(FlowVis
    "Flow Dataset/flowVisSample.cpp"
)

add_executable(CombinedVolume
    "Volumetric Dataset/CombinedVolume.cpp"
)

target_link_libraries(Isosurface PRIVATE ${VTK_LIBRARIES})
target_link_libraries(RayMarching PRIVATE ${VTK_LIBRARIES})
target_link_libraries(FlowVis PRIVATE ${VTK_LIBRARIES})
target_link_libraries(CombinedVolume PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
    TARGETS Isosurface RayMarching FlowVis CombinedVolume
    MODULES ${VTK_LIBRARIES}
)