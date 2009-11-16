## Makefile for trend
## Copyright(c) 2003-2009 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>
## Distributed under GNU LGPL WITHOUT ANY WARRANTY.

#if $(CXX) == "CC"
# pmake (assume Mipspro)
TREND_CXXFLAGS :=
TREND_CPPFLAGS := -I/usr/freeware/include -I/usr/local/include
TREND_LDFLAGS := -FE:template_in_elf_section -quiet_prelink
TREND_LDADD := -lpthread -L/usr/freeware/lib32 -L/usr/local/lib32 -lm -lglut -lGL -lGLU -lX11 -lXmu
#else
ifeq ($(shell uname), Darwin)
# OS X (_nice_ framework system, I admit)
TREND_CXXFLAGS := -pthread
TREND_CPPFLAGS :=
TREND_LDFLAGS :=
TREND_LDADD := -framework GLUT -framework OpenGL
else
# classic posix
TREND_CXXFLAGS := -pthread
TREND_CPPFLAGS := -MD
TREND_LDFLAGS :=
TREND_LDADD := -lglut -lGL -lGLU
endif
#endif

CXXFLAGS += $(TREND_CXXFLAGS)
CPPFLAGS += $(TREND_CPPFLAGS)
LDFLAGS += $(TREND_LDFLAGS)
LDADD += $(TREND_LDADD)


# Config
TREND_BUILT = version.h
TREND_OBJECTS = trend.o color.o
TARGETS = trend


# Rules
.SUFFIXES: .cc .o
.PHONY: all dist clean distclean

.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


# Targets
all: $(TARGETS)

dist: $(TREND_BUILT)

trend: $(TREND_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(TREND_OBJECTS) $(LDADD)

# just update upon tagging
version.h:
	echo -n `git describe` | cencode -q 'static const' -s TREND_VERSION > $@

distclean: clean
	rm -rf $(TREND_BUILT)

clean:
	rm -rf *.[do] core ii_files $(TARGETS)


# Dependencies
trend.o: version.h
sinclude *.d
