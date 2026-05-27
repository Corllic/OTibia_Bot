cmake_minimum_required(VERSION 3.16)
project(EasyBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.10.0/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent)

set(SOURCES
    main.cpp
    src/Core/Addresses.cpp
    src/Core/Logger.cpp
    src/Functions/Memory.cpp
    src/General/SelectTibiaTab.cpp
    src/General/MainWindowTab.cpp
    src/Healing/HealingTab.cpp
    src/Spell/SpellTab.cpp
    src/Walker/WalkerTab.cpp
    src/Target/TargetTab.cpp
    src/Settings/SettingsTab.cpp
    src/Status/StatusTab.cpp
    src/Creature/CreatureScanner.cpp
)

if(WIN32)
    set(APP_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/Icon.rc")
    if(EXISTS "${APP_ICON_RC}")
        list(APPEND SOURCES "${APP_ICON_RC}")
    endif()
endif()

add_executable(EasyBot ${SOURCES})

target_include_directories(EasyBot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(EasyBot PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Concurrent
)

if(WIN32)
    target_link_libraries(EasyBot PRIVATE psapi)
endif()
