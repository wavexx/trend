trend 1.4 17/07/2016
--------------------

* Fixed another build failure with GCC 6.1.


trend 1.3 20/01/2016
--------------------

* Fixed build failure with GCC >= 6.
* Fixed spelling errors in the manpage.


trend 1.2 17/11/2009
--------------------

* Fix linking error with "binutils-gold"


trend 1.1 14/08/2009
--------------------

* Auto-scaling now considers only the current graph when multiple graphs are
  present and "hide others" is in effect.
* 'Legend' is now properly called 'Key'. The 'nN' key bindings were also
  remapped to 'kK' for consistency.
* Infinity is now handled like NaNs.
* DEPRECATES: 'nN' key bindings are still available, but should no longer be
  used, use 'kK' instead.


trend 1.0 11/08/2009
--------------------

* Fix man page warnings.
* Use a traditional version numbering scheme.


Rev #58 03/10/2006 to Rev #68 02/11/2007
----------------------------------------

* Polling rate limits can now be configured dynamically.
* Latency sampling now also shows maximal sync times.
* NaNs can now be entered in the stream and highlighted.
* Memory usage reduction (reduced in half).
* 'Z' allows to specify view limits by center and amplitude.
* Support for multiple graphs in a single instance.
* Using '-' as a file name now causes stdin to be read.


Rev #54 28/04/2006 to Rev #58 03/10/2006
----------------------------------------

* Graph filling can be enabled with 'f'.
* 'd' now affects filling mode as well.
* Messages are now removed also when paused.
* Created a man page (trend.1).


Rev #49 05/12/2005 to Rev #54 28/04/2006
----------------------------------------

* Changing zero now aligns the graph instead of the grid.
* Fixed hung on negative 'grid-spec'.


Rev #44 23/07/2005 to Rev #49 05/12/2005
----------------------------------------

* Console is no longer used for input, an embedded line editor is now used (ie
  when setting limits/grid-spec interactively, stdin will eventually be used
  for remote controlling).
* Console is no longer used for output (except for fatal errors), messages are
  now displayed on screen.
* Zero can be set interactively with 'z'.
* Dragging inside the distribution graph now shows the distribution count for
  the selected value.


Rev #40 15/04/2005 to Rev #44 23/07/2005
----------------------------------------

* Solaris build fixes.
* Ported to OS X.


Rev #35 14/12/2004 to Rev #40 15/04/2005
----------------------------------------

* Correctly align X grid lines in scrolling mode.
* hist-spec now uses "x" instead of "*".
* Support for direct binary input.
* New example (see timeq) and cleanup.
* DEPRECATES: N*M hist-specs should no longer be used, use NxM instead. -i and
  -r flags should no longer be used, use -ci -cd.


Rev #27 02/11/2004 to Rev #35 14/12/2004
----------------------------------------

* An "history specification" can be used on the command line as a simplified
  alternative to the hist-sz/x-div pair.
* Visual latency sampling/visualisation.
* Differential input.
* Grid enhancements/offsetting.


Rev #23 29/10/2004 to Rev #27 02/11/2004
----------------------------------------

* Incorrectly initialised buffers could cause interactive indicators to
  malfunction for first values.
* Visualisation can be paused while still consuming data.
* 'A' re-scales the graph without activating auto-scaling.


Rev #20 21/10/2004 to Rev #23 29/10/2004
----------------------------------------

* Optimisations and huge internal latency reductions.
* Fixed precision truncation in auto-scaling.
* Fixed scrolling mode for "history" non multiple of "divisions".
* Created a support/testing mailing list. See README for details.


Rev #15 27/09/2004 to Rev #20 21/10/2004
----------------------------------------

* Implemented distribution graph.
* Fixed trend for plain files.
* Fixed parsing bug (unterminated buffer).
* Fixed precision truncation in drawing code.


Rev #11 21/09/2004 to Rev #15 27/09/2004
----------------------------------------

* Fixed standard X11/GLUT options (-display, -geometry, etc)
* Implemented interactive indicators.


Rev #4 09/09/2004 to Rev #11 21/09/2004
---------------------------------------

* Several optimisations. Code cleanup.
* Colours are configurable.
* All options have a command line flag now.
* current/min/max values can be shown on the graph.
* New shading mode.
* Input can be an incremental counter.
* Grid positioning was fixed.
* The grid now disables itself when it's too dense to be drawn.
* Auto-scaling can be toggled dynamically.
