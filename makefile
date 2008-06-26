# this is the Makefile for the kgui library

include makefile.in

LIB_SRC=big.cpp datahandle.cpp hash.cpp heap.cpp kgui.cpp kguibasic.cpp kguibrowse.cpp kguibsp.cpp kguicolors.cpp kguicsv.cpp \
kguidb.cpp kguidir.cpp kguifont.cpp kguigrid.cpp kguihtml.cpp kguiimage.cpp kguilist.cpp kguiobj.cpp \
kguipolygon.cpp kguiprot.cpp kguirandom.cpp kguireport.cpp kguireq.cpp kguistring.cpp \
kguitsp.cpp kguixml.cpp kguithread.cpp kguidl.cpp kguiemail.cpp \
kguibutton.cpp kguicombo.cpp kguicontrol.cpp kguidivider.cpp kguiinput.cpp \
kguimenu.cpp kguimcontrol.cpp kguimovie.cpp kguiradio.cpp kguitab.cpp kguitable.cpp kguiwindow.cpp kguitext.cpp \
kguitick.cpp kguibusy.cpp kguimutex.cpp kguicookies.cpp kguiskin.cpp kguimovieplugin.cpp kguipolygon2.cpp kguifont2.cpp kguissl.cpp

FIXDEP_SRC=fixdep.cpp

BINTOC_SRC=bintoc.cpp

KGUIBIG_SRC=big.cpp datahandle.cpp hash.cpp heap.cpp kguibig.cpp kguiprot.cpp kguirandom.cpp kguistring.cpp

LIB_OBJ=$(LIB_SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

FIXDEP_OBJ=$(FIXDEP_SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

BINTOC_OBJ=$(BINTOC_SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

KGUIBIG_OBJ=$(KGUIBIG_SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

LIB_OUT=$(OBJDIR)/libkgui.a

BINTOC_OUT=$(OBJDIR)/bintoc$(EXE)

KGUIBIG_OUT=$(OBJDIR)/kguibig$(EXE)

FIXDEP_OUT=$(OBJDIR)/fixdep$(EXE)

DEP=makefile_$(OBJDIR).dep

CFLAGS= -I"freetype/include" -I"jpeg" -I"lpng128" -I"zlib" -I"mysql/include" -I"ffmpeg/libswscale" -I"ffmpeg/libavformat" -I"ffmpeg/libavcodec" -I"ffmpeg/libavutil"

$(OBJDIR)/%.o: %.cpp         # combined w/ next line will compile recently changed .cpp files
ifeq (,$(findstring $(OBJDIR),$(wildcard $(OBJDIR) )))
	-mkdir $(OBJDIR)
endif
	$(CC) $(CCOPTS) $(SYS) $(CFLAGS) -o $@ -c $<

.PHONY : all doxygen other other-clean # .PHONY ignores files named all
all: $(FIXDEP_OUT) $(BINTOC_OUT) $(KGUIBIG_OUT) _data.cpp $(DEP) $(LIB_OUT)  # all is dependent on  to be complete

$(FIXDEP_OUT): $(FIXDEP_OBJ)
	$(LINK) $(FIXDEP_OBJ) $(LDFLAGS) -o $@

doxygen: dox.txt
	doxygen dox.txt

$(LIB_OUT): $(FIXDEP_OUT) $(BINTOC_OUT) $(KGUIBIG_OUT) _data.cpp $(DEP) $(LIB_OBJ)
	$(MAKELIB) rc $@ $(LIB_OBJ)
	$(RANLIB) $@

$(BINTOC_OUT): $(BINTOC_OBJ)
	$(LINK) $(BINTOC_OBJ) $(LDFLAGS) -o $@

$(KGUIBIG_OUT): $(KGUIBIG_OBJ)
	$(LINK) $(KGUIBIG_OBJ) $(LDFLAGS) -o $@

_data.cpp: $(KGUIBIG_OUT) $(BINTOC_OUT) big/*
	./$(KGUIBIG_OUT) _data.big big big/
	./$(BINTOC_OUT) _data.big _data.cpp

other:		# generate other support libraries
	$(MAKE) -C zlib
	$(MAKE) -C jpeg
	$(MAKE) -C lpng128
	$(MAKE) -C freetype
	$(MAKE) -C ffmpeg/libavutil
	$(MAKE) -C ffmpeg/libavcodec
	$(MAKE) -C ffmpeg/libavformat
	$(MAKE) -C ffmpeg/libswscale

other-clean:		# generate other support libraries
	$(MAKE) -C zlib clean
	$(MAKE) -C jpeg clean
	$(MAKE) -C lpng128 clean
	$(MAKE) -C freetype clean
	$(MAKE) -C ffmpeg clean
	$(MAKE) -C ffmpeg/libavutil clean
	$(MAKE) -C ffmpeg/libavcodec clean
	$(MAKE) -C ffmpeg/libavformat clean
	$(MAKE) -C ffmpeg/libswscale clean

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) _data.cpp $(FIXDEP_OUT) $(FIXDEP_OBJ) $(DEP) $(LIB_OBJ) $(LIB_OUT) $(BINTOC_OBJ) $(BINTOC_OUT) $(KGUIBIG_OBJ) $(KGUIBIG_OUT) # '-' causes errors not to exit the process

$(DEP): _data.cpp $(FIXDEP_OUT)
	@echo "Generating Dependencies"
	-$(CC) -E -MM $(SYS) $(CFLAGS) $(LIB_SRC) >>$(DEP)
	$(FIXDEP_OUT) $(DEP) $(OBJDIR)/

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifeq (,$(findstring other,$(MAKECMDGOALS)))
ifeq (,$(findstring doxygen,$(MAKECMDGOALS)))
-include $(DEP)
endif
endif
endif
