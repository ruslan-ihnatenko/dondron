cmake_minimum_required(VERSION 3.8)
project(dondron_state_machine)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)

find_package(behaviortree_cpp QUIET)
if(NOT behaviortree_cpp_FOUND)
  include(FetchContent)
  fetchcontent_declare(
    behaviortree_cpp
    GIT_REPOSITORY https://github.com/BehaviorTree/BehaviorTree.CPP.git
    GIT_TAG 4.6.2
  )
  set(BTCPP_SHARED_LIBS ON CACHE BOOL "" FORCE)
  set(BTCPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(BTCPP_GROOT_INTERFACE OFF CACHE BOOL "" FORCE)
  fetchcontent_makeavailable(behaviortree_cpp)
  set(BEHAVIORTREE_CPP_TARGET BT::behaviortree_cpp)
else()
  set(BEHAVIORTREE_CPP_TARGET behaviortree_cpp::behaviortree_cpp)
endif()

add_library(bt_nodes SHARED src/bt_nodes.cpp)
target_include_directories(bt_nodes PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(bt_nodes
  rclcpp
  geometry_msgs
  vision_msgs
  px4_msgs
  std_msgs
)
target_link_libraries(bt_nodes ${BEHAVIORTREE_CPP_TARGET})

add_executable(state_machine_node src/state_machine_node.cpp)
target_include_directories(state_machine_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(state_machine_node bt_nodes ${BEHAVIORTREE_CPP_TARGET})
ament_target_dependencies(state_machine_node
  rclcpp
  ament_index_cpp
)

install(TARGETS bt_nodes state_machine_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY behavior_trees launch
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_altitude_control test/test_altitude_control.cpp)
  target_include_directories(test_altitude_control PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
