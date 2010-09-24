all: egl-demo

egl-demo: main.cpp
	g++ -o $@ $^ -lX11 -lEGL -lGLESv2
