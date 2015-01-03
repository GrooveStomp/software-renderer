mkdir -p build
pushd build
c++ ../code/main.cpp `sdl2-config --cflags --libs` -o test -g
popd
