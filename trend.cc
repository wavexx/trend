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

#include <cmath>
using std::isnan;

// c system headers
#include <cstdlib>
using std::strtod;
using std::strtoul;

#include <stdio.h>
#include <ctype.h>
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

  // Data and fixed parameters
  deque<Value> data;
  double loLimit;
  double hiLimit;
  size_t history;
  size_t divisions;

  // Visual/Fixed settings
  const char* title = NULL;

  // Visual/Changeable settings
  bool autoLimit;
  bool smooth = Trend::smooth;
  bool scroll = Trend::scroll;
  bool values = Trend::values;
  bool marker = Trend::marker;
  bool grid = Trend::grid;
  double gridres = Trend::gridres;
}


/*
 * I/O and data manipulation
 */

// skip whitespace
void
skipSpc(FILE* fd)
{
  char c;

  do { c = getc_unlocked(fd); }
  while(isspace(c) && c != EOF);
  ungetc(c, fd);
}


// skip a space-separated string
void
skipStr(FILE* fd)
{
  char c;

  do { c = getc_unlocked(fd); }
  while(!isspace(c) && c != EOF);
  ungetc(c, fd);
}


// read a space-separated string
char*
readStr(FILE* fd, char* buf, size_t len)
{
  char* p(buf);
  int c;

  while(len--)
  {
    c = getc_unlocked(fd);
    if(c == EOF || isspace(c))
    {
      ungetc(c, fd);
      return p;
    }
    *p++ = c;
  }

  // overflow
  return NULL;
}


// read a number from the stream
double
readNum(FILE* fd)
{
  // discard initial whitespace
  skipSpc(fd);
  if(feof(fd))
    return NAN;

  // read the number
  char buf[Trend::maxNumLen];
  char* end = readStr(fd, buf, sizeof(buf));
  if(feof(fd))
    return NAN;
  if(!end)
  {
    // long string, skip it.
    skipStr(fd);
    return NAN;
  }
  
  // convert the number
  double num = strtod(buf, &end);
  if(end == buf)
    return NAN;
  
  return num;
}


// producer thread
void*
thread(void*)
{
  // iostreams under gcc 3.x are completely unusable for advanced tasks such as
  // customizable buffering/locking/etc. They also removed the (really
  // standard) ->fd() access for "encapsulation"...
  FILE* in;

  for(size_t pos = 0; fileName;)
  {
    // open the file and disable buffering
    in = fopen(fileName, "r");
    if(!in) break;
    setvbuf(in, NULL, _IONBF, 0);

    // read all data
    double num;

    while(!isnan(num = readNum(in)) && fileName)
    {
      // append the value
      pthread_mutex_lock(&mutex);

      data.push_back(Value(num, pos));
      if(data.size() == history + 2)
	data.pop_front();
      damaged = true;

      pthread_mutex_unlock(&mutex);

      // wrap pos when possible
      if(!(++pos % divisions))
	pos = 0;
    }

    // close the stream
    fclose(in);
  }

  return NULL;
}


/*
 * OpenGL functions
 */

// OpenGL state initializer
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


#if 0
// Write an OpenGL string using glut.
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
#endif


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


// redraw handler
void
display()
{
  glLoadIdentity();
  gluOrtho2D(0, divisions, loLimit, hiLimit);

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
  glutPostRedisplay();
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

  // check if a redraw is necessary
  pthread_mutex_lock(&mutex);
  if(damaged)
  {
    if(autoLimit)
      setLimits();

    glutPostRedisplay();
    damaged = false;
  }
  pthread_mutex_unlock(&mutex);
}


/*
 * Keyboard interation
 */

void
showToggleStatus(const char* str, bool& var)
{
  var = !var;
  std::cout << str << ": " << (var? "enabled": "disabled") << std::endl;
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

  glutPostRedisplay();
}


/*
 * CLI and options
 */

// Initialize globals through command line
int
parseOptions(int argc, char* const argv[])
{
  // starting options
  autoLimit = false;

  int arg;
  while((arg = getopt(argc, argv, "aSsvmgG:ht:")) != -1)
    switch(arg)
    {
    case 'a':
      autoLimit = true;
      break;

    case 'S':
      smooth = true;
      break;

    case 's':
      scroll = true;
      break;

    case 'v':
      values = true;
      break;

    case 'm':
      marker = true;
      break;

    case 'g':
      grid = true;
      break;

    case 'G':
      gridres = strtod(optarg, NULL);
      break;

    case 't':
      title = optarg;
      break;

    case 'h':
      cout << argv[0] << " usage: " <<
	argv[0] << " [options] fifo hist-sz x-div [-y +y]\n" <<
	argv[0] << " version: $Revision$ $Date$\n";
      return 1;

    default:
      return -1;
    }

  // main parameters
  argc -= optind;
  if(argc != 3 && argc != 5)
  {
    cerr << argv[0] << ": bad parameters\n";
    return -1;
  }

  fileName = argv[1];
  history = strtoul(argv[2], NULL, 0);
  divisions = strtoul(argv[3], NULL, 0);

  // optional limiting factors
  if(argc == 5)
  {
    loLimit = strtod(argv[4], NULL);
    hiLimit = strtod(argv[5], NULL);
  }
  else
    autoLimit = true;

  return 0;
}


int
main(int argc, char* const argv[]) try
{
  // parameters
  if(parseOptions(argc, argv))
    return Trend::args;

  // start the producer thread
  pthread_t thrd;
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&thrd, NULL, thread, NULL);

  // display
  glutInit(const_cast<int*>(&argc), const_cast<char**>(argv));
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

  // main mindow and callbacks
  glutCreateWindow(title? title: argv[0]);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);

  // first redraw
  init();
  idle(0);

  // processing
  glutMainLoop();
  return Trend::success;
}
catch(const std::exception& e)
{
  std::cerr << argv[0] << ": " << e.what() << std::endl;
  throw;
}
