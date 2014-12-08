mkdir -p build
pushd build
clang ../Sdl2dGraphics/main.cpp `sdl2-config --cflags --libs` -o test
#clang ../code/software-renderer.cpp -o test
popd
