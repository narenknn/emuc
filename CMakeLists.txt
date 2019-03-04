cmake_minimum_required (VERSION 3.10.0)
project (EMUC)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library (emucClient STATIC src/client.cc)
target_include_directories (emucClient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include $ENV{BOOST_HOME}/include $ENV{VCS_HOME}/include)

add_library (emucServer STATIC src/server.cc)
target_include_directories (emucServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include $ENV{BOOST_HOME}/include $ENV{VCS_HOME}/include)

add_executable (client tests/client_hw.cc)
add_executable (server tests/server_hw.cc)

target_link_libraries (client LINK_PUBLIC emucClient $ENV{BOOST_HOME}/lib/libboost_filesystem.a $ENV{BOOST_HOME}/lib/libboost_system.a $ENV{BOOST_HOME}/lib/libboost_thread.a pthread rt)
target_link_libraries (server LINK_PUBLIC emucServer $ENV{BOOST_HOME}/lib/libboost_filesystem.a $ENV{BOOST_HOME}/lib/libboost_system.a $ENV{BOOST_HOME}/lib/libboost_thread.a pthread rt)
#target_link_libraries (client LINK_PUBLIC emucClient -lboost_filesystem -lboost_system -lboost_thread -lpthread -lrt)
#target_link_libraries (server LINK_PUBLIC emucServer -lboost_filesystem -lboost_system -lboost_thread -lpthread -lrt)
