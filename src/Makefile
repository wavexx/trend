## Makefile for trend
# Copyright(c) 2003-2018 by wave++ (Yuri D'Elia) <wavexx@thregr.org>

# Flags
CWARN += -Wall -Wextra -Wno-shadow -Wno-unused-parameter
CXXFLAGS += -pthread $(CWARN)
CPPFLAGS += -MD -D_FILE_OFFSET_BITS=64

# Paths
DESTDIR :=
PREFIX := /usr/local

# Objects/targets
TARGETS := trend
trend_OBJECTS := trend.o color.o
trend_LDADD := -lglut -lGL -lGLU


# Rules
.SUFFIXES:
.SECONDEXPANSION:
.PHONY: all clean distclean install

all_OBJECTS := $(foreach T,$(TARGETS),$($(T)_OBJECTS))
all_DEPS := $(all_OBJECTS:.o=.d)
all: $(TARGETS)

%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(TARGETS): %: $$($$@_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $($@_OBJECTS) $(LDFLAGS) $($@_LDADD)

clean:
	rm -f $(all_OBJECTS) $(all_DEPS) $(TARGETS)

install: $(TARGETS)
	install -p -t $(DESTDIR)$(PREFIX)/bin/ $(TARGETS)


# Built files
trend.o: version.h
version.h:
	echo "static const char TREND_VERSION[]=\"`git describe`\";" > $@

distclean: clean
	rm -f version.h


# Dependencies
sinclude $(all_DEPS)
