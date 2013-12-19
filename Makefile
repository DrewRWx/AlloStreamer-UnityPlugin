deps=../deps

LIBS=\
-L$(deps)/boost_1_54/build/lib \
-lboost_thread \
-lboost_system \
-loscpack \
-lGL

INCLUDE=\
-I$(deps)/boost_1_54/build/include \
-I../AlloServer

SRC= RenderingPlugin.cpp

all:
	g++ -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(INCLUDE) $(LIBS)
