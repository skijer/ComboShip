# Standard and HD Wolf models are bundled in each game's assets/custom tree and
# included automatically by GenerateSohOtr/Generate2ShipOtr. Retain this explicit
# loose-file deployment option for older port archives and development fixtures.
set(NEI_WOLF_LINK_ASSET "" CACHE FILEPATH "Path to the real exported NEI wolf_link.bin to deploy and package")
if(NEI_WOLF_LINK_ASSET)
    if(NOT EXISTS "${NEI_WOLF_LINK_ASSET}" OR IS_DIRECTORY "${NEI_WOLF_LINK_ASSET}")
        message(FATAL_ERROR "NEI_WOLF_LINK_ASSET must name an existing wolf_link.bin file")
    endif()
    get_filename_component(NEI_WOLF_LINK_ASSET "${NEI_WOLF_LINK_ASSET}" ABSOLUTE)
    file(SIZE "${NEI_WOLF_LINK_ASSET}" _wolf_size)
    file(READ "${NEI_WOLF_LINK_ASSET}" _wolf_header OFFSET 0 LIMIT 12 HEX)
    # Magic NEIWOLF1 followed by little-endian version 1 or 2. The runtime checks the
    # complete layout, rig, weights, transforms and required animation clips.
    if(_wolf_size LESS 88 OR _wolf_size GREATER 67108864 OR
       NOT _wolf_header MATCHES "^4e4549574f4c46310[12]000000$" OR
       (_wolf_header STREQUAL "4e4549574f4c463102000000" AND _wolf_size LESS 92))
        message(FATAL_ERROR "NEI_WOLF_LINK_ASSET has an invalid NEIWOLF1/version-1-or-2 header or file size")
    endif()
    file(SHA256 "${NEI_WOLF_LINK_ASSET}" _wolf_sha256)
    message(STATUS "ComboShip Wolf asset: ${NEI_WOLF_LINK_ASSET} (${_wolf_size} bytes, SHA256 ${_wolf_sha256})")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${NEI_WOLF_LINK_ASSET}")
    # A target runs even when the executable does not relink, so replacing a
    # configured asset updates the development runtime too.
    add_custom_target(CopyNeiWolfAsset
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:ComboShip>/nei/soh" "$<TARGET_FILE_DIR:ComboShip>/nei/2ship"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NEI_WOLF_LINK_ASSET}" "$<TARGET_FILE_DIR:ComboShip>/nei/soh/wolf_link.bin"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NEI_WOLF_LINK_ASSET}" "$<TARGET_FILE_DIR:ComboShip>/nei/2ship/wolf_link.bin"
        VERBATIM)
    add_dependencies(ComboShip CopyNeiWolfAsset)
    install(FILES "${NEI_WOLF_LINK_ASSET}" DESTINATION nei/soh RENAME wolf_link.bin COMPONENT combo)
    install(FILES "${NEI_WOLF_LINK_ASSET}" DESTINATION nei/2ship RENAME wolf_link.bin COMPONENT combo)
else()
    message(STATUS "ComboShip Wolf models are bundled in soh.o2r/2ship.o2r; no loose Wolf asset configured")
endif()
