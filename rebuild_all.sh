 mkdir -p build
 pushd build
 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
 make -j 16
 popd
 
 mkdir -p build_dbg
 pushd build_dbg
 cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
 make -j 16
 popd
 ln -s build_dbg/compile_commands.json compile_commands.json
 
 mkdir -p build_asan
 pushd build_asan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" -DCMAKE_C_FLAGS="-fsanitize=address -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=address" ..
 make -j 16
 popd
 
 mkdir -p build_tsan
 pushd build_tsan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" -DCMAKE_C_FLAGS="-fsanitize=thread -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=thread" ..
 make -j 16
 popd
 
 mkdir -p build_ubsan
 pushd build_ubsan
 cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" -DCMAKE_C_FLAGS="-fsanitize=undefined -g" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=undefined" -DCMAKE_MODULE_LINKER_FLAGS="-fsanitize=undefined" ..
 make -j 16
 popd

