// Example low level rendering Unity plugin

#include "UnityPluginInterface.h"

#include <pthread.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <FrameData.h>
#include <cstring>
#include <cstdlib>
#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/ip/UdpSocket.h"
#ifdef __cplusplus


extern "C" {
#endif


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

unsigned char *image = NULL;


FILE * pluginFile;
bool startedServer = false;
// --------------------------------------------------------------------------
// Include headers for the graphics APIs we support
}
#if SUPPORT_D3D9
	#include <d3d9.h>
#endif
#if SUPPORT_D3D11
	#include <d3d11.h>
#endif
#if SUPPORT_OPENGL
#include <GL/gl.h>
#include <GL/glu.h>
#endif



// --------------------------------------------------------------------------
// Helper utilities


// Prints a string
static void DebugLog (const char* str)
{
	#if UNITY_WIN
	OutputDebugStringA (str);
	#else
	printf ("%s", str);
	#endif
}



// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time=1;

extern "C" void EXPORT_API SetTimeFromUnity (float t) { g_Time = t; }



// --------------------------------------------------------------------------
// SetTextureFromUnity, an example function we export which is called by one of the scripts.

static void* g_TexturePointer;
unsigned int logCount=0;
unsigned char* pixels;

extern "C" void EXPORT_API SetTextureFromUnity (void* texturePtr)
{
	// A script calls this at initialization time; just remember the texture pointer here.
	// Will update texture pixels each frame from the plugin rendering event (texture update
	// needs to happen on the rendering thread).
	g_TexturePointer = texturePtr;



    //pixels = new unsigned char[image_width*image_height*3];

    //    if ((image = (unsigned char*)malloc(3*image_width*image_height*sizeof(char))) == NULL) {
    //        fprintf(stderr,"Failed to allocate memory for image\n");
    //        return 0;
    //    }
    //


    //Returns the error code from starting a new thread (0 if thread was started)
    //int result = startRTSP();

    //fprintf(pluginFile, "StartRTSP() returned %i\n", result);
    //fflush(pluginFile);

    startedServer = true;
}


extern "C" void setLog()
{
	std::string plugin_log = log_dir + "UnityServerPlugin.log";
  pluginFile = fopen(plugin_log.c_str(), "w");

  fprintf(pluginFile, "Initializing interprocess memory...\n");
  fflush(pluginFile);
}
// --------------------------------------------------------------------------
// UnitySetGraphicsDevice

static int g_DeviceType = -1;


// Actual setup/teardown functions defined below
#if SUPPORT_D3D9
static void SetGraphicsDeviceD3D9 (IDirect3DDevice9* device, GfxDeviceEventType eventType);
#endif
#if SUPPORT_D3D11
static void SetGraphicsDeviceD3D11 (ID3D11Device* device, GfxDeviceEventType eventType);
#endif


extern "C" void EXPORT_API UnitySetGraphicsDevice (void* device, int deviceType, int eventType)
{

	// Set device type to -1, i.e. "not recognized by our plugin"
	g_DeviceType = -1;


	#if SUPPORT_OPENGL
	// If we've got an OpenGL device, remember device type. There's no OpenGL
	// "device pointer" to remember since OpenGL always operates on a currently set
	// global context.
	if (deviceType == kGfxRendererOpenGL)
	{
		DebugLog ("Set OpenGL graphics device\n");
		g_DeviceType = deviceType;
	}
	#endif
}



// --------------------------------------------------------------------------
// UnityRenderEvent
// This will be called for GL.IssuePluginEvent script calls; eventID will
// be the integer passed to IssuePluginEvent. In this example, we just ignore
// that value.


struct MyVertex {
	float x, y, z;
	unsigned int color;
};
static void SetDefaultGraphicsState ();
static void DoRendering (const float* worldMatrix, const float* identityMatrix, float* projectionMatrix, const MyVertex* verts);

using namespace boost::interprocess;
mapped_region region;
shared_memory_object shm;
extern "C" void EXPORT_API UnityRenderEvent (int eventID)
{
	// Unknown graphics device type? Do nothing.
	if (g_DeviceType == -1)
		return;


	// A colored triangle. Note that colors will come out differently
	// in D3D9/11 and OpenGL, for example, since they expect color bytes
	// in different ordering.
	MyVertex verts[3] = {
		{ -0.5f, -0.25f,  0, 0xFFff0000 },
		{  0.5f, -0.25f,  0, 0xFF00ff00 },
		{  0,     0.5f ,  0, 0xFF0000ff },
	};


	// Some transformation matrices: rotate around Z axis for world
	// matrix, identity view matrix, and identity projection matrix.

	float phi = g_Time;
	float cosPhi = cosf(phi);
	float sinPhi = sinf(phi);

	float worldMatrix[16] = {
		cosPhi,-sinPhi,0,0,
		sinPhi,cosPhi,0,0,
		0,0,1,0,
		0,0,0.7f,1,
	};
	float identityMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};
	float projectionMatrix[16] = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
	};

	// Actual functions defined below
	SetDefaultGraphicsState ();
	DoRendering (worldMatrix, identityMatrix, projectionMatrix, verts);
}




// --------------------------------------------------------------------------
// SetDefaultGraphicsState
//
// Helper function to setup some "sane" graphics state. Rendering state
// upon call into our plugin can be almost completely arbitrary depending
// on what was rendered in Unity before.
// Before calling into the plugin, Unity will set shaders to null,
// and will unbind most of "current" objects (e.g. VBOs in OpenGL case).
//
// Here, we set culling off, lighting off, alpha blend & test off, Z
// comparison to less equal, and Z writes off.

static void SetDefaultGraphicsState ()
{

	#if SUPPORT_OPENGL
	// OpenGL case
	if (g_DeviceType == kGfxRendererOpenGL)
	{
		glDisable (GL_CULL_FACE);
		glDisable (GL_LIGHTING);
		glDisable (GL_BLEND);
		glDisable (GL_ALPHA_TEST);
		glDepthFunc (GL_LEQUAL);
		glEnable (GL_DEPTH_TEST);
		glDepthMask (GL_FALSE);
	}
	#endif
}


FrameData *data;
static void DoRendering (const float* worldMatrix, const float* identityMatrix, float* projectionMatrix, const MyVertex* verts)
{
	// Does actual rendering of a simple triangle

//    fprintf(pluginFile, "%i: DoRendering()\n", logCount);
//    fflush(pluginFile);
    logCount++;


	#if SUPPORT_OPENGL
	// OpenGL case
	if (g_DeviceType == kGfxRendererOpenGL)
	{
//        fprintf(pluginFile, "%i: OpenGL\n", logCount);
//        fflush(pluginFile);
		// Transformation matrices
		glMatrixMode (GL_MODELVIEW);
		glLoadMatrixf (worldMatrix);
		glMatrixMode (GL_PROJECTION);
		// Tweak the projection matrix a bit to make it match what identity
		// projection would do in D3D case.
		projectionMatrix[10] = 2.0f;
		projectionMatrix[14] = -1.0f;
		glLoadMatrixf (projectionMatrix);

		// Vertex layout
		glVertexPointer (3, GL_FLOAT, sizeof(verts[0]), &verts[0].x);
		glEnableClientState (GL_VERTEX_ARRAY);
		glColorPointer (4, GL_UNSIGNED_BYTE, sizeof(verts[0]), &verts[0].color);
		glEnableClientState (GL_COLOR_ARRAY);

//        fprintf(pluginFile, "%i: DrawArrays()\n", logCount);
//        fflush(pluginFile);

		// Draw!
		//glDrawArrays (GL_TRIANGLES, 0, 3);

		// update native texture from code
		if (g_TexturePointer)
		{
            fprintf(pluginFile, "texture pointer: %u\n", g_TexturePointer);
            fflush(pluginFile);
			GLuint gltex = (GLuint)(size_t)(g_TexturePointer);
			glBindTexture (GL_TEXTURE_2D, gltex);
			int texWidth, texHeight;
			glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
			glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

//            fprintf(pluginFile, "%i: Set texture.\n", logCount);
//            fflush(pluginFile);
            fprintf(pluginFile, "Tex width: %i  Tex height: %i\n", texWidth, texHeight);
			fflush(pluginFile);

            //if(startedServer)
            //{
//                fprintf(pluginFile, "%i: Allocating texture.\n", logCount);
//                fflush(pluginFile);

                //glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);


				unsigned char tempData[1280*720*3] = {0};
                glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_UNSIGNED_BYTE,data->pixels);
//      			int err = glGetError();
//				fprintf(pluginFile,"gl error: %s", gluErrorString(err));
//				fflush(pluginFile);
//				for(int i=26664; i<26664+20; i++)
//				{
//					if(tempData[i] != 0)
//					{
//						fprintf(pluginFile, "tempData[%i]: %u  ", i, tempData[i]);
//					}
//				}
//				fprintf(pluginFile,"\n");
//				fflush(pluginFile);

				//usleep(50000);
                /*
                 *  Encode image into NALs with x264
                 */

//                AVFrame *frame1 = avcodec_alloc_frame();
//                frame1->format = AV_PIX_FMT_YUV420P;
//                frame1->width  = image_width;//c->width;
//                frame1->height = image_height;//c->height;
//                int ret2 = av_image_alloc(frame1->data, frame1->linesize, frame1->width, frame1->height, AV_PIX_FMT_YUV420P, 32);
//
//                avpicture_fill((AVPicture *) frame1, pixels, AV_PIX_FMT_RGB24,image_width,image_height);
//                //    if(frameRGB->linesize[0] >0){
//                //        frameRGB->data[0] += frameRGB->linesize[0]*(c->height -1);
//                //
//                //        frameRGB->linesize[0] = -frameRGB->linesize[0];
//                //    }
//
//                //Flip the image before encoding
//                frame1->data[0] += frame1->linesize[0]*(image_height -1);
//                frame1->linesize[0] = -frame1->linesize[0];
//                sws_scale(convertCtx, frame1->data, frame1->linesize,0, image_height, (u_int8_t *const *)pic_in.img.plane, (const int*)pic_in.img.i_stride);
//
//                x264_nal_t* nals = NULL;
//                int i_nals = 0;
//                int frame_size = -1;
//
//                //printf("encoding... \n");
//
//                frame_size = x264_encoder_encode(encoder, &nals, &i_nals, &pic_in, &pic_out);
//
//                static bool finished = false;
//
//                if (frame_size >= 0)
//                {
//                  static bool alreadydone = false;
//                  if(!alreadydone)
//                  {
//                    //printf("adding headers?... \n");
//
//                    //x264_encoder_headers(encoder, &nals, &i_nals);
//                    alreadydone = true;
//                  }
//
//                }
//
//                for(int i = 0; i < i_nals; ++i)
//                {
//                  //printf("adding to queue... \n");
//                  m_queue.push(nals[i]);
//                }


                //std::memset(region.get_address(), logCount, region.get_size());


                //memcpy(region.get_address(), pixels, image_width*image_height*3);

//                pthread_mutex_lock(&mutex);
//                avpicture_fill((AVPicture *) iframe, pixels, AV_PIX_FMT_RGB24,image_width,image_height);
//                pthread_mutex_unlock(&mutex);

//                fprintf(pluginFile, "%i: After uploading frame\n", logCount);
//                fflush(pluginFile);
                //delete[] pixels;
            //}

			//unsigned char* data = new unsigned char[texWidth*texHeight*4];
			//FillTextureFromCode (texWidth, texHeight, texHeight*4, data);
			//glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
			//delete[] data;
		}
	}
	#endif
}

#define PORT 7000
#define QPORT 8008
float a1 = 0;
float a2 = 0;
float a3 = 0;
float a4 = 0;
float a5 = 0;
float a6 = 0;
float a7 = 0;
float q1 = 0;
float q2 = 0;
float q3 = 0;
float q4 = 0;
extern "C" {
    int startStream();
    int initInterprocessMemory();
	float getRoll();
	float getPitch();
	float getYaw();
    float getTouchX();
    float getTouchY();
    float getDragX();
    float getDragY();
	float getPSQuatX();
	float getPSQuatY();
	float getPSQuatZ();
	float getPSQuatW();
	void endServer();
    void oscStart();
  void oscPhaseSpaceStart();
}
void endServer()
{
    data->shutdownServer = true;
    fprintf(pluginFile, "Ending server...\n");
    fflush(pluginFile);
}

float getRoll()
{
	return a3;
}

float getPitch()
{
	return a2;
}

float getYaw()
{
	return a1;
}
float getTouchX()
{
	return a4;
}
float getTouchY()
{
	return a5;
}
float getDragX()
{
	return a6;
}
float getDragY()
{
	return a7;
}
float getPSQuatX()
{
	return q1;
}
float getPSQuatY()
{
	return q2;
}
float getPSQuatZ()
{
	return q3;
}
float getPSQuatW()
{
	return q4;
}

class QuaternionPacketListener : public osc::OscPacketListener {
protected:

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try{
            // example of parsing single messages. osc::OsckPacketListener
            // handles the bundle traversal.

            // example #2 -- argument iterator interface, supports
            // reflection for overloaded messages (eg you can call
            // (*arg)->IsBool() to check if a bool was passed etc).
            osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
            q1 = (arg++)->AsFloat();
            q2 = (arg++)->AsFloat();
            q3 = (arg++)->AsFloat();
            q4 = (arg++)->AsFloat();

            if( arg != m.ArgumentsEnd() )
                throw osc::ExcessArgumentException();

			//fprintf(pluginFile, "Yaw: %f, Pitch: %f, Roll: %f \n", a1, a2, a3);
			//fflush(pluginFile);

        }catch( osc::Exception& e ){
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            std::cout << "error while parsing message: "
            << m.AddressPattern() << ": " << e.what() << "\n";
        }
    }
};

class OrientationPacketListener : public osc::OscPacketListener {
protected:

    virtual void ProcessMessage( const osc::ReceivedMessage& m,
                                const IpEndpointName& remoteEndpoint )
    {
        (void) remoteEndpoint; // suppress unused parameter warning

        try{
            // example of parsing single messages. osc::OsckPacketListener
            // handles the bundle traversal.

            // example #2 -- argument iterator interface, supports
            // reflection for overloaded messages (eg you can call
            // (*arg)->IsBool() to check if a bool was passed etc).
            osc::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
            a1 = (arg++)->AsFloat();
            a2 = (arg++)->AsFloat();
            a3 = (arg++)->AsFloat();
            a4 = (arg++)->AsFloat();
            a5 = (arg++)->AsFloat();
            a6 = (arg++)->AsFloat();
            a7 = (arg++)->AsFloat();

            if( arg != m.ArgumentsEnd() )
                throw osc::ExcessArgumentException();

			//fprintf(pluginFile, "Yaw: %f, Pitch: %f, Roll: %f \n", a1, a2, a3);
			//fflush(pluginFile);

        }catch( osc::Exception& e ){
            // any parsing errors such as unexpected argument types, or
            // missing arguments get thrown as exceptions.
            std::cout << "error while parsing message: "
            << m.AddressPattern() << ": " << e.what() << "\n";
        }
    }
};

UdpListeningReceiveSocket* s = NULL;
UdpListeningReceiveSocket* qs = NULL;
//UdpListeningReceiveSocket s;
void oscStart()
{
	/*
     * Start OSC client
     */
	OrientationPacketListener listener;
    s = new UdpListeningReceiveSocket(IpEndpointName( IpEndpointName::ANY_ADDRESS, PORT ), &listener );

    s->Run();
}

void oscPhaseSpaceStart()
{
	/*
     * Start OSC client
     */
	QuaternionPacketListener qlistener;
    qs = new UdpListeningReceiveSocket(IpEndpointName( IpEndpointName::ANY_ADDRESS, QPORT ), &qlistener );

    qs->Run();
}


// unsigned char testPixels[image_width*image_height*3];
int initInterprocessMemory()
{



	/*
     * Initialize shared memory between Unity plugin and the Live 555 server/encoder
     */
    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
    } remover;

    //Create a shared memory object.
    shm = shared_memory_object(create_only, "MySharedMemory", read_write);

    //Set size
    shm.truncate(sizeof(FrameData));

    //Map the whole shared memory in this process
    region = mapped_region(shm, read_write);


    //Write all the memory to 1
    //std::memset(region.get_address(), 2, region.get_size());

    //Get the address of the mapped region
    void * addr       = region.get_address();

    //Construct the shared structure in memory
    data = new (addr) FrameData;





//    for(int i=0; i<2048*2048*3; i++)
//    {
//        data->pixels[i] = rand() % 255;
//    }

	/*
     * Start server process
     * Will not return until endServer() is called
     */

   if(0 != std::system("/home/mathieu/Desktop/AlloStreamerServer/AlloServer/AlloServer"))
        return 1;

    //AlloServer is finished, so shutdown OSC
    if(s != NULL)
    {
        s->AsynchronousBreak();
        delete s;
        s = NULL;
    }
    if(qs != NULL)
    {
        qs->AsynchronousBreak();
        delete qs;
        qs = NULL;
    }

    fprintf(pluginFile,"AlloServer exited\n");
    fflush(pluginFile);


    return 0;
}
