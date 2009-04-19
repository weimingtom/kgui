# this is the makefile for the Space Sample

KGUILOC=../../
include $(KGUILOC)/makefile.in

SRC=space.cpp

OBJ=$(SRC:%.cpp=$(OBJDIR)/%.o) # replaces the .cpp from SRC with .o

PROGRAM=space$(EXE)

DEP=makefile_$(OBJDIR).dep

$(OBJDIR)/%.o: %.cpp         # combined w/ next line will compile recently changed .cpp files
	$(CC) $(CCOPTS) $(SYS) $(KGUILIB_INCLUDES) -o $@ -c $<

.PHONY : all
all: $(PROGRAM)		# all is dependent on $(PROGRAM) to be complete

ifeq ($(SYS),-DMINGW)

RESOBJ=$(OBJDIR)/simpleres.o

$(RESOBJ): space.rc
	windres space.rc $(RESOBJ)

endif

data.cpp:
	$(KGUILOC)$(OBJDIR)/kguibig$(EXE) data.big big big/
	$(KGUILOC)$(OBJDIR)/bintoc$(EXE) data.big data.cpp

$(PROGRAM): $(DEP) $(OBJ) $(RESOBJ) $(KGUILIB_DEP) data.cpp # $(PROGRAM) is dependent on all of the files in $(OBJ) to exist
	$(LINK) $(RESOBJ) $(KGUILIB_LIBPATHS) $(OBJ) $(KGUILIB_LIBS) $(KGUILIB_SYSLIBS) $(LDFLAGS) -o $@

.PHONY : clean   # .PHONY ignores files named clean
clean:
	-$(RM) $(OBJ) $(RESOBJ) $(PROGRAM) $(DEP) data.big data.cpp # '-' causes errors not to exit the process

$(DEP): data.cpp
	@echo "Generating Dependencies"
	mkdir -p $(OBJDIR)
	-$(CC) $(KGUILIB_INCLUDES) -E -MM $(SYS) $(CFLAGS) $(SRC) >>$(DEP)
	$(KGUILOC)$(OBJDIR)/fixdep$(EXE) $(DEP) $(OBJDIR)/
	

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(DEP)
endif
