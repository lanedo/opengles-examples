/* Created by exoticorn (http://talk.maemo.org/showthread.php?t=37356
 * edited and commented by André Bergner
 * g++ -I/media/mmc2/xchange/SDKPackage/Builds/OGLES2/Include -lEGL -lX11 -lGLESv2 egl-example.cpp
 */
#include  <iostream>
using namespace std;

#include  <cmath>
#include  <sys/time.h>

#include  <X11/Xatom.h>
#include  <X11/Xlib.h>
#include  <X11/Xutil.h>
#include  <GLES2/gl2.h>
#include  <EGL/egl.h>



const char vertex_src [] =
"					\
   attribute vec4        position;	\
   varying mediump vec2  pos;		\
   uniform vec4          offset;	\
					\
   void main()				\
   {					\
      gl_Position = position + offset;	\
      pos = position.xy;		\
   }					\
";


const char fragment_src [] =
"								\
   varying mediump vec2    pos;					\
   uniform mediump float   phase;				\
								\
   void  main()							\
   {								\
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) * 		\
      cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y 		\
       - 1.8*pos.x*pos.y*pos.y) + atan(pos.y,pos.x) - phase );	\
   }								\
";
//      cos( 20.*(exp(1.5*pos.x)*pos.x + 2.5*pos.y*pos.y) + phase );	\
//      cos( 30.*(pos.x*pos.x + 1.5*pos.y*pos.y - 1.8*pos.x*pos.y*pos.y) - phase );	\
//      cos( 20.*sqrt(pos.x*pos.x + pos.y*pos.y) + atan(pos.y,pos.x) - phase );	\


void
print_shader_info_log (
   GLuint  shader	// handle to the shader
)
{
   GLint  length;

   glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );

   if ( length ) {
      char* buffer  =  new char [ length ];
      glGetShaderInfoLog ( shader , length , NULL , buffer );
      cout << "shader info: " <<  buffer << flush;
      delete [] buffer;

      GLint success;
      glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
      if ( success != GL_TRUE )   exit ( 1 );
   }
}


GLuint
load_shader (
   const char*  ptr_source,
   GLenum       type
)
{
   GLuint  shader = glCreateShader( type );

   glShaderSource  ( shader , 1 , &ptr_source , NULL );
   glCompileShader ( shader );

   print_shader_info_log ( shader );

   return shader;
}


Display    *x_display;
Window      win;
EGLDisplay  egl_display;
EGLContext  egl_context;

GLfloat
   norm_x,
   norm_y,
   offset_x = 0.0,
   offset_y = 0.0,
   p1_pos_x,
   p1_pos_y;

GLint
   phase_loc,
   offset_loc,
   position_loc = 0;


EGLSurface  egl_surface;
bool        update_pos = false;

const float vertexArray[] = {
   -0.9 ,  -0.5 ,  0.0 ,  1.0,
   -0.9  ,  0.5 ,  0.0 ,  1.0,
    0.9  , -0.5 ,  0.0 ,  1.0,
    0.9  ,  0.5 ,  0.0 ,  1.0
};


void
render()
{
   static float  phase = 0;
   static int    donesetup = 0;

   static XWindowAttributes gwa;

   //// draw

   if ( !donesetup ) {
      XWindowAttributes  gwa;
      XGetWindowAttributes ( x_display , win , &gwa );
      glViewport ( 0 , 0 , gwa.width , gwa.height );
      glClearColor ( 0.08 , 0.06 , 0.07 , 1.);    // background color
      donesetup = 1;
   }
   glClear ( GL_COLOR_BUFFER_BIT );

   glUniform1f ( phase_loc , phase );  // write the value of phase to the shaders phase
   phase  =  fmodf ( phase + 0.5f , 2.f * 3.141f );    // and update the local variable

   if ( update_pos ) {  // if the position of the texture has changed due to user action
      GLfloat old_offset_x  =  offset_x;
      GLfloat old_offset_y  =  offset_y;

      offset_x  =  norm_x - p1_pos_x;
      offset_y  =  norm_y - p1_pos_y;

      p1_pos_x  =  norm_x;
      p1_pos_y  =  norm_y;

      offset_x  +=  old_offset_x;
      offset_y  +=  old_offset_y;

      update_pos = false;
   }

   glUniform4f ( offset_loc  ,  offset_x , offset_y , 0.0 , 0.0 );

   glVertexAttribPointer ( position_loc, 4, GL_FLOAT, false, 0, vertexArray );
   glEnableVertexAttribArray ( 0 );
   glDrawArrays ( GL_TRIANGLE_STRIP, 0, 4 );

   eglSwapBuffers ( egl_display, egl_surface );  // get the rendered buffer to the screen
}


///////////////////////////////////////////////////////////////////////////////


int  main()
{
   x_display = XOpenDisplay ( NULL );
   if ( x_display == NULL ) {
      cerr << "cannot connect to X server" << endl;
      return 1;
   }

   Window root  =  DefaultRootWindow( x_display );

   XSetWindowAttributes  swa;
   swa.event_mask  =  ExposureMask | PointerMotionMask;

   win  =  XCreateWindow (
              x_display, root,
              0, 0, 800, 400,   0,
              CopyFromParent, InputOutput,
              CopyFromParent, CWEventMask,
              &swa );

   XSetWindowAttributes  xattr;
   Atom  atom;
   int   one = 1;

   xattr.override_redirect = False;
   XChangeWindowAttributes ( x_display, win, CWOverrideRedirect, &xattr );

   atom = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", True );
   XChangeProperty (
      x_display, win,
      XInternAtom ( x_display, "_NET_WM_STATE", True ),
      XA_ATOM,  32,  PropModeReplace,
      (unsigned char *)&atom,  1 );

   XChangeProperty (
      x_display, win,
      XInternAtom ( x_display, "_HILDON_NON_COMPOSITED_WINDOW", True ),
      XA_INTEGER,  32,  PropModeReplace,
      (unsigned char *)&one,  1);

   XMapWindow ( x_display, win );
   XStoreName ( x_display, win, "GL test" );

   Atom wm_state   = XInternAtom ( x_display, "_NET_WM_STATE", False );
   Atom fullscreen = XInternAtom ( x_display, "_NET_WM_STATE_FULLSCREEN", False );

   XEvent xev;
   memset ( &xev, 0, sizeof(xev) );

   xev.type                 = ClientMessage;
   xev.xclient.window       = win;
   xev.xclient.message_type = wm_state;
   xev.xclient.format       = 32;
   xev.xclient.data.l[0]    = 1;
   xev.xclient.data.l[1]    = fullscreen;
   XSendEvent (
      x_display,
      DefaultRootWindow ( x_display ),
      False,
      SubstructureNotifyMask,
      &xev );


   ////  up to here x11 stuff only. now we get to the egl stuff

   egl_display  =  eglGetDisplay( (EGLNativeDisplayType)x_display );
   if ( egl_display == EGL_NO_DISPLAY ) {
      cerr << "Got no EGL display" << endl;
      return 1;
   }

   if ( !eglInitialize( egl_display, NULL, NULL ) ) {
      cerr << "Unable to initialize EGL" << endl;
      return 1;
   }

   EGLint attr[] = {
      EGL_BUFFER_SIZE, 16,
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES2_BIT,
      EGL_NONE
   };

   EGLConfig  ecfg;
   EGLint     num_config;
   if ( !eglChooseConfig( egl_display, attr, &ecfg, 1, &num_config ) ) {
      cerr << "Failed to choose config (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }

   if ( num_config != 1 ) {
      cerr << "Didn't get exactly one config, but " << num_config << endl;
      return 1;
   }

   egl_surface = eglCreateWindowSurface ( egl_display, ecfg, (void*)win, NULL );
   if ( egl_surface == EGL_NO_SURFACE ) {
      cerr << "Unable to create EGL surface (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }

   EGLint ctxattr[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
   };
   egl_context = eglCreateContext ( egl_display, ecfg, EGL_NO_CONTEXT, ctxattr );
   if ( egl_context == EGL_NO_CONTEXT ) {
      cerr << "Unable to create EGL context (eglError: " << eglGetError() << ")" << endl;
      return 1;
   }
   eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );


   ////  now we get to actual OpenGL part

   GLuint vertexShader   = load_shader ( vertex_src , GL_VERTEX_SHADER  );	// load vertex shader
   GLuint fragmentShader = load_shader ( fragment_src , GL_FRAGMENT_SHADER );	// load fragment shader

   GLuint shaderProgram  = glCreateProgram ();			// create program object
   glAttachShader ( shaderProgram, vertexShader );		// and attach both...
   glAttachShader ( shaderProgram, fragmentShader );		// ... shaders to it

   glBindAttribLocation( shaderProgram, position_loc, "position");  // bind attribute 'position' to location 0

   glLinkProgram ( shaderProgram );	// link the program
   glUseProgram  ( shaderProgram );	// and select it for usage

   // now get the locations (kind of handle) of the shaders variables 'phase' and 'offset'
   phase_loc  = glGetUniformLocation ( shaderProgram, "phase" );
   offset_loc = glGetUniformLocation ( shaderProgram, "offset" );
   if ( phase_loc < 0 ) {
      cerr << "Unable to get uniform location" << endl;
      return 1;
   }


   float
      window_width  = 800.0,
      window_height = 480.0;

   norm_x    =  -0.;
   norm_y    =  -0;
   p1_pos_x  =  -0.;
   p1_pos_y  =  -0;

   struct  timezone  tz;
   timeval  t1, t2;
   gettimeofday ( &t1 , &tz );
   int  num_frames = 0;

   bool quit = false;
   while ( !quit ) {

      while ( XPending ( x_display ) ) {   // check for events from the x-server

         XEvent  xev;
         XNextEvent( x_display, &xev );

         if ( xev.type == MotionNotify ) {
//            cout << "move to: << xev.xmotion.x << "," << xev.xmotion.y << endl;
            GLfloat window_y  =  (window_height - xev.xmotion.y) - window_height / 2.0;
            norm_y            =  window_y / (window_height / 2.0);
            GLfloat window_x  =  xev.xmotion.x - window_width / 2.0;
            norm_x            =  window_x / (window_width / 2.0);
            update_pos = true;
         }

//         if ( xev.type == KeyPress )   quit = true;	// doesn't work ???
      }

      render();   // now we finally get something on the screen

      if ( ++num_frames % 100 == 0 ) {
         gettimeofday( &t2, &tz );
         float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
         cout << "fps: " << num_frames / dt << endl;
         num_frames = 0;
         t1 = t2;
      }
//      usleep( 1000*10 );
   }


   ////  now clean up

   eglDestroyContext ( egl_display, egl_context );
   eglDestroySurface ( egl_display, egl_surface );
   eglTerminate      ( egl_display );
   XDestroyWindow    ( x_display, win );
   XCloseDisplay     ( x_display );

   return 0;
}

