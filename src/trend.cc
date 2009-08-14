/*
 * trend: display live data on a trend graph
 * Copyright(c) 2003-2009 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
 * Distributed under GNU LGPL WITHOUT ANY WARRANTY.
 */

/*
 * Headers
 */

// defaults
#include "version.h"
#include "defaults.hh"
#include "color.hh"
#include "timer.hh"
#include "rr.hh"
using Trend::Value;

// system headers
#include <stdexcept>
#include <algorithm>
#include <vector>
using std::vector;

#include <deque>
using std::deque;

#include <iostream>
using std::cout;
using std::cerr;

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
using std::strpbrk;
using std::strchr;
using std::strcmp;

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

// OpenGL/GLU
#include "gl.hh"

#ifndef NAN
#define NAN (numeric_limits<double>::quiet_NaN())
#endif

#ifndef INF
#define INF (numeric_limits<double>::infinity())
#endif

typedef void (*edit_callback_t)(const string& str);


/*
 * Data structures
 */

struct Intr
{
  // nearest value
  double near;
  size_t pos;

  // intersection
  double value;
  double dist;
};


struct Graph
{
  rr<Value>* rrData;
  Value* rrBuf;
  Value* rrEnd;
  size_t rrPos;
  double zero;
  GLfloat lineCol[3];
  string label;
};


bool
operator<(const Intr& l, const Intr& r)
{
  return (l.dist < r.dist);
}


struct Grid
{
  double res;
  unsigned long mayor;
};

struct GrSpec
{
  Grid x;
  Grid y;
};


/*
 * Graph/Program state
 * TODO: refactor! refactor! refactor!
 */

namespace
{
  // Basic data
  const char* fileName;
  pthread_mutex_t mutex;
  volatile bool damaged = false;
  Trend::input_t input = Trend::input;
  Trend::format_t format = Trend::format;
  bool allowEsc = false;

  // Main graph data
  vector<Graph> graphs;
  Graph* graph;
  double loLimit;
  double hiLimit;
  bool autoLimit;

  // Graph defaults
  vector<string> lineCol;
  vector<string> labels;
  vector<double> zeros;

  // Visual/Fixed settings
  size_t history;
  size_t divisions;
  size_t offset;
  const char* title = NULL;
  GLfloat backCol[3];
  GLfloat textCol[3];
  GLfloat gridCol[3];
  GLfloat markCol[3];
  GLfloat intrCol[3];
  GLfloat editCol[3];

  // Visual/Changeable settings
  int width;
  int height;
  int lc;
  bool dimmed = Trend::dimmed;
  bool smooth = Trend::smooth;
  bool scroll = Trend::scroll;
  bool values = Trend::values;
  bool marker = Trend::marker;
  bool filled = Trend::filled;
  bool showUndef = Trend::showUndef;
  Trend::view_t view = Trend::view;
  bool grid = Trend::grid;
  GrSpec grSpec;
  bool paused = false;
  deque<pair<time_t, string> > messages;
  int pollMs = Trend::pollMs;

  // Key
  bool graphKey;
  size_t maxLabel = 0;
  size_t maxValue = 0;

  // Indicator status
  bool intr = false;
  bool intrFg;
  double intrX;
  double intrY;

  // Distribution graph
  bool distrib = Trend::distrib;
  vector<double> distribData;

  // Latency
  bool latency = Trend::latency;
  ATimer atVLat(Trend::latAvg);
  ATimer atBLat(Trend::latAvg);
  double bLat = 0.;
  double vLat = 0.;

  // Edit form
  bool edit;
  edit_callback_t editCallback;
  string editTitle;
  string editStr;
}


/*
 * I/O and data manipulation
 */

// fill the round robin consistently with a single value
void
rrFill(const Graph& g, double v)
{
  for(size_t i = 0; i != history; ++i)
    g.rrData->push_back(v);
}


// shift values
void
rrShift(const Graph& g, double v)
{
  for(Value* it = g.rrBuf; it != g.rrEnd; ++it)
    *it -= v;
}


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


// read a number from an ascii stream
bool
readANum(FILE* fd, double& v)
{
  for(;;)
  {
    // read
    skipSpc(fd);
    char buf[Trend::maxNumLen];
    char* end = readStr(fd, buf, sizeof(buf) - 1);
    if(feof(fd))
      return false;

    if(!end)
    {
      // long string, skip it.
      skipStr(fd);
      continue;
    }
    *end = 0;

    // convert the number
    v = strtod(buf, &end);
    if(end != buf)
      break;
  }

  return true;
}


// read a number from a binary stream
template <class T> bool
readNum(FILE* fd, double& v)
{
  T buf;
  if(fread(&buf, sizeof(buf), 1, fd) != 1)
    return false;

  v = static_cast<double>(buf);
  return true;
}


// read up to the first number in the stream using the current format
bool
readFNum(FILE* fd, double& v)
{
  switch(format)
  {
  case Trend::f_ascii: return readANum(fd, v);
  case Trend::f_float: return readNum<float>(fd, v);
  case Trend::f_double: return readNum<double>(fd, v);
  case Trend::f_short: return readNum<short>(fd, v);
  case Trend::f_int: return readNum<int>(fd, v);
  case Trend::f_long: return readNum<long>(fd, v);
  }

  return false;
}


// read/handle incoming escape sequences
bool
readEsc(FILE* fd)
{
  // TODO: incomplete
  return true;
}


// read the next valid element from the stream
bool
readNext(FILE* fd, double& v)
{
  if(!allowEsc)
    return readFNum(fd, v);

  while(readFNum(fd, v))
  {
    if(!isinf(v))
      return true;
    if(!readEsc(fd))
      break;
  }

  return false;
}


// producer thread
void*
producer(void* prg)
{
  // iostreams under gcc 3.x are completely unusable for advanced tasks such as
  // customizable buffering/locking/etc. They also removed the (really
  // standard) ->fd() access for "encapsulation"...
  FILE* in;

  // some buffers
  size_t ng = graphs.size();
  double* old = new double[ng];

  for(;;)
  {
    // open the file and disable buffering
    in = (*fileName? fopen(fileName, "r"): stdin);
    if(!in) break;
    setvbuf(in, NULL, _IONBF, 0);

    // check for useless file types
    struct stat stBuf;
    fstat(fileno(in), &stBuf);
    if(S_ISDIR(stBuf.st_mode))
      break;

    // first value for incremental data
    if(input != Trend::absolute)
      for(size_t i = 0; i != ng; ++i)
	if(!readNext(in, old[i]))
	  goto end;

    // read all data
    for(;;)
    {
      for(size_t i = 0; i != ng; ++i)
      {
	double num;
	if(!readNext(in, num))
	  goto end;

	// determine the actual value
	switch(input)
	{
	case Trend::incremental:
	  {
	    double tmp = num;
	    num -= old[i];
	    old[i] = tmp;
	  }
	  break;

	case Trend::differential:
	  old[i] += num;
	  num = old[i];
	  break;

	default:;
	}

	// append the value
	graphs[i].rrData->push_back(num);
      }

      // notify
      pthread_mutex_lock(&mutex);
      if(!damaged)
      {
	atBLat.start();
	damaged = true;
      }
      pthread_mutex_unlock(&mutex);
    }

  end:
    // close the stream and terminate the loop for regular files
    fclose(in);
    if(S_ISREG(stBuf.st_mode) || S_ISBLK(stBuf.st_mode))
      break;
  }

  // should never get so far
  delete []old;
  cerr << reinterpret_cast<char*>(prg) << ": producer thread exiting\n";
  return NULL;
}


/*
 * OpenGL functions
 */

// project model coordinates
void
project(double xi, double yi, int& xo, int& yo)
{
  int width(distrib? ::width - Trend::distribWidth: ::width);
  int off(distrib? Trend::distribWidth: 0);

  xo = (off + static_cast<int>(xi * width / divisions));
  yo = static_cast<int>((yi - loLimit) * height / (hiLimit - loLimit));
}


// unproject video to model coordinates
void
unproject(int xi, int yi, double& xo, double& yo)
{
  int width = (distrib? ::width - Trend::distribWidth: ::width);
  int xr = (distrib? xi - Trend::distribWidth: xi);

  xo = (static_cast<double>(divisions) * xr / width);
  yo = hiLimit - (static_cast<double>(hiLimit - loLimit) * yi / height);
}


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
drawString(const int x, const int y, const string& str)
{
  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, x, y, NULL);

  for(string::const_iterator p = str.begin(); p != str.end(); ++p)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
}


// write strings in the lower-left corner
void
drawLEString(const string& str)
{
  drawString(Trend::strSpc, Trend::strSpc + Trend::fontHeight * lc++, str);
}


// push a message
void
pushMessage(const string& str)
{
  messages.push_back(pair<time_t, string>(time(NULL), str));
  while(messages.size() > static_cast<size_t>(Trend::maxPersist))
    messages.pop_front();
}


// write an on-screen string using video coordinates
void
drawOSString(const int x, const int y, const string& str)
{
  using Trend::strSpc;
  using Trend::fontHeight;
  using Trend::fontWidth;

  int len(str.size() * fontWidth);
  int rx(x + strSpc);
  int ry(y + strSpc);

  // check x
  if((rx + strSpc + len) > width)
    rx = width - len - strSpc;
  if(rx < 0)
    rx = strSpc;

  // check y
  if((ry + strSpc + fontHeight) > height)
    ry = height - fontHeight - strSpc;
  if(ry < 0)
    ry = strSpc;

  drawString(rx, ry, str);
}


void
drawGridX(double gridres)
{
  // horizontal lines
  double it;
  glBegin(GL_LINES);

  if(scroll)
    for(it = divisions; it > 0; it -= gridres)
    {
      glVertex2d(it, loLimit);
      glVertex2d(it, hiLimit);
    }
  else
    for(it = gridres; it <= divisions; it += gridres)
    {
      glVertex2d(it, loLimit);
      glVertex2d(it, hiLimit);
    }

  glEnd();
}


void
drawGridY(double gridres)
{
  // vertical lines
  double it = loLimit - fmod(loLimit, gridres);
  if(it <= loLimit)
    it += gridres;

  glBegin(GL_LINES);
  for(; it < hiLimit; it += gridres)
  {
    glVertex2d(0, it);
    glVertex2d(divisions, it);
  }
  glEnd();
}


void
drawGrid()
{
  using Trend::maxGridDens;
  double r, d;

  // x
  r = (divisions / grSpec.x.res);
  d = (width / maxGridDens);

  if(r < (d * grSpec.x.mayor))
  {
    // minor lines
    if(grSpec.x.mayor != 1 && r < d)
    {
      glColor4f(gridCol[0], gridCol[1], gridCol[2], 0.5);
      drawGridX(grSpec.x.res);
    }

    // mayor lines
    if(grSpec.x.mayor)
    {
      glColor3fv(gridCol);
      drawGridX(grSpec.x.res * grSpec.x.mayor);
    }
  }

  // y
  r = ((hiLimit - loLimit) / grSpec.y.res);
  d = (height / maxGridDens);

  if(r < (d * grSpec.y.mayor))
  {
    // minor lines
    if(grSpec.y.mayor != 1 && r < d)
    {
      glColor4f(gridCol[0], gridCol[1], gridCol[2], 0.5);
      drawGridY(grSpec.y.res);
    }

    // mayor lines
    if(grSpec.y.mayor)
    {
      glColor3fv(gridCol);
      drawGridY(grSpec.y.res * grSpec.y.mayor);
    }
  }
}


void
drawMarker(const double x)
{
  glColor3fv(markCol);
  glBegin(GL_LINES);
  glVertex2d(x, loLimit);
  glVertex2d(x, hiLimit);
  glEnd();
}


void
drawCircle(const int x, const int y)
{
  using Trend::intrRad;

  // ok, it's not really a circle.
  glBegin(GL_LINE_LOOP);
  glVertex2i(x - intrRad, y);
  glVertex2i(x, y + intrRad);
  glVertex2i(x + intrRad, y);
  glVertex2i(x, y - intrRad);
  glEnd();
}


// get count/drawing position based on current settings
size_t
getCount(const Graph& g, const Value* v)
{
  return g.rrPos - (g.rrEnd - v);
}


size_t
getPosition(const Graph& g, size_t pos, const Value* v)
{
  return ((scroll? pos: getCount(g, v)) % divisions);
}


size_t
drawLine(const Graph& g, double alphaMul)
{
  const Value* it = g.rrBuf;
  const Value* nit = it + 1;
  const size_t mark(history + offset - divisions - 1);
  bool st = false;
  size_t pos = 0;

  for(size_t i = offset; it != g.rrEnd; ++i, ++it, ++nit)
  {
    if(!st && isfinite(*it) && (nit == g.rrEnd || isfinite(*nit)))
    {
      st = true;
      glBegin(GL_LINE_STRIP);
    }

    // shade the color
    double alpha(dimmed?
	(i > mark? 1.: .5):
	(static_cast<float>(i - offset) / history));

    glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2], alpha * alphaMul);
    pos = getPosition(g, i, it);

    if(st)
    {
      if(pos)
	glVertex2d(pos, *it);
      else
      {
	// Cursor at the end
	glVertex2d(divisions, *it);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex2d(0, *it);
      }
    }
    else if(isfinite(*it))
    {
      glBegin(GL_LINES);
      if(pos)
      {
	glVertex2d(pos - 0.5, *it);
	glVertex2d(pos + 0.5, *it);
      }
      else
      {
	glVertex2d(0, *it);
	glVertex2d(0.5, *it);
	glVertex2d(divisions, *it);
	glVertex2d(divisions - 0.5, *it);
      }
      glEnd();
    }

    if(st && (nit == g.rrEnd || !isfinite(*nit)))
    {
      glEnd();
      st = false;
    }
  }

  return pos;
}


void
drawFillZero(const Graph& g)
{
  const size_t m = std::min(history, divisions + 1);
  const size_t mark(history + offset - m);
  const Value* it = g.rrEnd - m;
  const Value* nit = it + 1;
  bool st = false;
  Value last = NAN;

  glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2], Trend::fillTrendAlpha);
  for(size_t i = mark; it != g.rrEnd; ++i, ++it, ++nit)
  {
    if(!st && isfinite(*it) && (nit == g.rrEnd || isfinite(*nit)))
    {
      last = *it;
      st = true;
      glBegin(GL_QUAD_STRIP);
    }

    if(st)
    {
      size_t pos = getPosition(g, i, it);

      if((last < 0) != (*it < 0))
      {
	// extra truncation needed
	double zt = (pos? pos: divisions) - *it / (*it - last);
	glVertex2d(zt, 0);
	glVertex2d(zt, 0);
      }

      last = *it;

      if(pos)
      {
	glVertex2d(pos, *it);
	glVertex2d(pos, 0);
      }
      else
      {
	// cursor at the end
	glVertex2d(divisions, *it);
	glVertex2d(divisions, 0);
	glEnd();
	glBegin(GL_QUAD_STRIP);
	glVertex2d(0, *it);
	glVertex2d(0, 0);
      }

      if(nit == g.rrEnd || !isfinite(*nit))
      {
	glEnd();
	st = false;
      }
    }
  }
}


void
drawFillDelta(const Graph& g)
{
  const size_t m = std::min(history - divisions, divisions + 1);
  const size_t mark(history + offset - m);
  const Value* it = g.rrEnd - m;
  const Value* nit = it + 1;
  bool st = false;
  Value l1 = NAN;
  Value l2 = NAN;

  glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2], Trend::fillTrendAlpha);
  glBegin(GL_QUAD_STRIP);
  for(size_t i = mark; it != g.rrEnd; ++i, ++it, ++nit)
  {
    Value v1 = *it;
    Value v2 = *(it - divisions);

    if(!st && (isfinite(v1) && (nit == g.rrEnd || isfinite(*nit)))
	&& (isfinite(v2) && isfinite(*(nit - divisions))))
    {
      l1 = v1;
      l2 = v2;
      st = true;
      glBegin(GL_QUAD_STRIP);
    }

    if(st)
    {
      size_t pos = getPosition(g, i, it);

      if((v1 < v2) != (l1 < l2))
      {
	// extra truncation needed
	double r = (l1 - l2) / (l1 - v1 - l2 + v2);
	double zx = (pos? pos: divisions) - 1 + r;
	double zy = l1 + (v1 - l1) * r;
	glVertex2d(zx, zy);
	glVertex2d(zx, zy);
      }

      if(pos)
      {
	glVertex2d(pos, v1);
	glVertex2d(pos, v2);
      }
      else
      {
	// cursor at the end
	glVertex2d(divisions, v1);
	glVertex2d(divisions, v2);
	glEnd();
	glBegin(GL_QUAD_STRIP);
	glVertex2d(0, v1);
	glVertex2d(0, v2);
      }

      l1 = v1;
      l2 = v2;

      if(nit == g.rrEnd || !isfinite(*nit) || !isfinite(*(nit - divisions)))
      {
	glEnd();
	st = false;
      }
    }
  }
}


void
drawFill(const Graph& g)
{
  if(!dimmed || history < divisions + 2) drawFillZero(g);
  else drawFillDelta(g);
}


void
drawFillUndef(const Graph& g)
{
  const size_t m = std::min(history, divisions + 1);
  const size_t mark(history + offset - m);
  const Value* it = g.rrEnd - m;
  const Value* nit = it + 1;
  bool st = false;
  size_t pos;

  glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2], Trend::fillUndefAlpha);
  for(size_t i = mark; it != g.rrEnd; ++i, ++it, ++nit)
  {
    if(!st && ((nit == g.rrEnd && !isfinite(*it)) || !isfinite(*nit)))
    {
      st = true;
      glBegin(GL_QUAD_STRIP);
    }

    if(st)
    {
      pos = getPosition(g, i, it);

      if(pos)
      {
	glVertex2d(pos, loLimit);
	glVertex2d(pos, hiLimit);
      }
      else
      {
	glVertex2d(divisions, loLimit);
	glVertex2d(divisions, hiLimit);
	glEnd();
	glBegin(GL_QUAD_STRIP);
	glVertex2d(0, loLimit);
	glVertex2d(0, hiLimit);
      }

      if(nit == g.rrEnd || (isfinite(*it) && isfinite(*nit)))
      {
	glEnd();
	st = false;
      }
    }
  }
}


void
drawDistrib()
{
  // reallocate only if necessary. we must avoid to reallocate in order to
  // not fragment memory (resize() on gcc 3 isn't very friendly)
  if(distribData.size() != static_cast<size_t>(height))
    distribData.resize(height);

  // calculate distribution
  const Value* it = graph->rrBuf;
  const Value* end = graph->rrEnd - 1;

  distribData.assign(distribData.size(), 0.);
  double max = 0;

  for(; it != end; ++it)
  {
    const Value* a = it;
    const Value* b = (it + 1);
    if(!isfinite(*a) || !isfinite(*b)) continue;

    // projection
    double mul = (static_cast<double>(height) / (hiLimit - loLimit));
    int begin = static_cast<int>(mul * (*a - loLimit));
    int end = static_cast<int>(mul * (*b - loLimit));
    if(begin > end) std::swap(begin, end);

    // fixation
    if(end < 0 || begin > height) continue;
    if(begin < 0) begin = 0;
    if(end > height) end = height;

    // integration
    for(int y = begin; y != end; ++y)
    {
      if(++distribData[y] > max)
	max = distribData[y];
    }
  }
  if(max != 0.)
    max = 1. / max;

  // draw the results (optimize for continue zones)
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);

  using Trend::distribWidth;
  double oldColor = distribData[0] * max;
  glColor3d(oldColor, oldColor, oldColor);

  glBegin(GL_QUADS);
  glVertex2i(0, 0);
  glVertex2i(distribWidth, 0);

  for(int y = 1; y != (height - 1); ++y)
  {
    double color = distribData[y] * max;
    if(color != oldColor)
    {
      glVertex2i(distribWidth, y);
      glVertex2i(0, y);

      oldColor = color;
      glColor3d(color, color, color);

      glVertex2i(0, y);
      glVertex2i(distribWidth, y);
    }
  }

  glVertex2i(distribWidth, height);
  glVertex2i(0, height);
  glEnd();
  glPopMatrix();
}


void
drawTIntr()
{
  // handle side cases
  const double intrX = (::intrX < 0 || ::intrX > divisions? 0: ::intrX);

  // initial color and current position
  glColor3fv(intrCol);
  glBegin(GL_LINES);
  glVertex2d(intrX, loLimit);
  glVertex2d(intrX, hiLimit);
  glEnd();

  // scan for all intersections
  size_t trX = (static_cast<size_t>(floor(intrX)));
  if(trX == divisions) --trX;
  double mul = (intrX - trX);
  vector<Intr> intrs;

  // starting position
  size_t i = trX;
  const Value* it = graph->rrBuf + (trX - (scroll?
	  offset: getCount(*graph, graph->rrBuf)) % divisions);
  if(it < graph->rrBuf)
    it += divisions;
  if(intrFg)
    it += ((graph->rrEnd - it) / divisions) * divisions;

  const Value* end = graph->rrEnd - 1;
  for(; it < end; i += divisions, it += divisions)
  {
    // fetch the next value
    const Value* nit = (it + 1);
    if(!isfinite(*it) && !isfinite(*nit)) continue;

    Intr buf;
    double far;

    if(mul < 0.5)
    {
      buf.near = *it;
      buf.pos = getPosition(*graph, i, it);
      far = *nit;
    }
    else
    {
      buf.near = *nit;
      buf.pos = getPosition(*graph, i + 1, nit);
      far = *it;
    }

    if(isfinite(buf.near))
    {
      buf.value = (!isfinite(far)? buf.near: *it + mul * (*nit - *it));
      buf.dist = fabs(buf.value - intrY);
      intrs.push_back(buf);
    }
  }

  // no intersections found
  if(!intrs.size())
    return;

  // consider only the nearest n values
  std::sort(intrs.begin(), intrs.end());
  if(intrs.size() > static_cast<size_t>(Trend::intrNum))
    intrs.resize(Trend::intrNum);

  // draw intersections and estimate mean value
  double mean = 0;
  glBegin(GL_LINES);
  for(vector<Intr>::const_iterator it = intrs.begin();
      it != intrs.end(); ++it)
  {
    mean += it->value;
    glVertex2d(0, it->value);
    glVertex2d(divisions, it->value);
  }
  glEnd();
  mean /= intrs.size();

  // switch to video coordinates
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);

  // nearest point
  int nearX, nearY;
  project(intrs[0].pos, intrs[0].near, nearX, nearY);
  drawCircle(nearX, nearY);
  if(!intrs[0].pos)
    drawCircle(width, nearY);

  // plot values
  using Trend::strSpc;

  char buf[256];
  int curY = height;

  sprintf(buf, "nearest: %g, mean: %g", intrs[0].near, mean);
  drawString(strSpc, curY -= Trend::fontHeight + strSpc, buf);

  i = 1;
  for(vector<Intr>::const_iterator it = intrs.begin();
      it != intrs.end(); ++i, ++it)
  {
    sprintf(buf, "%lu: %g", i, it->value);
    drawString(strSpc, curY -= Trend::fontHeight + strSpc, buf);
  }

  // restore model space
  glPopMatrix();
}


void
drawDIntr()
{
  // handle side cases
  int x, y;
  project(intrX, intrY, x, y);
  int nY = (y < 0? 0: (y >= height? height - 1: y));

  // initial color and current position
  glColor3fv(intrCol);
  glBegin(GL_LINES);
  glVertex2d(0, y);
  glVertex2d(width, y);
  glEnd();

  char buf[256];
  sprintf(buf, "%g: %g", intrY, (y != nY? NAN: distribData[nY]));
  drawString(Trend::strSpc, height - Trend::strSpc - Trend::fontHeight, buf);
}


void
drawValues()
{
  const Value& last = graph->rrBuf[history - 1];
  char buf[256];
  glColor3fv(textCol);

  sprintf(buf, "%g", loLimit);
  drawOSString(width, 0, buf);

  sprintf(buf, "%g", hiLimit);
  drawOSString(width, height, buf);

  if(!graphs.size() || graphKey) sprintf(buf, "%g", last);
  else sprintf(buf, "%s: %g", graph->label.c_str(), last);
  drawLEString(buf);
}


void
drawGraphKey()
{
  using Trend::fontHeight;
  using Trend::fontWidth;
  using Trend::strSpc;

  // calculate dynamic widths
  vector<string> lines;
  char buf[256];

  if(values)
  {
    for(size_t i = 0; i != graphs.size(); ++i)
    {
      sprintf(buf, ": %g", graphs[i].rrBuf[history - 1]);
      string str(buf);
      lines.push_back(str);
      if(str.size() > maxValue)
	maxValue = str.size();
    }
  }

  // positioning
  size_t maxLen = maxLabel + (values? 1 + maxValue: 1);
  int boxWidth = fontWidth * 2;
  int boxX1 = width - boxWidth;
  int textX1 = boxX1 - maxLen * fontWidth;
  int textX2 = textX1 + maxLabel * fontWidth;
  int y = height - fontHeight * 2 - strSpc;
  int graphKeyX1 = textX1 - strSpc;
  int graphKeyY1 = y - graphs.size() * fontHeight - strSpc;
  int graphKeyY2 = y + strSpc;

  glColor4f(0., 0., 0., Trend::fillTextAlpha);
  glBegin(GL_QUADS);
  glVertex2i(width, graphKeyY1);
  glVertex2i(graphKeyX1, graphKeyY1);
  glVertex2i(graphKeyX1, graphKeyY2);
  glVertex2i(width, graphKeyY2);
  glEnd();

  for(size_t i = 0; i != graphs.size(); ++i, y -= fontHeight)
  {
    const Graph& g(graphs[i]);

    glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2],
	(&g == graph? 1.: Trend::fillTrendAlpha));
    glBegin(GL_QUADS);
    glVertex2i(width, y - fontHeight);
    glVertex2i(boxX1, y - fontHeight);
    glVertex2i(boxX1, y);
    glVertex2i(width, y);
    glEnd();

    if(&g == graph)
    {
      glColor4f(g.lineCol[0], g.lineCol[1], g.lineCol[2], Trend::fillTrendAlpha);
      glBegin(GL_QUADS);
      glVertex2i(boxX1, y - fontHeight);
      glVertex2i(textX1, y - fontHeight);
      glVertex2i(textX1, y);
      glVertex2i(boxX1, y);
      glEnd();
    }

    glColor3f(textCol[0], textCol[1], textCol[2]);
    drawString(textX1 + (maxLabel - graphs[i].label.size()) * fontWidth,
	y - fontHeight + strSpc, graphs[i].label);
    if(values) drawString(textX2, y - fontHeight + strSpc, lines[i]);
  }
}


void
drawLatency()
{
  char buf[Trend::maxNumLen];
  glColor3fv(textCol);

  sprintf(buf, "lat: %g/%g", vLat, bLat);
  drawLEString(buf);
}


void
drawEdit()
{
  using Trend::fontHeight;
  using Trend::fontWidth;

  const int height2 = height / 2;
  const int width2 = width / 2;
  const int blockH = fontHeight * 2;
  const int blockS = fontHeight / 2;

  glBegin(GL_QUADS);

  // background
  glColor4f(0., 0., 0., Trend::fillTextAlpha);
  glVertex2d(0, height2 + blockH);
  glVertex2d(width, height2 + blockH);
  glVertex2d(width, height2 - blockH);
  glVertex2d(0, height2 - blockH);

  // borders
  glColor3fv(editCol);
  glVertex2d(0, height2 + blockH + blockS);
  glVertex2d(width, height2 + blockH + blockS);
  glVertex2d(width, height2 + blockH);
  glVertex2d(0, height2 + blockH);
  glVertex2d(0, height2 - blockH - blockS);
  glVertex2d(width, height2 - blockH - blockS);
  glVertex2d(width, height2 - blockH);
  glVertex2d(0, height2 - blockH);

  glEnd();

  // edit string
  string buf(editTitle + ": " + editStr);
  drawString(width2 - fontWidth * buf.size() / 2, height2 - blockS, buf);
}


void
drawMessages()
{
  // purge old messages
  time_t maxTime = time(NULL) - Trend::persist;
  while(messages.size() && messages.front().first <= maxTime)
    messages.pop_front();
  if(!messages.size()) return;

  // draw background
  using Trend::fontHeight;
  using Trend::fontWidth;
  using Trend::strSpc;
  const int width2 = width / 2;

  int ty = height - fontHeight * messages.size() - strSpc * 4;
  glColor4f(0., 0., 0., Trend::fillTextAlpha);
  glBegin(GL_QUADS);
  glVertex2d(0, ty);
  glVertex2d(width, ty);
  glVertex2d(width, height);
  glVertex2d(0, height);
  glEnd();

  // draw messages
  int y = height - strSpc - fontHeight;
  glColor3fv(editCol);

  for(deque<pair<time_t, string> >::const_iterator it = messages.begin();
      it != messages.end(); ++it, y -= fontHeight)
  {
    int len = it->second.size() * fontWidth;
    drawString(width2 - len / 2, y, it->second);
  }
}


// redraw handler
void
display()
{
  // reset some data
  lc = 0;

  // setup model coordinates
  double zero = (distrib?
      -(static_cast<double>(Trend::distribWidth) * divisions /
	  (width - Trend::distribWidth)): 0);

  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  gluOrtho2D(zero, divisions, loLimit, hiLimit);

  // background grid and main data
  if(grid) drawGrid();
  if(filled) drawFill(*graph);
  if(showUndef) drawFillUndef(*graph);

  // graphs
  if(view != Trend::v_hide)
  {
    double alphaMul = (view == Trend::v_dim? Trend::drawOthersAlpha: 1.);
    for(vector<Graph>::iterator gi = graphs.begin(); gi != graphs.end(); ++gi)
      if(&*gi != graph)
	drawLine(*gi, alphaMul);
  }
  size_t pos = drawLine(*graph, 1.);

  // other data
  if(distrib) drawDistrib();
  if(marker && !scroll) drawMarker(pos);
  if(intr && (!distrib || intrX >= 0))
    drawTIntr();

  // setup video coordinates
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);

  // video stuff
  if(values) drawValues();
  if(graphKey) drawGraphKey();
  if(latency) drawLatency();
  if(intr && distrib && intrX < 0)
    drawDIntr();

  // editing
  if(edit) drawEdit();
  if(messages.size()) drawMessages();

  // flush buffers
  glutSwapBuffers();
  atVLat.stop();
  vLat = atVLat.avg();
}


void
setGraphLimits(const Graph& g, Value& lo, Value& hi)
{
  for(const Value* it = g.rrBuf; it != g.rrEnd; ++it)
  {
    if(isfinite(*it))
    {
      if(!isfinite(lo) || *it < lo)
	lo = *it;
      if(!isfinite(hi) || *it > hi)
	hi = *it;
    }
  }
}


void
setLimits()
{
  Value lo = *graph->rrBuf;
  Value hi = lo;

  if(view == Trend::v_hide)
  {
    // only operate on the curren graph
    setGraphLimits(*graph, lo, hi);
  }
  else
  {
    for(vector<Graph>::iterator it = graphs.begin(); it != graphs.end(); ++it)
      setGraphLimits(*it, lo, hi);
  }

  // some vertical bounds
  hiLimit = hi + grSpec.y.res;
  loLimit = lo - grSpec.y.res;
}


void
check()
{
  // check if a redraw is really necessary
  bool recalc = false;

  pthread_mutex_lock(&mutex);
  if(damaged)
  {
    damaged = false;
    atBLat.stop();
    bLat = atBLat.avg();
    recalc = true;
  }
  pthread_mutex_unlock(&mutex);

  if(recalc)
  {
    atVLat.start();

    // update buffers
    for(vector<Graph>::iterator gi = graphs.begin(); gi != graphs.end(); ++gi)
    {
      gi->rrPos = gi->rrData->copy(gi->rrBuf);
      if(gi->zero) rrShift(*gi, gi->zero);
    }

    // recalculate limits seldom
    if(autoLimit) setLimits();

    glutPostRedisplay();
  }
}


void
idle(int = 0)
{
  // re-register the callback
  glutTimerFunc(pollMs, idle, 0);

  // consume messages when paused
  if(paused)
  {
    if(messages.size())
      glutPostRedisplay();
    return;
  }

  check();
}


/*
 * Keyboard interaction
 */

// Parse a grid (res+mayor)
void
parseGrid(Grid& grid, char* spec)
{
  char* p = strchr(spec, '+');
  if(p)
  {
    *p++ = 0;
    if(*p) grid.mayor = strtoul(p, NULL, 0);
  }
  if(*spec) grid.res = fabs(strtod(spec, NULL));
}


// Parse a grid-spec
void
parseGrSpec(GrSpec& grid, char* spec)
{
  char* p = strchr(spec, 'x');
  if(p)
  {
    *p++ = 0;
    if(*p) parseGrid(grid.x, p);
  }
  if(*spec) parseGrid(grid.y, spec);
}


void
toggleStatus(const string& str, bool& var)
{
  var = !var;
  pushMessage(str + ": " + (var? "enabled": "disabled"));
}


void
editMode(bool mode);


void
editMode(const string& str, edit_callback_t call)
{
  editTitle = str;
  editStr.clear();
  editCallback = call;
  editMode(true);
}


edit_callback_t
editCall()
{
  edit_callback_t call = editCallback;
  editCallback = NULL;
  (*call)(editStr);
  return editCallback;
}


void
editKeyboard(const unsigned char key, const int, const int)
{
  switch(key)
  {
  case 13:
  case 27:
    if(key == 27 || !editStr.size() || !editCall())
      editMode(false);
    break;

  case 8:
  case 127:
    if(!editStr.size())
      return;
    editStr.resize(editStr.size() - 1);
    break;

  default:
    if(!isprint(key) && editStr.size() + 1 !=
	static_cast<size_t>(Trend::maxNumLen))
      return;
    editStr += key;
  }

  glutPostRedisplay();
}


void
getLimits2(const string& str)
{
  hiLimit = strtod(str.c_str(), NULL);
}


void
getLimits1(const string& str)
{
  if(autoLimit) toggleStatus("autolimit", autoLimit);
  loLimit = strtod(str.c_str(), NULL);
  editMode("+y", getLimits2);
}


void
getLimits()
{
  editMode("-y", getLimits1);
}


void
getCenterAmp2(const string& str)
{
  double amp2 = strtod(str.c_str(), NULL) / 2;
  double c = loLimit + (hiLimit - loLimit) / 2;
  loLimit = c - amp2;
  hiLimit = c + amp2;
}


void
getCenterAmp1(const string& str)
{
  if(autoLimit) toggleStatus("autolimit", autoLimit);
  double c = strtod(str.c_str(), NULL);
  double amp2 = (hiLimit - loLimit) / 2;
  loLimit = c - amp2;
  hiLimit = c + amp2;
  editMode("amplitude", getCenterAmp2);
}


void
getCenterAmp()
{
  editMode("center", getCenterAmp1);
}


void
getGrid(const string& str)
{
  if(!grid) toggleStatus("grid", grid);
  char* buf(strdup(str.c_str()));
  parseGrSpec(grSpec, buf);
  free(buf);
}


void
getZero(const string& str)
{
  double nZero = strtod(str.c_str(), NULL);
  if(nZero != graph->zero)
  {
    rrShift(*graph, nZero - graph->zero);
    graph->zero = nZero;
    if(autoLimit) setLimits();
  }
}


void
setPollRate(const double rate)
{
  pollMs = static_cast<int>(1000. / rate);
}


void
getPollRate(const string& str)
{
  double rate = strtod(str.c_str(), NULL);
  if(rate > 0) setPollRate(rate);
}


void
changeGraph()
{
  if(++graph == &*graphs.end())
    graph = &graphs[0];
  if(!graphKey && !values)
    pushMessage(string("current graph: ") + graph->label);
  if(autoLimit && view == Trend::v_hide)
    setLimits();
}


void
toggleView()
{
  switch(view)
  {
  case Trend::v_normal:
    view = Trend::v_dim;
    pushMessage("view mode: dim others");
    break;

  case Trend::v_dim:
    view = Trend::v_hide;
    if(autoLimit) setLimits();
    pushMessage("view mode: hide others");
    break;

  case Trend::v_hide:
    view = Trend::v_normal;
    if(autoLimit) setLimits();
    pushMessage("view mode: normal");
    break;
  }
}


void
dispKeyboard(const unsigned char key, const int x, const int y)
{
  switch(key)
  {
  case Trend::quitKey:
    exit(Trend::success);
    break;

  // redraw alteration
  case Trend::changeKey:
    changeGraph();
    break;

  case Trend::dimmedKey:
    toggleStatus("dimmed", dimmed);
    break;

  case Trend::viewModeKey:
  case 'N':
    // TODO: 'N' is deprecated
    toggleView();
    break;

  case Trend::distribKey:
    toggleStatus("distribution", distrib);
    break;

  case Trend::autolimKey:
    toggleStatus("autolimit", autoLimit);
    break;

  case Trend::resetlimKey:
    setLimits();
    pushMessage("limits reset");
    break;

  case Trend::setlimKey:
    getLimits();
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

  case Trend::fillKey:
    toggleStatus("fill", filled);
    break;

  case Trend::showUndefKey:
    toggleStatus("show undefined", showUndef);
    break;

  case Trend::graphKeyKey:
  case 'n':
    // TODO: 'n' is deprecated
    toggleStatus("graph key", graphKey);
    break;

  case Trend::gridKey:
    toggleStatus("grid", grid);
    break;

  case Trend::setResKey:
    editMode("grid-spec", getGrid);
    break;

  case Trend::setZeroKey:
    editMode("zero", getZero);
    break;

  case Trend::setCAKey:
    getCenterAmp();
    break;

  case Trend::latKey:
    toggleStatus("latency", latency);
    break;

  case Trend::pollRateKey:
    editMode("poll rate", getPollRate);
    break;

  case Trend::pauseKey:
    toggleStatus("paused", paused);

  default:
    return;
  }

  glutPostRedisplay();
}


void
dispMotion(int x, int y)
{
  unproject(x, y, intrX, intrY);
  glutPostRedisplay();
}


void
dispMouse(int button, int state, int x, int y)
{
  // cause a motion event internally
  intr = (button != GLUT_RIGHT_BUTTON);
  intrFg = (glutGetModifiers() & GLUT_ACTIVE_CTRL);
  dispMotion(x, y);
}


/*
 * CLI and options
 */

// Parse a hist/n, div*n, div spec
bool
parseHistSpec(size_t& hist, size_t& div, const char* spec)
{
  // find the separator first.
  // TODO: * is deprecated
  const char* p = strpbrk(spec, "/*x");
  if((p == spec) || (p && *(p + 1) == 0))
    return true;

  if(!p)
  {
    div = strtoul(spec, NULL, 0);
    hist = div + 1;
    return false;
  }
  else if(*p == '/')
  {
    hist = strtoul(spec, NULL, 0);
    div = strtoul(p + 1, NULL, 0);
    if(!div) return true;
    div = hist / div;
  }
  else
  {
    div = strtoul(spec, NULL, 0);
    hist = div * strtoul(p + 1, NULL, 0);
  }

  return false;
}


size_t
parseInput(Trend::input_t& input, const char* arg)
{
  char* end;
  size_t n = strtoul(arg, &end, 10);
  if(end == arg) n = 1;

  switch(*end)
  {
  case 'a': input = Trend::absolute; break;
  case 'i': input = Trend::incremental; break;
  case 'd': input = Trend::differential; break;

  default:
    return 0;
  };

  return n;
}


bool
parseFormat(Trend::format_t& format, const char* arg)
{
  switch(arg[0])
  {
  case 'a': format = Trend::f_ascii; break;
  case 'f': format = Trend::f_float; break;
  case 'd': format = Trend::f_double; break;
  case 's': format = Trend::f_short; break;
  case 'i': format = Trend::f_int; break;
  case 'l': format = Trend::f_long; break;

  default:
    return true;
  };

  return false;
}


bool
parseNums(vector<double>& nums, char* arg)
{
  char* p;
  while((p = strsep(&arg, ",")))
    nums.push_back(strtod(p, NULL));
  return false;
}


bool
parseStrings(vector<string>& strings, char* arg)
{
  char* p;
  while((p = strsep(&arg, ",")))
    strings.push_back(p);
  return false;
}


// Initialize globals through command line
int
parseOptions(int argc, char* const argv[])
{
  // starting options
  memcpy(backCol, Trend::backCol, sizeof(backCol));
  memcpy(textCol, Trend::textCol, sizeof(textCol));
  memcpy(gridCol, Trend::gridCol, sizeof(gridCol));
  memcpy(markCol, Trend::markCol, sizeof(markCol));
  memcpy(intrCol, Trend::intrCol, sizeof(intrCol));
  memcpy(editCol, Trend::editCol, sizeof(editCol));
  grSpec.x.res = grSpec.y.res = Trend::gridres;
  grSpec.x.mayor = grSpec.y.mayor = Trend::mayor;

  int arg;
  while((arg = getopt(argc, argv, "dDSsvlmFgG:ht:A:E:R:I:M:N:T:L:irz:f:c:p:u:e")) != -1)
    switch(arg)
    {
    case 'd':
      dimmed = !dimmed;
      break;

    case 'D':
      distrib = !distrib;
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

    case 'l':
      latency = !latency;
      break;

    case 'm':
      marker = !marker;
      break;

    case 'F':
      filled = !filled;
      break;

    case 'u':
      showUndef = !showUndef;
      break;

    case 'e':
      allowEsc = !allowEsc;
      break;

    case 'g':
      grid = !grid;
      break;

    case 'G':
      parseGrSpec(grSpec, optarg);
      break;

    case 'z':
      parseNums(zeros, optarg);
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
      parseStrings(lineCol, optarg);
      break;

    case 'M':
      parseColor(markCol, optarg);
      break;

    case 'N':
      parseColor(intrCol, optarg);
      break;

    case 'T':
      parseColor(editCol, optarg);
      break;

    case 'L':
      parseStrings(labels, optarg);
      break;

    case 'c':
      graphs.resize(parseInput(input, optarg));
      if(!graphs.size())
      {
	cerr << argv[0] << ": bad input mode\n";
	return -1;
      }
      break;

    case 'i':
      // TODO: deprecated
      input = Trend::incremental;
      break;

    case 'r':
      // TODO: deprecated
      input = Trend::differential;
      break;

    case 'p':
      {
	double rate = strtod(optarg, NULL);
	if(!rate)
	{
	  cerr << argv[0] << "bad polling rate";
	  return -1;
	}
	setPollRate(rate);
      }
      break;

    case 'f':
      if(parseFormat(format, optarg))
      {
	cerr << argv[0] << ": bad format type\n";
	return -1;
      }
      break;

    case 'h':
      cout << argv[0] << " usage: " <<
	argv[0] << " [options] <fifo|-> <hist-spec|hist-sz x-sz> [-y +y]\n" <<
	argv[0] << " version: " << TREND_VERSION << "\n";
      return 1;

    default:
      return -1;
    }

  // main parameters
  argc -= optind;
  if(argc < 2 || argc > 5)
  {
    cerr << argv[0] << ": bad number of parameters\n";
    return -1;
  }

  // fifo
  fileName = argv[optind++];
  if(!strcmp(fileName, "-"))
    *const_cast<char*>(fileName) = 0;

  // history/divisions
  if(argc == 2 || argc == 4)
  {
    if(parseHistSpec(history, divisions, argv[optind++]))
    {
      cerr << argv[0] << ": bad hist-spec\n";
      return -1;
    }
  }
  else
  {
    history = strtoul(argv[optind++], NULL, 0);
    divisions = strtoul(argv[optind++], NULL, 0);
  }

  // parameters may still be buggy
  if(!divisions)
  {
    cerr << argv[0] << ": x-sz can't be zero\n";
    return -1;
  }
  if(history < 2)
  {
    cerr << argv[0] << ": history must be at least 2\n";
    return -1;
  }
  offset = divisions - (history % divisions) + 1;
  graphKey = labels.size();

  // optional y limits
  if(argc == 4 || argc == 5)
  {
    loLimit = strtod(argv[optind++], NULL);
    hiLimit = strtod(argv[optind++], NULL);
    autoLimit = false;
  }
  else
    autoLimit = true;

  return 0;
}


void
editMode(bool edit)
{
  if(edit)
  {
    glutKeyboardFunc(editKeyboard);
    glutMouseFunc(NULL);
    glutMotionFunc(NULL);
  }
  else
  {
    glutKeyboardFunc(dispKeyboard);
    glutMouseFunc(dispMouse);
    glutMotionFunc(dispMotion);
  }

  ::edit = edit;
}


void
initGraphs()
{
  char buf[Trend::maxNumLen];
  const size_t maxLineCol = (sizeof(Trend::lineCol) / sizeof(*Trend::lineCol));

  for(vector<Graph>::iterator gi = graphs.begin(); gi != graphs.end(); ++gi)
  {
    gi->rrData = new rr<Value>(history);
    gi->rrBuf = new Value[history];
    gi->rrEnd = gi->rrBuf + history;
    gi->rrPos = 0;
    rrFill(*gi, NAN);

    size_t n = gi - graphs.begin();
    gi->zero = (zeros.size() > n? zeros[n]: 0.);

    if(labels.size() > n)
      gi->label = labels[n];
    else
    {
      sprintf(buf, "%lu", n + 1);
      gi->label = buf;
    }

    if(gi->label.size() > maxLabel)
      maxLabel = gi->label.size();

    if(lineCol.size() > n && lineCol[n].size())
      parseColor(gi->lineCol, lineCol[n].c_str());
    else
      memcpy(gi->lineCol, Trend::lineCol[n % maxLineCol], sizeof(GLfloat[3]));
  }
}


int
main(int argc, char* argv[]) try
{
  // parameters
  glutInit(&argc, argv);
  if(parseOptions(argc, argv))
    return Trend::args;

  // initialize rr buffers
  if(!graphs.size()) graphs.resize(1);
  graph = &graphs[0];
  initGraphs();

  // start the producer thread
  pthread_t thrd;
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&thrd, NULL, producer, argv[0]);

  // display, main mindow and callbacks
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutCreateWindow(title? title: argv[0]);
  glutReshapeFunc(reshape);
  glutDisplayFunc(display);
  editMode(false);

  // first redraw
  atVLat.start();
  init();
  idle();

  // processing
  glutMainLoop();
  return Trend::success;
}
catch(const std::exception& e)
{
  std::cerr << argv[0] << ": " << e.what() << std::endl;
  throw;
}
