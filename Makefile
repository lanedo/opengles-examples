all: egl-demo

clean:
	rm -rf egl-demo

egl-demo: main.c
	gcc -o $@ $^ -lX11 -lEGL -lGLESv2
