// display live data on a trend graph
// Copyright(c) 2003 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
// Distributed under GNU LGPL WITHOUT ANY WARRANTY.

// system headers
#include <stdexcept>
#include <deque>
using std::deque;

#include <iostream>
using std::cout;
using std::cerr;
using std::cin;

#include <string>
using std::string;

#include <utility>
using std::pair;

// c system headers
#include <cstdlib>
using std::strtod;
using std::strtoul;

#include <cstdio>
using std::FILE;
using std::fopen;
using std::fclose;
using std::fscanf;

#include <unistd.h>
#include <pthread.h>

// OpenGL/GLU
#include <GL/glut.h>


// Constants
namespace Exit
{
  const int success(0);
  const int fail(1);
  const int args(2);
}

namespace Default
{
  // Valid keys
  const unsigned char quitKey(27);
  const unsigned char smoothKey('S');
  const unsigned char scrollKey('s');
  const unsigned char valuesKey('v');
  const unsigned char markerKey('m');
  const unsigned char gridKey('g');
  const unsigned char gridOnKey('G');
}


// Horriphilant globals
namespace
{
  // Basic globals
  const char* fileName;
  pthread_mutex_t mutex;
  bool damaged(false);

  // Data and parameters
  deque<pair<float, size_t> > data;
  bool autoLimit;
  float loLimit;
  float hiLimit;
  size_t history;

  // Settings
  size_t divisions;
  bool smooth(false);
  bool values(false);
  bool grid(false);
  bool first(false);
  bool scroll(false);
  bool marker(false);
  float gridOn(1);
}


// Notify OpenGL to redraw everything.
void
notify()
{
  glLoadIdentity();
  gluOrtho2D(0, divisions, loLimit, hiLimit);
  if(!first)
    glutPostRedisplay();
}


// Basic Window OpenGL initializer
void
init()
{
  // Smoothing
  if(smooth)
  {
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
  }
  else
    glDisable(GL_LINE_SMOOTH);

  // Blending should be enabled by default
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Clear color
  glClearColor(0.0, 0.0, 0.0, 0.0);
}


// Write al OpenGL string using glut.
void
drawString(const float scale, const float x, const float y, const char* string)
{
  glPushMatrix();
  glTranslatef(x, y, 0);
  glScalef(scale, scale, 0);
  for(const char* p = string; *p; ++p)
    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
  glPopMatrix();
}


void
drawGrid()
{
  glBegin(GL_LINES);

  // horizontal scanlines
  for(size_t it = 1; it != divisions; ++it)
  {
    glVertex2f(it, loLimit);
    glVertex2f(it, hiLimit);
  }

  // vertical rasterlines
  for(float it = loLimit; it <= hiLimit; it += gridOn)
  {
    glVertex2f(0, it);
    glVertex2f(divisions, it);
  }

  glEnd();
}


void
drawMarker(const float x)
{
  glBegin(GL_LINES);
  glVertex2f(x, loLimit);
  glVertex2f(x, hiLimit);
  glEnd();
}


// Redraw handler
void
display()
{
  // Clear the device.
  pthread_mutex_lock(&mutex);
  glClear(GL_COLOR_BUFFER_BIT);

  if(grid)
  {
    glColor3f(0.5, 0., 0.5);
    drawGrid();
  }

  deque<pair<float, size_t> >::const_iterator it(data.begin());
  size_t pos;

  glBegin(GL_LINE_STRIP);
  for(size_t i = 0; i != data.size(); ++i, ++it)
  {
    glColor4f(1., 1., 1., static_cast<float>(i) / data.size());
    pos = ((scroll? i: it->second) % divisions);
    if(!pos)
    {
      // Cursor at the end
      glVertex2f(divisions, it->first);
      glEnd();
      glBegin(GL_LINE_STRIP);
      glVertex2f(0, it->first);
    }
    else
      glVertex2f(pos, it->first);
  }
  glEnd();

  if(marker)
  {
    glColor3f(0.5, 0.5, 0.);
    drawMarker(pos);
  }

  // Flush buffers
  glutSwapBuffers();
  pthread_mutex_unlock(&mutex);
}


// Resize handler
void
reshape(const int w, const int h)
{
  glViewport(0, 0, w, h);
  notify();
}


void
showToggleStatus(const char* str, bool& var)
{
  var = !var;
  std::cout << str << " == " << (var? "enabled": "disabled") << std::endl;
}


float
getUnit()
{
  cout << "u? ";
  float u;
  cin >> u;

  return u;
}


void
keyboard(const unsigned char key, const int x, const int y)
{
  switch(key)
  {
  case Default::quitKey:
    exit(Exit::success);
    break;

  // Redraw alteration
  case Default::smoothKey:
    showToggleStatus("smoothing", smooth);
    init();
    break;

  case Default::scrollKey:
    showToggleStatus("scrolling", scroll);
    break;

  case Default::markerKey:
    showToggleStatus("marker", marker);
    break;

  case Default::gridKey:
    showToggleStatus("grid", grid);
    break;

  case Default::valuesKey:
    showToggleStatus("values", values);
    break;

  case Default::gridOnKey:
    gridOn = getUnit();
    break;

  default:
    return;
  }

  notify();
}


void
setLimits()
{
  deque<pair<float, size_t> >::const_iterator it(data.begin());
  float lo(it->first);
  float hi(lo);

  for(; it != data.end(); ++it)
  {
    if(it->first > hi)
      hi = it->first;

    if(it->first < lo)
      lo = it->first;
  }

  hiLimit = hi + gridOn;
  loLimit = lo - gridOn;
}


void
idle(int)
{
  // re-register the callback
  glutTimerFunc(1, idle, 0);

  // sadly, using glut we must rely on polling(!)
  pthread_mutex_lock(&mutex);
  if(damaged)
  {
    if(autoLimit)
      setLimits();

    notify();
    damaged = false;
  }
  pthread_mutex_unlock(&mutex);
}


void*
thread(void*)
{
  // Open the file and set some handlers
  FILE* in;
  size_t pos(0);

  while(fileName)
  {
    if(!(in = fopen(fileName, "r")))
      // TODO: should never happen ;)
      break;

    // read all data
    float num;
    while(fscanf(in, "%f", &num) && !feof(in))
    {
      pthread_mutex_lock(&mutex);

      // add the new value
      data.push_back(pair<float, size_t>(num, pos));
      if(data.size() == history + 2)
        data.pop_front();

      // wrap pos when possible
      if(!(++pos % divisions))
        pos = 0;

      damaged = true;
      pthread_mutex_unlock(&mutex);
    }

    fclose(in);
  }

  return NULL;
}


int
main(const int argc, const char* const argv[]) try
{
  // Parameters
  if(argc != 4 && argc != 6)
  {
    cerr << argv[0] << " usage: " <<
      argv[0] << " [options] <fifo> <hist-sz> <x-div> [-y +y]\n" <<
      argv[0] << " version: $Revision$ $Date$\n";

    return Exit::args;
  }

  // TODO: parse some options
  // Start the producer thread
  fileName = argv[1];
  pthread_t thrd;
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&thrd, NULL, thread, NULL);

  // Initialize data
  history = strtoul(argv[2], NULL, 0);
  divisions = strtoul(argv[3], NULL, 0);
  if(argc == 6)
  {
    autoLimit = false;
    loLimit = strtod(argv[4], NULL);
    hiLimit = strtod(argv[5], NULL);
  }
  else
  {
    autoLimit = true;
    loLimit = 0;
    hiLimit = 0;
  }

  // Display
  glutInit(const_cast<int*>(&argc), const_cast<char**>(argv));
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  // Main Window
  glutCreateWindow(argv[0]);
  init();
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);

  // Let glut do its work.
  idle(0);
  first = false;
  glutMainLoop();

  // Terminate the data thread
  fileName = NULL;
  pthread_join(thrd, NULL);
  pthread_mutex_destroy(&mutex);

  return Exit::success;
}
catch(const std::exception& e)
{
  std::cerr << argv[0] << ": " << e.what() << std::endl;
  throw;
}

