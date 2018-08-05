cmake_minimum_required(VERSION 3.8)
project(hw3)

set(CMAKE_CXX_STANDARD 11)

#set(SOURCE_FILES
#        receiver.c
#        sender.c)

# Force using C++ compiler
set_source_files_properties(sender.c PROPERTIES LANGUAGE CXX )
set_source_files_properties(receiver.c PROPERTIES LANGUAGE CXX )

add_executable(sender sender.c)
add_executable(receiver receiver.c)
