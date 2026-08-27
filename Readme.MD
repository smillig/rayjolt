To get working with this repo first get the submodule dependancies:

git submodule update --init

Then config cmake build:
cmake -S . -B Build

Then Build:
cmake --build build --config Debug

For shaders to work properly the built exe must be moved to the project root directory or the Asssets forlder needs to be copied to the build folder.