/*
 * defaults: trend defaults and constants
 * Copyright(c) 2004-2018 by wave++ "Yuri D'Elia" <wavexx@thregr.org>
 * Distributed under GNU LGPL WITHOUT ANY WARRANTY.
 */

#ifndef defaults_hh
#define defaults_hh

// GL headers
#include "gl.hh"


namespace Trend
{
  // Keys
  const unsigned char quitKey =	27;
  const unsigned char autolimKey = 'a';
  const unsigned char resetlimKey = 'A';
  const unsigned char setlimKey = 'L';
  const unsigned char dimmedKey = 'd';
  const unsigned char distribKey = 'D';
  const unsigned char smoothKey = 'S';
  const unsigned char scrollKey = 's';
  const unsigned char valuesKey = 'v';
  const unsigned char markerKey = 'm';
  const unsigned char gridKey = 'g';
  const unsigned char setResKey = 'G';
  const unsigned char setZeroKey = 'z';
  const unsigned char setCAKey = 'Z';
  const unsigned char pauseKey = ' ';
  const unsigned char latKey = 'l';
  const unsigned char fillKey = 'f';
  const unsigned char showUndefKey = 'u';
  const unsigned char pollRateKey = 'p';
  const unsigned char changeKey = '\t';
  const unsigned char graphKeyKey = 'k';
  const unsigned char viewModeKey = 'K';
  const unsigned char helpKey = '?';

  // Some types
  typedef double Value;
  enum input_t {absolute, incremental, differential};
  enum format_t {f_ascii, f_float, f_double, f_short, f_int, f_long};
  enum view_t {v_normal, v_dim, v_hide};
  enum mode_t {m_normal, m_editing, m_message};

  // Defaults
  const input_t input = absolute;
  const format_t format = f_ascii;
  const bool dimmed = false;
  const bool distrib = false;
  const bool smooth = false;
  const bool scroll = false;
  const bool values = false;
  const bool marker = true;
  const bool latency = false;
  const bool filled = false;
  const bool showUndef = false;
  const view_t view = v_normal;
  const bool grid = false;
  const double gridres = 1.;
  const int mayor = 10;
  const int pollMs = 1;

  // Colors
  const GLfloat backCol[3] = {0.0, 0.0, 0.0};
  const GLfloat textCol[3] = {1.0, 1.0, 1.0};
  const GLfloat gridCol[3] = {0.5, 0.0, 0.5};
  const GLfloat markCol[3] = {0.5, 0.5, 0.0};
  const GLfloat intrCol[3] = {0.0, 1.0, 0.0};
  const GLfloat editCol[3] = {1.0, 0.0, 0.0};
  const GLfloat helpCol[3] = {0.0, 1.0, 0.0};

  const GLfloat lineCol[][3] =
  {
    {1.0, 1.0, 1.0},
    {0.0, 1.0, 1.0},
    {0.3, 0.0, 1.0},
    {1.0, 0.0, 0.7},
    {1.0, 0.4, 0.0},
    {1.0, 0.8, 0.0}
  };

  const GLfloat drawOthersAlpha = 0.3;
  const GLfloat fillTrendAlpha = 0.25;
  const GLfloat fillUndefAlpha = 0.125;
  const GLfloat fillTextAlpha = 0.9;

  // Exit
  const int success = 0;
  const int fail = 1;
  const int args = 2;

  // Constants
  const int maxNumLen = 128;
  const int fontHeight = 13;
  const int fontWidth = 8;
  const int strSpc = 2;
  const int intrRad = 4;
  const int intrNum = 3;
  const int distribWidth = 30;
  const int maxGridDens = 4;
  const int latAvg = 5;
  const int persist = 2;
  const int maxPersist = 5;

  // Help string
  const char helpStr[] =
  {
   "HELP\n"
   "\n"
   "-\nKeys:\n"
   "\n"
   "  ESC: quit/exit\n"
   "  TAB: cycle current graph\n"
   "  SPC: pause visualisation (but still continue to consume input)\n"
   "    a: toggle auto-scaling\n"
   "    A: re-scale the graph without activating auto-scaling\n"
   "    d: toggle dimmed shading mode\n"
   "    D: toggle distribution graph\n"
   "    S: toggle anti-aliasing\n"
   "    s: switch scrolling mode (wrap-around or scrolling)\n"
   "    v: toggle values\n"
   "    l: show visual and maximal sync latency\n"
   "    L: set limits interactively\n"
   "    m: activate a marker on the current cursor position\n"
   "    f: toggle filling\n"
   "    g: toggle grid\n"
   "    G: change grid-spec interactively\n"
   "    z: change zero interactively\n"
   "    Z: set limits by center and amplitude\n"
   "    p: change polling rate interactively\n"
   "    u: toggle display of undefined values\n"
   "    k: toggle the graph key\n"
   "    K: cycle view mode (normal, dim others or hide others)\n"
   "\n"
   "-\ngrid-spec:\n"
   "\n"
   "           [[A][+C]][x[B][+C]]\n"
   "\n"
   "           A: y grid resolution\n"
   "           B: x grid resolution\n"
   "           C: draw a mayor line every C normal grid lines"
  };
}

#endif
