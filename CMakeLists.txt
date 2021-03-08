cmake_minimum_required(VERSION 3.17)
project(boostServerspecific)

set(CMAKE_CXX_STANDARD 17)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system thread regex)
FIND_PACKAGE(Boost)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})

add_executable(boostServerspecific main.cpp chat_message.hpp)
TARGET_LINK_LIBRARIES(boostServerspecific ${USED_LIBS} Boost::boost)