ARCH          =
OPT           = -O3
CXX           = g++
CXXFLAGS      = -Wall $(OPT) $(ARCH) -fopenmp -fPIC -D_GPHOTO2_INTERNAL_CODE -fpermissive
CC            = gcc
CCFLAGS       = -Wall $(OPT) $(ARCH) -fopenmp -fPIC -D_GPHOTO2_INTERNAL_CODE -fpermissive
INCS          =  -I/usr/local/include/
LD            = g++ -fPIC
LDFLAGS       = 
SOFLAGS       = -shared
LIB_EXTRA     = 
LIBS          = -ljpeg \
							  -L/usr/local/lib \
								-lgphoto2 -lgphoto2_port\
								-Wl,-rpath -Wl,/usr/local/lib\
								-Wl,-rpath -Wl,/usr/lib -Wl,-rpath \
								-Wl,/usr/local/lib -Wl,-rpath -Wl,/usr/lib -fopenmp


HDRS	= gp-params.h actions.h funcs.h logger.h
OBJS	= gp-params.o actions.o funcs.o logger.o

exe:	gui

gui: $(OBJS) capture.o
	$(LD) $(SOFLAGS) -o libcapture.so $(LDFLAGS) capture.o $(OBJS)  $(ARCH) $(LIBS)
	#cp libcapture.so /usr/local/lib/libcapture.so
	LIBRARY_PATH=`pwd` ARCHFLAGS="-arch x86_64" python setup.py build_ext -i

%.o:	%.cxx
	$(CXX) -c $(INCS) $(CXXFLAGS) $(ARCH) $<

%.o:	%.c
	$(CC) -c $(INCS) $(CCFLAGS) $(ARCH) $<

clean:
	rm -f *.o
	rm -f *~
	rm -f .*~
	rm -f py_capture.cpp
	#rm -f /usr/local/lib/libcapture.so

distclean: clean
	rm -f *.so
	rm -f gui
	rm -f *.so
	rm -rf build
	rm -f py_capture.cpp

