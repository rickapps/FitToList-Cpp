# Produces an unsigned FitToList.msix wrapping the FitToList.exe build as a
# Desktop Bridge (Win32-in-MSIX) package. Not part of the default build - run
# `cmake --build <dir> --target msix` explicitly. Sideloading the result
# requires signing it first; see CLAUDE.md for the self-signed dev-certificate
# steps (a machine cert-store change CMake deliberately doesn't do for you).

set(MSIX_PACKAGE_NAME "FitToList" CACHE STRING "MSIX package Identity/Name")
set(MSIX_PUBLISHER "CN=Rick Eichhorn" CACHE STRING
    "MSIX package Identity/Publisher - must exactly match the signing certificate's Subject")
set(MSIX_PUBLISHER_DISPLAY_NAME "Rick Eichhorn" CACHE STRING "MSIX package PublisherDisplayName")
set(MSIX_VERSION "${PROJECT_VERSION}.0")
set(MSIX_MAX_TESTED_WINDOWS_VERSION "${CMAKE_SYSTEM_VERSION}.0")

set(MSIX_STAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/staging")

configure_file(
    AppxManifest.xml.in
    "${MSIX_STAGING_DIR}/AppxManifest.xml"
    @ONLY
)

qt_add_executable(GenerateMsixAssets EXCLUDE_FROM_ALL GenerateAppIcon.cpp)
target_link_libraries(GenerateMsixAssets PRIVATE Qt6::Gui)

# windeployqt lives next to Qt itself; makeappx/signtool ship with the
# Windows SDK, which is on PATH in a Developer Command Prompt but not
# necessarily elsewhere, so widen the search to every installed SDK version.
get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)

find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
    HINTS "${QT_BIN_DIR}")

file(GLOB _msix_sdk_bin_dirs
    "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.*/x64"
    "C:/Program Files (x86)/Windows Kits/10/bin/10.0.*/x64")
find_program(MAKEAPPX_EXECUTABLE NAMES makeappx HINTS ${_msix_sdk_bin_dirs})

if(NOT WINDEPLOYQT_EXECUTABLE)
    message(WARNING "windeployqt not found - the msix target will fail; pass -DWINDEPLOYQT_EXECUTABLE=...")
endif()
if(NOT MAKEAPPX_EXECUTABLE)
    message(WARNING "makeappx not found - the msix target will fail; pass -DMAKEAPPX_EXECUTABLE=..., "
                     "or build from a Developer Command Prompt where it's already on PATH")
endif()

add_custom_command(
    OUTPUT "${MSIX_STAGING_DIR}/Assets/Square44x44Logo.png"
           "${MSIX_STAGING_DIR}/Assets/Square150x150Logo.png"
           "${MSIX_STAGING_DIR}/Assets/StoreLogo.png"
    COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
        "$<TARGET_FILE:GenerateMsixAssets>" "${MSIX_STAGING_DIR}/Assets"
    DEPENDS GenerateMsixAssets
    COMMENT "Generating MSIX logo assets"
)

add_custom_command(
    OUTPUT "${MSIX_STAGING_DIR}/FitToList.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:FitToList>" "${MSIX_STAGING_DIR}/FitToList.exe"
    COMMAND ${CMAKE_COMMAND} -E env "PATH=${QT_BIN_DIR};$ENV{PATH}"
        "${WINDEPLOYQT_EXECUTABLE}" --dir "${MSIX_STAGING_DIR}" "${MSIX_STAGING_DIR}/FitToList.exe"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/user_manual.html" "${MSIX_STAGING_DIR}/user_manual.html"
    DEPENDS FitToList
    COMMENT "Staging FitToList.exe + Qt runtime for MSIX packaging"
)

set(MSIX_OUTPUT "${CMAKE_BINARY_DIR}/FitToList.msix")

add_custom_command(
    OUTPUT "${MSIX_OUTPUT}"
    COMMAND "${MAKEAPPX_EXECUTABLE}" pack /d "${MSIX_STAGING_DIR}" /p "${MSIX_OUTPUT}" /o
    DEPENDS
        "${MSIX_STAGING_DIR}/AppxManifest.xml"
        "${MSIX_STAGING_DIR}/FitToList.exe"
        "${MSIX_STAGING_DIR}/Assets/Square44x44Logo.png"
        "${MSIX_STAGING_DIR}/Assets/Square150x150Logo.png"
        "${MSIX_STAGING_DIR}/Assets/StoreLogo.png"
    COMMENT "Packing FitToList.msix (unsigned - see CLAUDE.md to sign it for sideloading)"
)

add_custom_target(msix DEPENDS "${MSIX_OUTPUT}")
