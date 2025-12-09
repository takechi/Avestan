cmake_minimum_required(VERSION 4.2)

project(Avestan)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
  ${SOURCE_DIR}/server/AvestaSDK.hpp
  ${SOURCE_DIR}/server/callback.cpp
  ${SOURCE_DIR}/server/command.hpp
  ${SOURCE_DIR}/server/CommandHandler.hpp
  ${SOURCE_DIR}/server/CommandList.hpp
  ${SOURCE_DIR}/server/CommandProvider.hpp
  ${SOURCE_DIR}/server/ExecuteCommand.cpp
  ${SOURCE_DIR}/server/FolderList.cpp
  ${SOURCE_DIR}/server/FolderList.hpp
  ${SOURCE_DIR}/server/FolderTree.cpp
  ${SOURCE_DIR}/server/Globals.cpp
  ${SOURCE_DIR}/server/main.cpp
  ${SOURCE_DIR}/server/main.hpp
  ${SOURCE_DIR}/server/NavigateCommand.cpp
  ${SOURCE_DIR}/server/RenameDialog.cpp
  ${SOURCE_DIR}/server/resource.h
  ${SOURCE_DIR}/server/server.rc
  ${SOURCE_DIR}/server/server.vcproj
  ${SOURCE_DIR}/server/server.vcxproj
  ${SOURCE_DIR}/server/server.vcxproj.filters
  ${SOURCE_DIR}/server/ShellStorage.cpp
  ${SOURCE_DIR}/server/stdafx.cpp
  ${SOURCE_DIR}/server/stdafx.h
  ${SOURCE_DIR}/server/TaskTrayProvider.hpp
  ${SOURCE_DIR}/server/utils.cpp
  ${SOURCE_DIR}/server/utils.hpp
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
  ${SOURCE_DIR}/mew/private.h
  ${SOURCE_DIR}/mew/registrar.cpp
  ${SOURCE_DIR}/mew/string.cpp
  ${SOURCE_DIR}/mew/variant.cpp
  ${SOURCE_DIR}/mew/version.cpp

  ${SOURCE_DIR}/mew/widgets/DefaultTreeItem.cpp
  ${SOURCE_DIR}/mew/widgets/Display.cpp
  ${SOURCE_DIR}/mew/widgets/Form.cpp
  ${SOURCE_DIR}/mew/widgets/MenuProvider.cpp
  ${SOURCE_DIR}/mew/widgets/MenuProvider.hpp
  ${SOURCE_DIR}/mew/widgets/Preview.cpp
  ${SOURCE_DIR}/mew/widgets/ReBar.cpp
  ${SOURCE_DIR}/mew/widgets/ShellFolder.cpp
  ${SOURCE_DIR}/mew/widgets/ShellFolder.h
  ${SOURCE_DIR}/mew/widgets/ShellListView.cpp
  ${SOURCE_DIR}/mew/widgets/ShellNotify.h
  ${SOURCE_DIR}/mew/widgets/StatusBar.cpp
  ${SOURCE_DIR}/mew/widgets/Tab.cpp
  ${SOURCE_DIR}/mew/widgets/ToolBar.cpp
  ${SOURCE_DIR}/mew/widgets/TreeView.cpp
  ${SOURCE_DIR}/mew/widgets/WallPaperDialog.cpp
  ${SOURCE_DIR}/mew/widgets/WindowExtension.h
  ${SOURCE_DIR}/mew/widgets/WindowImpl.cpp
  ${SOURCE_DIR}/mew/widgets/WindowImpl.h
  ${SOURCE_DIR}/mew/widgets/WindowMessage.h

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

  ${SOURCE_DIR}/include/afx.hpp
  ${SOURCE_DIR}/include/application.hpp
  ${SOURCE_DIR}/include/avesta.hpp
  ${SOURCE_DIR}/include/basic.hpp
  ${SOURCE_DIR}/include/drawing.hpp
  ${SOURCE_DIR}/include/error.hpp
  ${SOURCE_DIR}/include/io.hpp
  ${SOURCE_DIR}/include/math.hpp
  ${SOURCE_DIR}/include/message.hpp
  ${SOURCE_DIR}/include/meta.hpp
  ${SOURCE_DIR}/include/mew.hpp
  ${SOURCE_DIR}/include/object.hpp
  ${SOURCE_DIR}/include/path.hpp
  ${SOURCE_DIR}/include/preprocessor.hpp
  ${SOURCE_DIR}/include/reference.hpp
  ${SOURCE_DIR}/include/shell.hpp
  ${SOURCE_DIR}/include/signal.hpp
  ${SOURCE_DIR}/include/storage.hpp
  ${SOURCE_DIR}/include/string.hpp
  ${SOURCE_DIR}/include/struct.hpp
  ${SOURCE_DIR}/include/theme.hpp
  ${SOURCE_DIR}/include/thread.hpp
  ${SOURCE_DIR}/include/widgets.client.hpp
  ${SOURCE_DIR}/include/widgets.hpp
  ${SOURCE_DIR}/include/xml.hpp

  ${SOURCE_DIR}/include/detail/headers.hpp
  ${SOURCE_DIR}/include/detail/objectimpl.hpp
  ${SOURCE_DIR}/include/detail/uuids.hpp

  ${SOURCE_DIR}/include/impl/InitUtil.hpp
  ${SOURCE_DIR}/include/impl/WTLControls.hpp
  ${SOURCE_DIR}/include/impl/WTLSplitter.hpp

  ${SOURCE_DIR}/include/std/algorithm.hpp
  ${SOURCE_DIR}/include/std/array_map.hpp
  ${SOURCE_DIR}/include/std/array_set.hpp
  ${SOURCE_DIR}/include/std/buffer.hpp
  ${SOURCE_DIR}/include/std/deque.hpp
  ${SOURCE_DIR}/include/std/hash_map.hpp
  ${SOURCE_DIR}/include/std/list.hpp
  ${SOURCE_DIR}/include/std/map.hpp
  ${SOURCE_DIR}/include/std/sequence.hpp
  ${SOURCE_DIR}/include/std/set.hpp
  ${SOURCE_DIR}/include/std/str.hpp
  ${SOURCE_DIR}/include/std/vector.hpp
  )
target_include_directories(server PRIVATE
  ${SOURCE_DIR}/server
  ${SOURCE_DIR}/include
  ${SOURCE_DIR}/WTL10_10320_Release/include
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
  
find_package (Python 3.11 COMPONENTS Development)
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