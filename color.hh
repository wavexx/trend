/*
 * color: color parsing/lookup functions
 * Copyright(c) 2004 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
 * Distributed under GNU LGPL WITHOUT ANY WARRANTY.
 */

#ifndef color_hh
#define color_hh

// GL headers
#include <GL/gl.h>


// parse a color in hexadecimal format ([[#]0x]RRGGBB)
GLfloat*
parseColor(GLfloat* buf, const char* color);

#endif
