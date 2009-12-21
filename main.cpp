#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

const char* vertexSrc = "attribute vec4 position; varying mediump vec2 pos; void main() { gl_Position = position; pos = position.xy; }";
const char* fragmentSrc = "varying mediump vec2 pos; uniform mediump float phase; void main() { gl_FragColor = vec4(1, 1, 1, 1) * sin((pos.x * pos.x + pos.y * pos.y) * 40.0 + phase); }";
//const char* fragmentSrc = "varying mediump vec2 pos; uniform mediump float phase; void main() { gl_FragColor = vec4(1, 1, 1, 1) * step(pos.x * pos.x + pos.y * pos.y, phase * 0.2); }";

void printShaderInfoLog(GLuint shader) {
	GLint length;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	if(length) {
		char* buffer = new char[length];
		glGetShaderInfoLog(shader, length, NULL, buffer);
		printf("%s", buffer);
		delete [] buffer;
		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if(success != GL_TRUE) {
			exit(1);
		}
	}
}

GLuint createShader(GLenum type, const char* pSource) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &pSource, NULL);
	glCompileShader(shader);
	printShaderInfoLog(shader);
	return shader;
}

Display* dpy;
Window win;
EGLDisplay edpy;
EGLContext ectxt;
int phaseLocation;
EGLSurface esfc;
	
const float vertexArray[] = {
	0, -1, 0, 1,
	1, 1, 0, 1,
	-1, 1, 0, 1
};

void render() {
		static float offset = 0;
		// draw
		XWindowAttributes gwa;
		XGetWindowAttributes(dpy, win, &gwa);
		glViewport(0, 0, gwa.width, gwa.height);
		glClearColor(0, 1, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUniform1f(phaseLocation, offset);
		
		glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, vertexArray);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		
		eglSwapBuffers(edpy, esfc);
		
		offset = fmodf(offset + 0.2, 2*3.141f);
}

int main() {
	dpy = XOpenDisplay(NULL);
	if(dpy == NULL) {
		printf("cannot connect to X server\n");
		return 1;
	}
	
	Window root = DefaultRootWindow(dpy);

	XSetWindowAttributes swa;
	swa.event_mask = ExposureMask | PointerMotionMask;
	
	win = XCreateWindow(dpy, root, 0, 0, 600, 400, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa);
	
	Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	
	XChangeProperty(dpy, win, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)&fullscreen, 1);
	int one = 1;
	Atom non_composited = XInternAtom(dpy, "_HILDON_NON_COMPOSITED_WINDOW", False);
	XChangeProperty(dpy, win, non_composited, XA_INTEGER, 32, PropModeReplace, (unsigned char*)&one, 1);

	XMapWindow(dpy, win);
	
	XStoreName(dpy, win, "GL test");
	
	edpy = eglGetDisplay((EGLNativeDisplayType)dpy);
	if(edpy == EGL_NO_DISPLAY) {
		printf("Got no EGL display\n");
		return 1;
	}
	
	if(!eglInitialize(edpy, NULL, NULL)) {
		printf("Unable to initialize EGL\n");
		return 1;
	}
	
	EGLint attr[] = {
		EGL_BUFFER_SIZE, 16,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	
	EGLConfig ecfg;
	EGLint num_config;
	if(!eglChooseConfig(edpy, attr, &ecfg, 1, &num_config)) {
		printf("Failed to choose config (%x)\n", eglGetError());
		return 1;
	}
	if(num_config != 1) {
		printf("Didn't get exactly one config, but %d\n", num_config);
		return 1;
	}
	
	esfc = eglCreateWindowSurface(edpy, ecfg, (void*)win, NULL);
	if(esfc == EGL_NO_SURFACE) {
		printf("Unable to create EGL surface (%x)\n", eglGetError());
		return 1;
	}
	
	EGLint ctxattr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	ectxt = eglCreateContext(edpy, ecfg, EGL_NO_CONTEXT, ctxattr);
	if(ectxt == EGL_NO_CONTEXT) {
		printf("Unable to create EGL context (%x)\n", eglGetError());
		return 1;
	}
	eglMakeCurrent(edpy, esfc, esfc, ectxt);
	
	GLuint shaderProgram = glCreateProgram();
	GLuint vertexShader = createShader(GL_VERTEX_SHADER, vertexSrc);
	GLuint fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentSrc);
	
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	
	glLinkProgram(shaderProgram);
	
	glUseProgram(shaderProgram);
	
	phaseLocation = glGetUniformLocation(shaderProgram, "phase");
	if(phaseLocation < 0) {
		printf("Unable to get uniform location\n");
		return 1;
	}
	
	bool quit = false;
	
	timeval startTime;
	timezone tz;
	gettimeofday(&startTime, &tz);
	int numFrames = 0;
	
	while(!quit) {

		while(XPending(dpy)) {
			XEvent xev;
			XNextEvent(dpy, &xev);
		
			if(xev.type == MotionNotify) {
				quit = true;
			}
		}
				render();
				numFrames++;
				if(numFrames % 100 == 0) {
					timeval now;
					gettimeofday(&now, &tz);
					float delta = now.tv_sec - startTime.tv_sec + (now.tv_usec - startTime.tv_usec) * 0.000001f;
					printf("fps: %f\n", numFrames / delta);
				}
	}
	
	eglDestroyContext(edpy, ectxt);
	eglDestroySurface(edpy, esfc);
	eglTerminate(edpy);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	
	return 0;
}
