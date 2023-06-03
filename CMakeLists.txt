cmake_minimum_required(VERSION 3.26)

project(Avestan)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/debug)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/usr DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/var DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/opt DESTINATION ${PROJECT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/usr/callback.py DESTINATION ${PROJECT_BINARY_DIR}/bin/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/usr/callback.py DESTINATION ${PROJECT_BINARY_DIR}/debug/)


add_library(server SHARED
  ${SOURCE_DIR}/server/AvestaDialog.cpp
  ${SOURCE_DIR}/server/callback.cpp
  ${SOURCE_DIR}/server/ExecuteCommand.cpp
  ${SOURCE_DIR}/server/FolderList.cpp
  ${SOURCE_DIR}/server/FolderTree.cpp
  ${SOURCE_DIR}/server/Globals.cpp
  ${SOURCE_DIR}/server/main.cpp
  ${SOURCE_DIR}/server/NavigateCommand.cpp
  ${SOURCE_DIR}/server/RenameDialog.cpp
  ${SOURCE_DIR}/server/server.rc
  ${SOURCE_DIR}/server/server.vcproj
  ${SOURCE_DIR}/server/server.vcxproj
  ${SOURCE_DIR}/server/server.vcxproj.filters
  ${SOURCE_DIR}/server/ShellStorage.cpp
  ${SOURCE_DIR}/server/stdafx.cpp
  ${SOURCE_DIR}/server/utils.cpp
  ${SOURCE_DIR}/server/XmlForm.cpp
  ${SOURCE_DIR}/server/XmlResource.cpp

  ${SOURCE_DIR}/mew/afx.cpp
  ${SOURCE_DIR}/mew/command.cpp
  ${SOURCE_DIR}/mew/debug.cpp
  ${SOURCE_DIR}/mew/dialogs.cpp
  ${SOURCE_DIR}/mew/DragDrop.cpp
  ${SOURCE_DIR}/mew/Entry.cpp
  ${SOURCE_DIR}/mew/expose.cpp
  ${SOURCE_DIR}/mew/Folder.cpp
  ${SOURCE_DIR}/mew/gesture.cpp
  ${SOURCE_DIR}/mew/imagelist.cpp
  ${SOURCE_DIR}/mew/io.cpp
  ${SOURCE_DIR}/mew/keymap.cpp
  ${SOURCE_DIR}/mew/message.cpp
  ${SOURCE_DIR}/mew/messenger.cpp
  ${SOURCE_DIR}/mew/registrar.cpp
  ${SOURCE_DIR}/mew/string.cpp
  ${SOURCE_DIR}/mew/variant.cpp
  ${SOURCE_DIR}/mew/version.cpp

  ${SOURCE_DIR}/mew/widgets/DefaultTreeItem.cpp
  ${SOURCE_DIR}/mew/widgets/Display.cpp
  ${SOURCE_DIR}/mew/widgets/Form.cpp
  ${SOURCE_DIR}/mew/widgets/MenuProvider.cpp
  ${SOURCE_DIR}/mew/widgets/Preview.cpp
  ${SOURCE_DIR}/mew/widgets/ReBar.cpp
  ${SOURCE_DIR}/mew/widgets/ShellFolder.cpp
  ${SOURCE_DIR}/mew/widgets/ShellListView.cpp
  ${SOURCE_DIR}/mew/widgets/StatusBar.cpp
  ${SOURCE_DIR}/mew/widgets/Tab.cpp
  ${SOURCE_DIR}/mew/widgets/ToolBar.cpp
  ${SOURCE_DIR}/mew/widgets/TreeView.cpp
  ${SOURCE_DIR}/mew/widgets/WallPaperDialog.cpp
  ${SOURCE_DIR}/mew/widgets/WindowImpl.cpp

  ${SOURCE_DIR}/avesta/commandline.cpp
  ${SOURCE_DIR}/avesta/dialog.cpp
  ${SOURCE_DIR}/avesta/dialog_name.cpp
  ${SOURCE_DIR}/avesta/dialog_path.cpp
  ${SOURCE_DIR}/avesta/file.cpp
  ${SOURCE_DIR}/avesta/fileop.cpp
  ${SOURCE_DIR}/avesta/itemid.cpp
  ${SOURCE_DIR}/avesta/registry.cpp
  ${SOURCE_DIR}/avesta/theme.cpp
  ${SOURCE_DIR}/avesta/thread.cpp
  ${SOURCE_DIR}/avesta/window.cpp
  ${SOURCE_DIR}/avesta/xmlcmd.cpp
  ${SOURCE_DIR}/avesta/xmlmessage.cpp
  ${SOURCE_DIR}/avesta/xmlparse.cpp
  ${SOURCE_DIR}/avesta/xmlreader.cpp
  ${SOURCE_DIR}/avesta/xmlwriter.cpp
  )
target_include_directories(server PRIVATE
  ${SOURCE_DIR}/server
  ${SOURCE_DIR}/include
  ${SOURCE_DIR}/WTL91_5321_Final/include
  )
target_link_libraries(server PRIVATE
  Gdiplus.lib
  )
target_compile_definitions(server PRIVATE
  -D_UNICODE
  )
set_target_properties(server PROPERTIES OUTPUT_NAME "avesta")


add_library(pygmy SHARED
  ${SOURCE_DIR}/pygmy/main.cpp
  ${SOURCE_DIR}/pygmy/pygmy.def
  ${SOURCE_DIR}/pygmy/pygmy.hpp
  ${SOURCE_DIR}/pygmy/stdafx.cpp
  ${SOURCE_DIR}/pygmy/stdafx.h
  )
target_include_directories(pygmy PRIVATE
  ${SOURCE_DIR}/include
  )
target_link_libraries(pygmy PRIVATE
  server
  )
target_compile_definitions(pygmy PRIVATE
  -D_UNICODE
  )
  
find_package (Python COMPONENTS Development)
if(Python_FOUND)
  target_include_directories(pygmy PRIVATE
    ${Python_INCLUDE_DIRS}
    )
  target_link_libraries(pygmy PRIVATE
    ${Python_LIBRARIES}
    )
endif()

set_target_properties(pygmy PROPERTIES OUTPUT_NAME "pygmy"
                                       SUFFIX ".pyd")


add_executable(client WIN32
  ${SOURCE_DIR}/client/client.cpp
  ${SOURCE_DIR}/client/resource.h
  ${SOURCE_DIR}/client/client.rc
  ${SOURCE_DIR}/client/main.ico
  ${SOURCE_DIR}/client/avesta.exe.manifest
  )
target_compile_definitions(client PRIVATE
  -D_UNICODE
  )

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT client)
add_dependencies(client server)
add_dependencies(pygmy server)
if(Python_FOUND)
  add_dependencies(client pygmy)
endif()


add_library(clipboard SHARED
  ${SOURCE_DIR}/tools-clipboard/main.cpp
  ${SOURCE_DIR}/tools-clipboard/stdafx.cpp
  ${SOURCE_DIR}/tools-clipboard/clipboard.def
  ${SOURCE_DIR}/tools-clipboard/stdafx.h
  )
target_include_directories(clipboard PRIVATE
  ${SOURCE_DIR}/pygmy
  ${SOURCE_DIR}/include
  )
if(Python_FOUND)
  target_include_directories(clipboard PRIVATE
    ${Python_INCLUDE_DIRS}
    )
  target_link_libraries(clipboard PRIVATE
    ${Python_LIBRARIES}
    )
endif()
set_target_properties(clipboard
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/opt
)