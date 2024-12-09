RM = rm -rf
CPP = g++

CXXFLAGS += -g

INCLUDE_DIRS += -I./include \
                -I../Control/include \
                -I../Guidance/include \
                -I../Motor/include \
                -I$(TRICK_HOME)/include
OBJDIR = obj
LIBDIR = lib
LIBNAME = libVehicleOne.a
LIBOBJS = $(OBJDIR)/VehicleOne.o 

all: ${LIBDIR}/${LIBNAME}

$(LIBOBJS): $(OBJDIR)/%.o : src/%.cpp | $(OBJDIR)
	$(CPP) $(CXXFLAGS) ${INCLUDE_DIRS} -c $< -o $@

${LIBDIR}/${LIBNAME}: ${LIBOBJS} | ${LIBDIR}
	ar crs $@ ${LIBOBJS}

${OBJDIR}:
	mkdir -p ${OBJDIR}

${LIBDIR}:
	mkdir -p ${LIBDIR}

clean:
	${RM} *~
	${RM} ${OBJDIR}

spotless: clean
	${RM} ${LIBDIR}/${LIBNAME}

