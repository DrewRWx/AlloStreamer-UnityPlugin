LIBS=\
-lboost_thread \
-lboost_system \
-loscpack \
-lGL

INCLUDE=\
-I../AlloStreamer-Server

SRC= RenderingPlugin.cpp

all:
	g++ -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)
