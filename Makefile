C=-Wall -Wextra -std=c++14 -g -O0\
 -fno-rtti -fno-exceptions -fno-threadsafe-statics
L=-Wl,--as-needed -lGL -lGLU -lglut
space: space.cpp
	clang $(C) $< -o $@ $(L)