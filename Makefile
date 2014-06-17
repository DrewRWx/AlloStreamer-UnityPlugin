include ../Makefile.inc

ifeq ($(UNAME),Darwin)
LIBS=\
-L/opt/local/lib \
-lboost_thread-mt \
-lboost_system-mt

INCLUDE=\
-I/usr/local/include
else
LIBS=\
-lboost_thread \
-lboost_system
endif

LIBS+=\
-loscpack \
-lGL

INCLUDE+=\
-I../AlloStreamer-Server

SRC= RenderingPlugin.cpp

all: clean
	g++ -g -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)

pedantic: clean
	g++ -pedantic -g -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)

clean:
	rm -f libUnityServerPlugin.so