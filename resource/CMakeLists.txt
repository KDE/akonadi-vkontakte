
if (XSLTPROC_EXECUTABLE)
  macro( kcfg_generate_dbus_interface _kcfg _name )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
              ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
              > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
    )
  endmacro( kcfg_generate_dbus_interface )
endif (XSLTPROC_EXECUTABLE)

# Turn this off by default when KDEPIM 4.4 is not in use anymore
option(KDEPIM_44_COMPAT "Don't crash when using KDEPIM runtime 4.4.x, at the cost of using deprecated KCal API" TRUE)
if(KDEPIM_44_COMPAT)
  add_definitions(-DWANT_DEPRECATED_KCAL_API)
endif(KDEPIM_44_COMPAT)

find_package(KdepimLibs 4.5.63 REQUIRED)

configure_file(config.h.cmake "${CMAKE_CURRENT_BINARY_DIR}/config.h")

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( vkontakteresource_SRCS
  vkontakteresource.cpp
  vkontakteresource_events.cpp
  vkontakteresource_friends.cpp
  vkontakteresource_notes.cpp
  vkontakteresource_messages.cpp
  settings.cpp
  settingsdialog.cpp
)

install( FILES vkontakteresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files( vkontakteresource_SRCS settingsdialog.ui)
kde4_add_kcfg_files(vkontakteresource_SRCS settingsbase.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/settingsbase.kcfg org.kde.Akonadi.Vkontakte.Settings)
qt4_add_dbus_adaptor(vkontakteresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Vkontakte.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_vkontakte_resource RUN_UNINSTALLED ${vkontakteresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_vkontakte_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.template)
  set_target_properties(akonadi_vkontakte_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.Vkontakte")
  set_target_properties(akonadi_vkontakte_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi VKontakte Resource")
endif (Q_WS_MAC)

include_directories(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  ${KDE4_INCLUDES}
  ${QJSON_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${KDEPIMLIBS_INCLUDE_DIRS}
  ${LIBKVKONTAKTE_INCLUDES}
)

target_link_libraries( akonadi_vkontakte_resource
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${LIBKVKONTAKTE_LIBRARIES}
)

if(KDEPIM_44_COMPAT)
  target_link_libraries(akonadi_vkontakte_resource ${KDEPIMLIBS_KCAL_LIBS})
else(KDEPIM_44_COMPAT)
  target_link_libraries(akonadi_vkontakte_resource ${KDEPIMLIBS_KCALCORE_LIBS})
endif(KDEPIM_44_COMPAT)

add_subdirectory(icons)

install(TARGETS akonadi_vkontakte_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
