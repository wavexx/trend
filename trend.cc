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
#include "color.hh"

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

#include <limits>
using std::numeric_limits;

// c system headers
#include <cstdlib>
using std::strtod;
using std::strtoul;

#include <cstring>
using std::memcpy;
using std::strlen;

#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>

// OpenGL/GLU
#include <GL/glut.h>

#ifndef NAN
#define NAN numeric_limits<double>::quiet_NaN()
#endif


/*
 * Data structures
 */

struct Value
{
  Value()
  {}

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
  bool incr = Trend::incr;

  // Data and fixed parameters
  deque<Value> data;
  double loLimit;
  double hiLimit;
  size_t history;
  size_t divisions;

  // Visual/Fixed settings
  int width;
  int height;
  const char* title = NULL;
  GLfloat backCol[3];
  GLfloat textCol[3];
  GLfloat gridCol[3];
  GLfloat lineCol[3];
  GLfloat markCol[3];

  // Visual/Changeable settings
  bool autoLimit;
  bool dimmed = Trend::dimmed;
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
  int c;

  do { c = getc_unlocked(fd); }
  while(isspace(c) && c != EOF);
  ungetc(c, fd);
}


// skip a space-separated string
void
skipStr(FILE* fd)
{
  int c;

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


// read up to the first number in the stream
double
readFirstNum(FILE* fd)
{
  double num;

  do { num = readNum(fd); }
  while(!feof(fd) && isnan(num));

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

    // first value for incremental data
    double old, num;
    if(incr)
      old = readFirstNum(in);

    // read all data
    while(!feof(in))
    {
      num = readNum(in);
      if(isnan(num))
	continue;

      // determine the actual value
      if(incr)
      {
	double tmp = num;
	num = (num - old);
	old = tmp;
      }

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
  glClearColor(backCol[0], backCol[1], backCol[2], 0.0);
}


// Resize handler
void
reshape(const int w, const int h)
{
  width = w;
  height = h;

  glViewport(0, 0, w, h);
}


// write a normal string
void
drawString(const int x, const int y, const char* str)
{
  glRasterPos2i(x, y);
  for(const char* p = str; *p; ++p)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
}


// write an on-screen string using video coordinates
void
drawOSString(const int x, const int y, const char* str)
{
  using Trend::strSpc;

  int len(strlen(str) * 8);
  int rx(x + strSpc);
  int ry(y + strSpc);

  // check x
  if((rx + strSpc + len) > width)
    rx = width - len - strSpc;
  if(rx < 0)
    rx = strSpc;

  // check y
  if((ry + strSpc + 13) > height)
    ry = height - 13 - strSpc;
  if(ry < 0)
    ry = strSpc;

  drawString(rx, ry, str);
}


void
drawGrid()
{
  glColor3fv(gridCol);
  glBegin(GL_LINES);
  double it;

  // horizontal scanlines
  if(divisions < (width / 4))
  {
    for(it = 1; it != divisions; ++it)
    {
      glVertex2d(it, loLimit);
      glVertex2d(it, hiLimit);
    }
  }

  // vertical rasterlines
  if(((hiLimit - loLimit) / gridres) < (height / 4))
  {
    it = loLimit - drem(loLimit, gridres);
    for(; it <= hiLimit; it += gridres)
    {
      glVertex2d(0, it);
      glVertex2d(divisions, it);
    }
  }

  glEnd();
}


void
drawMarker(const float x)
{
  glColor3fv(markCol);
  glBegin(GL_LINES);
  glVertex2d(x, loLimit);
  glVertex2d(x, hiLimit);
  glEnd();
}


size_t
drawLine(Value& last)
{
  last.value = NAN;
  last.count = 0;

  deque<Value>::const_iterator it(data.begin());
  size_t size(data.size());
  size_t pos;

  glBegin(GL_LINE_STRIP);
  for(size_t i = 0; i != data.size(); ++i, ++it)
  {
    last = *it;

    // shade the color
    double alpha(dimmed?
	((size - i) <= divisions? 1.: .5):
	(static_cast<float>(i) / size));
	
    glColor4f(lineCol[0], lineCol[1], lineCol[2], alpha);
    pos = ((scroll? i: last.count) % divisions);
    if(!pos)
    {
      // Cursor at the end
      glVertex2f(divisions, last.value);
      glEnd();
      glBegin(GL_LINE_STRIP);
      glVertex2d(0, last.value);
    }
    else
      glVertex2d(pos, last.value);
  }
  glEnd();

  return pos;
}


void
drawValues(const Value& last)
{
  char buf[Trend::maxNumLen];
  glColor3fv(textCol);

  sprintf(buf, "%g", loLimit);
  drawOSString(width, 0, buf);

  sprintf(buf, "%g", hiLimit);
  drawOSString(width, height, buf);

  sprintf(buf, "%g", last.value);
  drawString(Trend::strSpc, Trend::strSpc, buf);
}


// redraw handler
void
display()
{
  // setup model coordinates
  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  gluOrtho2D(0, divisions, loLimit, hiLimit);

  // background grid
  if(grid)
    drawGrid();

  // data
  pthread_mutex_lock(&mutex);
  Value last;
  size_t pos(drawLine(last));
  pthread_mutex_unlock(&mutex);

  // marker
  if(marker)
    drawMarker(pos);

  // values
  if(values)
  {
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    drawValues(last);
  }

  // flush buffers
  glutSwapBuffers();
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

  // some vertical bounds
  hiLimit = hi + gridres;
  loLimit = lo - gridres;
}


void
idle(int)
{
  // re-register the callback
  glutTimerFunc(1, idle, 0);

  // check if a redraw is really necessary
  pthread_mutex_lock(&mutex);
  if(damaged)
  {
    // recalculate limits seldom
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
toggleStatus(const char* str, bool& var)
{
  var = !var;
  std::cout << str << ": " << (var? "enabled": "disabled") << std::endl;
}


double
getUnit(const char* str)
{
  cout << str << "? ";
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

  // redraw alteration
  case Trend::dimmedKey:
    toggleStatus("dimmed", dimmed);
    break;

  case Trend::autolimKey:
    toggleStatus("autolimit", autoLimit);
    break;

  case Trend::smoothKey:
    toggleStatus("smoothing", smooth);
    init();
    break;

  case Trend::scrollKey:
    toggleStatus("scrolling", scroll);
    break;

  case Trend::valuesKey:
    toggleStatus("values", values);
    break;

  case Trend::markerKey:
    toggleStatus("marker", marker);
    break;

  case Trend::gridKey:
    toggleStatus("grid", grid);
    break;

  case Trend::setResKey:
    gridres = getUnit("grid resolution");
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
  memcpy(backCol, Trend::backCol, sizeof(backCol));
  memcpy(textCol, Trend::textCol, sizeof(textCol));
  memcpy(gridCol, Trend::gridCol, sizeof(gridCol));
  memcpy(lineCol, Trend::lineCol, sizeof(lineCol));
  memcpy(markCol, Trend::markCol, sizeof(markCol));

  int arg;
  while((arg = getopt(argc, argv, "dSsvmgG:ht:A:E:R:I:M:i")) != -1)
    switch(arg)
    {
    case 'd':
      dimmed = !dimmed;
      break;

    case 'S':
      smooth = !smooth;
      break;

    case 's':
      scroll = !scroll;
      break;

    case 'v':
      values = !values;
      break;

    case 'm':
      marker = !marker;
      break;

    case 'g':
      grid = !grid;
      break;

    case 'G':
      gridres = strtod(optarg, NULL);
      break;

    case 't':
      title = optarg;
      break;

    case 'A':
      parseColor(backCol, optarg);
      break;

    case 'E':
      parseColor(textCol, optarg);
      break;

    case 'R':
      parseColor(gridCol, optarg);
      break;

    case 'I':
      parseColor(lineCol, optarg);
      break;

    case 'M':
      parseColor(markCol, optarg);
      break;

    case 'i':
      incr = !incr;
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

  fileName = argv[optind++];
  history = strtoul(argv[optind++], NULL, 0);
  divisions = strtoul(argv[optind++], NULL, 0);

  // optional limiting factors
  if(argc == 5)
  {
    loLimit = strtod(argv[optind++], NULL);
    hiLimit = strtod(argv[optind++], NULL);
    autoLimit = false;
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
