# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/cheng/faiz/distsys/distsys_sp24/MapReduce

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build

# Include any dependencies generated for this target.
include CMakeFiles/mapreduce_grpc_proto.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/mapreduce_grpc_proto.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/mapreduce_grpc_proto.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/mapreduce_grpc_proto.dir/flags.make

mapreduce.pb.cc: ../../src/proto/mapreduce.proto
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating mapreduce.pb.cc, mapreduce.pb.h, mapreduce.grpc.pb.cc, mapreduce.grpc.pb.h"
	/home/cheng/.local/bin/protoc-25.1.0 --grpc_out /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build --cpp_out /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build -I /home/cheng/faiz/distsys/distsys_sp24/MapReduce/src/proto --plugin=protoc-gen-grpc="/home/cheng/.local/bin/grpc_cpp_plugin" /home/cheng/faiz/distsys/distsys_sp24/MapReduce/src/proto/mapreduce.proto

mapreduce.pb.h: mapreduce.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate mapreduce.pb.h

mapreduce.grpc.pb.cc: mapreduce.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate mapreduce.grpc.pb.cc

mapreduce.grpc.pb.h: mapreduce.pb.cc
	@$(CMAKE_COMMAND) -E touch_nocreate mapreduce.grpc.pb.h

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o: CMakeFiles/mapreduce_grpc_proto.dir/flags.make
CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o: mapreduce.grpc.pb.cc
CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o: CMakeFiles/mapreduce_grpc_proto.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o -MF CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o.d -o CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o -c /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.grpc.pb.cc

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.grpc.pb.cc > CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.i

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.grpc.pb.cc -o CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.s

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o: CMakeFiles/mapreduce_grpc_proto.dir/flags.make
CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o: mapreduce.pb.cc
CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o: CMakeFiles/mapreduce_grpc_proto.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o -MF CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o.d -o CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o -c /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.pb.cc

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.pb.cc > CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.i

CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/mapreduce.pb.cc -o CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.s

# Object files for target mapreduce_grpc_proto
mapreduce_grpc_proto_OBJECTS = \
"CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o" \
"CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o"

# External object files for target mapreduce_grpc_proto
mapreduce_grpc_proto_EXTERNAL_OBJECTS =

libmapreduce_grpc_proto.a: CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.grpc.pb.cc.o
libmapreduce_grpc_proto.a: CMakeFiles/mapreduce_grpc_proto.dir/mapreduce.pb.cc.o
libmapreduce_grpc_proto.a: CMakeFiles/mapreduce_grpc_proto.dir/build.make
libmapreduce_grpc_proto.a: CMakeFiles/mapreduce_grpc_proto.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Linking CXX static library libmapreduce_grpc_proto.a"
	$(CMAKE_COMMAND) -P CMakeFiles/mapreduce_grpc_proto.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mapreduce_grpc_proto.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/mapreduce_grpc_proto.dir/build: libmapreduce_grpc_proto.a
.PHONY : CMakeFiles/mapreduce_grpc_proto.dir/build

CMakeFiles/mapreduce_grpc_proto.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/mapreduce_grpc_proto.dir/cmake_clean.cmake
.PHONY : CMakeFiles/mapreduce_grpc_proto.dir/clean

CMakeFiles/mapreduce_grpc_proto.dir/depend: mapreduce.grpc.pb.cc
CMakeFiles/mapreduce_grpc_proto.dir/depend: mapreduce.grpc.pb.h
CMakeFiles/mapreduce_grpc_proto.dir/depend: mapreduce.pb.cc
CMakeFiles/mapreduce_grpc_proto.dir/depend: mapreduce.pb.h
	cd /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/cheng/faiz/distsys/distsys_sp24/MapReduce /home/cheng/faiz/distsys/distsys_sp24/MapReduce /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build /home/cheng/faiz/distsys/distsys_sp24/MapReduce/cmake/build/CMakeFiles/mapreduce_grpc_proto.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/mapreduce_grpc_proto.dir/depend

