/*
 * defaults: trend defaults and constants
 * Copyright(c) 2004 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
 * Distributed under GNU LGPL WITHOUT ANY WARRANTY.
 */

#ifndef defaults_hh
#define defaults_hh

namespace Trend
{
  // Keys
  const unsigned char quitKey =	27;
  const unsigned char autolimKey = 'a';
  const unsigned char smoothKey = 'S';
  const unsigned char scrollKey = 's';
  const unsigned char valuesKey = 'v';
  const unsigned char markerKey = 'm';
  const unsigned char gridKey = 'g';
  const unsigned char setResKey = 'G';

  // Defaults
  const bool smooth = false;
  const bool scroll = false;
  const bool values = false;
  const bool marker = true;
  const bool grid = false;
  const double gridres = 1.;

  // Exit
  const int success = 0;
  const int fail = 1;
  const int args = 2;

  // Constants
  const int maxNumLen = 128;
}

#endif