cmake_minimum_required( VERSION 3.3 )
project( meshview_example )

find_package( meshview REQUIRED )
add_executable( example ../example.cpp )
target_link_libraries( example meshview::meshview )
