/*
 * trend: display live data on a trend graph
 * Copyright(c) 2003-2004 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
 * Distributed under GNU LGPL WITHOUT ANY WARRANTY.
 */

/*
 * Headers
 */

// defaults
#include "defaults.hh"

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

#include <unistd.h>
#include <pthread.h>

// OpenGL/GLU
#include <GL/glut.h>


/*
 * Data structures
 */

struct Value
{
  Value(double value, size_t count)
  : value(value), count(count)
  {}

  double value;
  size_t count;
};


/*
 * Graph/Program state
 */

namespace
{
  // Basic data
  const char* fileName;
  pthread_mutex_t mutex;
  bool damaged = false;
  bool first = false;

  // Data and parameters
  deque<Value> data;
  double loLimit;
  double hiLimit;
  size_t history;
  size_t divisions;

  // Visual/Changeable settings
  bool autoLimit = false;
  bool smooth = Trend::smooth;
  bool scroll = Trend::scroll;
  bool values = Trend::values;
  bool marker = Trend::marker;
  bool grid = Trend::grid;
  double gridres = Trend::gridres;
}


/*
 * OpenGL functions
 */

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
  for(float it = loLimit; it <= hiLimit; it += gridres)
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

  deque<Value>::const_iterator it(data.begin());
  size_t pos;

  glBegin(GL_LINE_STRIP);
  for(size_t i = 0; i != data.size(); ++i, ++it)
  {
    glColor4f(1., 1., 1., static_cast<float>(i) / data.size());
    pos = ((scroll? i: it->count) % divisions);
    if(!pos)
    {
      // Cursor at the end
      glVertex2f(divisions, it->value);
      glEnd();
      glBegin(GL_LINE_STRIP);
      glVertex2f(0, it->value);
    }
    else
      glVertex2f(pos, it->value);
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


double
getUnit()
{
  cout << "u? ";
  double u;
  cin >> u;

  return u;
}


void
keyboard(const unsigned char key, const int x, const int y)
{
  switch(key)
  {
  case Trend::quitKey:
    exit(Trend::success);
    break;

  // Redraw alteration
  case Trend::smoothKey:
    showToggleStatus("smoothing", smooth);
    init();
    break;

  case Trend::scrollKey:
    showToggleStatus("scrolling", scroll);
    break;

  case Trend::markerKey:
    showToggleStatus("marker", marker);
    break;

  case Trend::gridKey:
    showToggleStatus("grid", grid);
    break;

  case Trend::valuesKey:
    showToggleStatus("values", values);
    break;

  case Trend::setResKey:
    gridres = getUnit();
    break;

  default:
    return;
  }

  notify();
}


void
setLimits()
{
  deque<Value>::const_iterator it(data.begin());
  float lo(it->value);
  float hi(lo);

  for(; it != data.end(); ++it)
  {
    if(it->value > hi)
      hi = it->value;

    if(it->value < lo)
      lo = it->value;
  }

  hiLimit = hi + gridres;
  loLimit = lo - gridres;
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
      data.push_back(Value(num, pos));
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

    return Trend::args;
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

  return Trend::success;
}
catch(const std::exception& e)
{
  std::cerr << argv[0] << ": " << e.what() << std::endl;
  throw;
}
