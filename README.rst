===============================================
Trend: a general-purpose, efficient trend graph
===============================================

trend is a general-purpose, efficient trend graph for "live" data. Data is read
in ASCII form from a file or continuously from a FIFO and displayed in
real-time into a multi-pass trend (much like a CRT oscilloscope). trend can be
used as a rapid analysis tool for progressive or time-based data series
together with trivial scripting.

.. contents::


Features and requirements
=========================

Features:

- OpenGL graphics
- Automatic or fixed graph scaling
- Two graph scrolling, shading and filling modes
- Configurable colours/grid
- Flexible input
- Interactivity

Requirements:

- OpenGL
- GLUT (http://www.opengl.org/resources/libraries/glut.html) or
  (preferably) FreeGLUT (http://freeglut.sourceforge.net/)
- A recent C++ compiler
- POSIX system (currently tested on Solaris, FreeBSD, OS X, Linux and IRIX).


Building
========

Cd into the distribution's "src" directory and execute "make".

Compiler optimisations are left to the user. Set your standard compiler flags
(CPPFLAGS/CXXFLAGS/LDFLAGS) before building.

Copy the resulting "src/trend" executable and trend's manual "trend.1" where
appropriate.

trend should work on any POSIX/OpenGL capable system.


Usage examples
==============

Some simple example scripts are included in the package.
Within the "examples" directory you can find:

``./mem <seconds>``:

  Print-out active memory (incl. swap) of a linux kernel using /proc/meminfo
  using Perl each tenth of second or the specified number of seconds.

``./imem <seconds>``:

  The same using pmval from Peformance Co-Pilot.

``./net <seconds> [if]``:

  Show network usage (in and out) in bytes using /proc/net/dev
  using Perl (the default network interface if you don't specify any is eth0).

``./tstimes``:

  A more complicated example I use to display server production times without
  particular requirements (the log is parsed and displayed in realtime).

``./timeq [-s] <seconds>``:

  Time-quantize ASCII input: show an average (or total with -s) for received
  values (from stdin) in the specified time-lapse. A common example of usage
  would be in conjunction with the preceding "tstimes", or see the following
  example with apache. Note that timeq outputs binary values to avoid
  double-parses (see/use -fd on the command line). The ASCII parser is not as
  flexible as trend's and requires each value to be in a separated line. Do not
  use this sample implementation for any serious work.

In the following example we will display the latest two minutes of network
activity (with the first one being in front of the other) sampled each tenth of
second::

  ./examples/net 0.1 | trend -c2a -Lin,out - 1200 600

To display the number of current active processes over time you can do::

  (while true; do ps -A | wc -l; sleep 1; done) | trend - ...

Bytes roughly transferred each minute on an apache server?::

  tail -f access.log | \
    sed -une 's/.* \([0-9][0-9]*\) [0-9]*$/\1/p' | \
    ./examples/timeq -s 60 | trend -fd -d - 60x24

An example of using snmpdelta from the NET-SNMP utilities to monitor a remote
IF-MIB network interface::

  snmpdelta -v1 -CT -c public router ifInOctets.1 | trend - ...

A collection of contributed data-gathering scripts is kept at
http://www.thregr.org/~wavexx/software/trend/contrib/ (if you want to make a
contribution just mail me). Alternatively, many (if not all) of the contributed
MRTG scripts are a valuable resource to system administrators in particular.


Accurate timing
===============

trend was designed with accuracy and speed in mind (I use it literally as a
virtual oscilloscope). For this reasons trend offloads to the caller both the
timing and sampling responsibility, allowing trend to be used for any purpose
with maximum precision.

For the non-experts, the scripting convention of sleeping a fixed amount of
time after sampling the value will lead to cumulative timing errors. ASCII
input by itself adds a variable delay, so use binary formats when performance
and latency are a concern.


General/support mailing list
============================

If you feel to discuss improvements and suggestions, and/or test beta releases
before announcement you can subscribe to `trend-users` by either sending an
empty email to <trend-users+subscribe@thregr.org>, using GMane_ (group
"gmane.comp.graphics.trend.general") or by contacting the author at
<wavexx@thregr.org>. The archives are accessible via web through
http://news.gmane.org/gmane.comp.graphics.trend.general or via news directly.

.. _GMane: http://www.gmane.org/


Troubleshooting
===============

trend crashes on start with SIGBUS/SIGSEGV:

  This problem experienced on some machines is caused by the new joystick
  support present in FreeGLUT 2.2.0. Either use standard GLUT, or upgrade to a
  later/cvs version of FreeGLUT (nightly snapshots are fine), where joystick
  initialisation has been made conditional.


Screen-shots
============

Due to popular demand, here's how the screen-shots as found in
http://www.thregr.org/~wavexx/software/trend/ were generated:

trend-and-ion:

	Several instances of trend running under the `ION
	<http://tuomov.iki.fi/software/>`_ window manager. Data source: /proc/
	and mrtg-utils.

trend-distrib:

	trend with the distribution graph active, showing a sine, tangent,
	random-incremental and random function.

trend-intr:

	``trend -d fifo 1200 600``, with the interactive examiners active.
	Input is from a custom board.

trend-oversample:

	``trend -S -I 0x00FF00 fifo 10000x3`` on a ~700 pixels wide window
	(implicit 1x14 oversampling), showing buffer and visual latency in
	respect to the source (taken before the actual release).

trend-qtac:

	Multiple instances of trend running as a server room monitoring system.
	Courtesy of Liam MacKenzie and qtac.edu.au


Further customisation and development
=====================================

Almost all internal aspects and defaults of trend can be changed by modifying
"defaults.hh" and recompiling. If you feel that a default should be changed or
an internal constant be exposed contact me to make the change customizable
instead.

trend's GIT repository is publicly accessible at::

  git://src.thregr.org/trend

or at https://github.com/wavexx/trend


Authors and Copyright
=====================

trend is distributed under LGPL (see COPYING) WITHOUT ANY WARRANTY.
Copyright(c) 2003-2016 by wave++ "Yuri D'Elia" <wavexx@thregr.org>.
Suggestions/comments are welcome. A new version of trend is coming out shortly,
so don't hesitate. Latest trend versions can be downloaded from
http://www.thregr.org/~wavexx/software/trend/
