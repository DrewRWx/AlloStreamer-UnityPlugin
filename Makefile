UNAME := $(shell uname)

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
	g++ -O2 -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)

clean:
	rm libUnityServerPlugin.so