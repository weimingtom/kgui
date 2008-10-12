# this is the makefile for GPSTurbo

KGUILOC=../../
include $(KGUILOC)/makefile.in

SRC=browse.cpp

OBJ=$(SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

PROGRAM=browse$(EXE)

DEP=makefile_$(OBJDIR).dep

$(OBJDIR)/%.o: %.cpp         # combined w/ next line will compile recently changed .cpp files
	$(CC) $(CCOPTS) $(SYS) $(KGUILIB_INCLUDES) $(KGUILIBSSL_INCLUDES) -o $@ -c $<

.PHONY : all
all: _brdata.cpp $(PROGRAM)		# all is dependent on $(PROGRAM) to be complete

ifeq ($(SYS),-DMINGW)

RESOBJ=$(OBJDIR)/simpleres.o

$(RESOBJ): browse.rc
	windres browse.rc $(RESOBJ)

endif

$(PROGRAM): $(DEP) $(OBJ) $(RESOBJ) $(KGUILIB_DEP) # $(PROGRAM) is dependent on all of the files in $(OBJ) to exist
	$(LINK) $(RESOBJ) $(KGUILIB_LIBPATHS) $(KGUILIBSSL_LIBPATHS) $(KGUILIBMOVIE_LIBPATHS) $(OBJ) $(KGUILIB_LIBS) $(KGUILIBSSL_LIBS) $(KGUILIBMOVIE_LIBS) $(KGUILIB_SYSLIBS) $(LDFLAGS) -o $@

_brdata.cpp:
	$(KGUILOC)$(OBJDIR)/kguibig$(EXE) _brdata.big big big/
	$(KGUILOC)$(OBJDIR)/bintoc$(EXE) _brdata.big _brdata.cpp

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) $(OBJ) $(RESOBJ) $(PROGRAM) $(DEP) _brdata.big _brdata.cpp

$(DEP): _brdata.cpp
	@echo "Generating Dependencies"
	-mkdir $(OBJDIR)
	-$(CC) $(KGUILIB_INCLUDES) $(KGUILIBSSL_INCLUDES) -E -MM $(SYS) $(CFLAGS) $(SRC) >>$(DEP)
	$(KGUILOC)$(OBJDIR)/fixdep$(EXE) $(DEP) $(OBJDIR)/
	

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(DEP)
endif
