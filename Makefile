deps=../deps

OSCSRC= \
	$(deps)/oscpack/osc/OscPrintReceivedElements.cpp \
	$(deps)/oscpack/osc/OscReceivedElements.cpp \
	$(deps)/oscpack/osc/OscTypes.cpp \
	$(deps)/oscpack/ip/IpEndpointName.cpp \
	$(deps)/oscpack/ip/posix/NetworkingUtils.cpp \
	$(deps)/oscpack/ip/posix/UdpSocket.cpp

OSCINCLUDE= \
	-I$(deps)/oscpack/ \
	-I$(deps)/oscpack/ip \
	-I$(deps)/oscpack/ip/posix \
	-I$(deps)/oscpack/osc \

LIBS=-L$(deps)/boost_1_54/build/lib \
	-lboost_thread \
	-lboost_system \
	-lGL \


INCLUDE=-I$(deps)/boost_1_54/build/include \
	-I../AlloServer \

SRC= RenderingPlugin.cpp

all:
	g++ -shared -o libUnityServerPlugin.so -fPIC $(SRC) $(OSCSRC) $(INCLUDE) $(OSCINCLUDE) $(LIBS)
