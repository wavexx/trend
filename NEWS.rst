trend 1.5: 2018-??-??
---------------------

* Interactive help is shown when '?' is pressed.


trend 1.4: 2016-07-17
---------------------

* Fixed another build failure with GCC 6.1.


trend 1.3: 2016-01-20
---------------------

* Fixed build failure with GCC >= 6.
* Fixed spelling errors in the manpage.


trend 1.2: 2009-11-17
---------------------

* Fix linking error with "binutils-gold"


trend 1.1: 2009-08-14
---------------------

* Auto-scaling now considers only the current graph when multiple graphs are
  present and "hide others" is in effect.
* 'Legend' is now properly called 'Key'. The 'nN' key bindings were also
  remapped to 'kK' for consistency.
* Infinity is now handled like NaNs.
* DEPRECATES: 'nN' key bindings are still available, but should no longer be
  used, use 'kK' instead.


trend 1.0: 2009-08-11
---------------------

* Fix man page warnings.
* Use a traditional version numbering scheme.


Rev #68: 2007-11-02
-------------------

* Polling rate limits can now be configured dynamically.
* Latency sampling now also shows maximal sync times.
* NaNs can now be entered in the stream and highlighted.
* Memory usage reduction (reduced in half).
* 'Z' allows to specify view limits by center and amplitude.
* Support for multiple graphs in a single instance.
* Using '-' as a file name now causes stdin to be read.


Rev #58: 2006-10-03
-------------------

* Graph filling can be enabled with 'f'.
* 'd' now affects filling mode as well.
* Messages are now removed also when paused.
* Created a man page (trend.1).


Rev #54: 2006-04-28
-------------------

* Changing zero now aligns the graph instead of the grid.
* Fixed hung on negative 'grid-spec'.


Rev #49: 2005-12-05
-------------------

* Console is no longer used for input, an embedded line editor is now used (ie
  when setting limits/grid-spec interactively, stdin will eventually be used
  for remote controlling).
* Console is no longer used for output (except for fatal errors), messages are
  now displayed on screen.
* Zero can be set interactively with 'z'.
* Dragging inside the distribution graph now shows the distribution count for
  the selected value.


Rev #44: 2005-07-23
-------------------

* Solaris build fixes.
* Ported to OS X.


Rev #40: 2005-04-15
-------------------

* Correctly align X grid lines in scrolling mode.
* hist-spec now uses "x" instead of "*".
* Support for direct binary input.
* New example (see timeq) and cleanup.
* DEPRECATES: N*M hist-specs should no longer be used, use NxM instead. -i and
  -r flags should no longer be used, use -ci -cd.


Rev #35: 2004-12-14
-------------------

* An "history specification" can be used on the command line as a simplified
  alternative to the hist-sz/x-div pair.
* Visual latency sampling/visualisation.
* Differential input.
* Grid enhancements/offsetting.


Rev #27: 2004-11-02
-------------------

* Incorrectly initialised buffers could cause interactive indicators to
  malfunction for first values.
* Visualisation can be paused while still consuming data.
* 'A' re-scales the graph without activating auto-scaling.


Rev #23: 2004-10-29
-------------------

* Optimisations and huge internal latency reductions.
* Fixed precision truncation in auto-scaling.
* Fixed scrolling mode for "history" non multiple of "divisions".
* Created a support/testing mailing list. See README for details.


Rev #20: 2004-10-21
-------------------

* Implemented distribution graph.
* Fixed trend for plain files.
* Fixed parsing bug (unterminated buffer).
* Fixed precision truncation in drawing code.


Rev #15: 2004-09-27
-------------------

* Fixed standard X11/GLUT options (-display, -geometry, etc)
* Implemented interactive indicators.


Rev #11: 2004-09-21
-------------------

* Several optimisations. Code cleanup.
* Colours are configurable.
* All options have a command line flag now.
* current/min/max values can be shown on the graph.
* New shading mode.
* Input can be an incremental counter.
* Grid positioning was fixed.
* The grid now disables itself when it's too dense to be drawn.
* Auto-scaling can be toggled dynamically.
