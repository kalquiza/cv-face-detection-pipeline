CC = g++

# define any compile-time flags
CFLAGS = -Wno-deprecated -O3

# define any directories containing header files other than /usr/include
INCLUDES =

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
LFLAGS =

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = `pkg-config --cflags --libs opencv`
LIBS += -L/usr/local/pgsql/lib -lpq;

# define the C source files
IMG_EXTRACT_SRCS = ./img_extract/img_extract.c
HAAR_CASCADES_SRCS = ./haar_cascades/haar_cascades.cpp
BOUNDING_BOXES_SRCS = ./cv_draw/bounding_boxes.cpp
PUPIL_TRACKING_SRCS = ./pupil_tracking/pupil_tracking.cpp ./pupil_tracking/helpers.cpp ./pupil_tracking/findEyeCenter.cpp
PUPIL_CROSSHAIRS_SRCS = ./cv_draw/pupil_crosshairs.cpp
ACTIVE_SHAPE_MODELLING_SRCS = ./active_shape_modelling/active_shape_modelling.cpp ./stasm/asm.cpp ./stasm/convshape.cpp ./stasm/eyedet.cpp ./stasm/faceroi.cpp ./stasm/hatdesc.cpp ./stasm/misc.cpp ./stasm/print.cpp ./stasm/shapehacks.cpp ./stasm/startshape.cpp ./stasm/stasm_lib.cpp ./stasm/classicdesc.cpp ./stasm/err.cpp ./stasm/eyedist.cpp ./stasm/hat.cpp ./stasm/landmarks.cpp ./stasm/pinstart.cpp ./stasm/shape17.cpp ./stasm/shapemod.cpp ./stasm/stasm.cpp ./stasm/MOD_1/facedet.cpp ./stasm/MOD_1/initasm.cpp
DELAUNAY_TRIANGULATION_SRCS = ./cv_draw/delaunay_triangulation.cpp

# define the C object files
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
IMG_EXTRACT_OBJS = $(IMG_EXTRACT_SRCS:.c=.o)
HAAR_CASCADES_OBJS = $(HAAR_CASCADES_SRCS:.c=.o)
BOUNDING_BOXES_OBJS = $(BOUNDING_BOXES_SRCS:.c=.o)
PUPIL_TRACKING_OBJS = $(PUPIL_TRACKING_SRCS:.c=.o)
PUPIL_CROSSHAIRS_OBJS = $(PUPIL_CROSSHAIRS_SRCS:.c=.o)
ACTIVE_SHAPE_MODELLING_OBJS = $(ACTIVE_SHAPE_MODELLING_SRCS:.c=.o)
DELAUNAY_TRIANGULATION_OBJS = $(DELAUNAY_TRIANGULATION_SRCS:.c=.o)

# define the executable file
IMG_EXTRACT = ./build/img_extract
HAAR_CASCADES = ./build/haar_cascades
BOUNDING_BOXES = ./build/bounding_boxes
PUPIL_TRACKING = ./build/pupil_tracking
PUPIL_CROSSHAIRS =  ./build/pupil_crosshairs
ACTIVE_SHAPE_MODELLING = ./build/active_shape_modelling
DELAUNAY_TRIANGULATION = ./build/delaunay_triangulation

# define build directory

MKDIR_P = mkdir -p
OUT_DIR = build

#
# The following part of the makefile is generic; it can be used to
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: build_dir depend clean

ALL: build_dir img_extract haar_cascades bounding_boxes pupil_tracking pupil_crosshairs active_shape_modelling delaunay_triangulaion

build_dir: $(OUT_DIR)
img_extract: $(IMG_EXTRACT)
haar_cascades: $(HAAR_CASCADES)
bounding_boxes: $(BOUNDING_BOXES)
pupil_tracking: $(PUPIL_TRACKING)
pupil_crosshairs: $(PUPIL_CROSSHAIRS)
active_shape_modelling: $(ACTIVE_SHAPE_MODELLING)
delaunay_triangulaion: $(DELAUNAY_TRIANGULATION)

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

$(IMG_EXTRACT): $(IMG_EXTRACT_OBJS)
	$(CC) -o $(IMG_EXTRACT) $(IMG_EXTRACT_OBJS) $(LIBS)

$(HAAR_CASCADES): $(HAAR_CASCADES_OBJS)
	$(CC) -o $(HAAR_CASCADES) $(HAAR_CASCADES_OBJS) $(LIBS)

$(BOUNDING_BOXES): $(BOUNDING_BOXES_OBJS)
	$(CC) -o $(BOUNDING_BOXES) $(BOUNDING_BOXES_OBJS) $(LIBS)

$(PUPIL_TRACKING): $(PUPIL_TRACKING_OBJS)
	$(CC) -o $(PUPIL_TRACKING) $(PUPIL_TRACKING_OBJS) $(LIBS)

$(PUPIL_CROSSHAIRS): $(PUPIL_CROSSHAIRS_OBJS)
	$(CC) -o $(PUPIL_CROSSHAIRS) $(PUPIL_CROSSHAIRS_OBJS) $(LIBS)

$(ACTIVE_SHAPE_MODELLING): $(ACTIVE_SHAPE_MODELLING_OBJS)
	$(CC) $(CFLAGS) -o $(ACTIVE_SHAPE_MODELLING) $(ACTIVE_SHAPE_MODELLING_OBJS) $(LIBS)

$(DELAUNAY_TRIANGULATION): $(DELAUNAY_TRIANGULATION_OBJS)
	$(CC) -o $(DELAUNAY_TRIANGULATION) $(DELAUNAY_TRIANGULATION_OBJS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
