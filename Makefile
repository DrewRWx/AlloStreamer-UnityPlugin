LIBS=\
-lboost_thread \
-lboost_system \
-loscpack \
-lGL

INCLUDE=\
-I../AlloStreamer-Server

SRC= RenderingPlugin.cpp

all:
	g++ -O2 -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)
