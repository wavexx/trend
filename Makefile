# Makefile for trend
# Copyright(c) 2003 by wave++ "Yuri D'Elia" <wavexx@users.sf.net>

# Some flags (MIPSPro)
CPPFLAGS = -I/usr/freeware/include
CXXFLAGS = -g
LDFLAGS = -FE:template_in_elf_section -quiet_prelink
LDADD = -lpthread -L/usr/freeware/lib32 -lglut -lGL -lGLU -lX11 -lXmu


# Config
TARGETS = trend


# Rules
.SUFFIXES: .cc .o
.PHONY: all clean

.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


# Targets
all: $(TARGETS)

trend: trend.o
	$(CXX) $(LDFLAGS) -o $@ trend.o $(LDADD)

clean:
	rm -rf *.o core ii_files *.o $(TARGETS)

