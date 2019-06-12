/************************************************************************************

Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
Copyright   :   Copyright Brad Davis. All Rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#include <iostream>
#include <memory>
#include <exception>
#include <algorithm>
#include "ServerGame.h"
#include <Windows.h>

#define __STDC_FORMAT_MACROS 1

#define FAIL(X) throw std::runtime_error(X)

///////////////////////////////////////////////////////////////////////////////
//
// GLM is a C++ math library meant to mirror the syntax of GLSL 
//

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Skybox.h"

// Import the most commonly used types into the default namespace
using glm::ivec3;
using glm::ivec2;
using glm::uvec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::quat;

///////////////////////////////////////////////////////////////////////////////
//
// GLEW gives cross platform access to OpenGL 3.x+ functionality.  
//

#include <GL/glew.h>

bool checkFramebufferStatus(GLenum target = GL_FRAMEBUFFER)
{
  GLuint status = glCheckFramebufferStatus(target);
  switch (status)
  {
  case GL_FRAMEBUFFER_COMPLETE:
    return true;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
    std::cerr << "framebuffer incomplete attachment" << std::endl;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
    std::cerr << "framebuffer missing attachment" << std::endl;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
    std::cerr << "framebuffer incomplete draw buffer" << std::endl;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
    std::cerr << "framebuffer incomplete read buffer" << std::endl;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
    std::cerr << "framebuffer incomplete multisample" << std::endl;
    break;

  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
    std::cerr << "framebuffer incomplete layer targets" << std::endl;
    break;

  case GL_FRAMEBUFFER_UNSUPPORTED:
    std::cerr << "framebuffer unsupported internal format or image" << std::endl;
    break;

  default:
    std::cerr << "other framebuffer error" << std::endl;
    break;
  }

  return false;
}

bool checkGlError()
{
  GLenum error = glGetError();
  if (!error)
  {
    return false;
  }
  else
  {
    switch (error)
    {
    case GL_INVALID_ENUM:
      std::cerr <<
        ": An unacceptable value is specified for an enumerated argument.The offending command is ignored and has no other side effect than to set the error flag.";
      break;
    case GL_INVALID_VALUE:
      std::cerr <<
        ": A numeric argument is out of range.The offending command is ignored and has no other side effect than to set the error flag";
      break;
    case GL_INVALID_OPERATION:
      std::cerr <<
        ": The specified operation is not allowed in the current state.The offending command is ignored and has no other side effect than to set the error flag..";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      std::cerr <<
        ": The framebuffer object is not complete.The offending command is ignored and has no other side effect than to set the error flag.";
      break;
    case GL_OUT_OF_MEMORY:
      std::cerr <<
        ": There is not enough memory left to execute the command.The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
      break;
    case GL_STACK_UNDERFLOW:
      std::cerr <<
        ": An attempt has been made to perform an operation that would cause an internal stack to underflow.";
      break;
    case GL_STACK_OVERFLOW:
      std::cerr << ": An attempt has been made to perform an operation that would cause an internal stack to overflow.";
      break;
    }
    return true;
  }
}

void glDebugCallbackHandler(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg,
                            GLvoid* data)
{
  OutputDebugStringA(msg);
  std::cout << "debug call: " << msg << std::endl;
}

//////////////////////////////////////////////////////////////////////
//
// GLFW provides cross platform window creation
//

#include <GLFW/glfw3.h>

namespace glfw
{
  inline GLFWwindow* createWindow(const uvec2& size, const ivec2& position = ivec2(INT_MIN))
  {
    GLFWwindow* window = glfwCreateWindow(size.x, size.y, "glfw", nullptr, nullptr);
    if (!window)
    {
      FAIL("Unable to create rendering window");
    }
    if ((position.x > INT_MIN) && (position.y > INT_MIN))
    {
      glfwSetWindowPos(window, position.x, position.y);
    }
    return window;
  }
}

// A class to encapsulate using GLFW to handle input and render a scene
class GlfwApp
{
protected:
  uvec2 windowSize;
  ivec2 windowPosition;
  GLFWwindow* window{nullptr};
  unsigned int frame{0};

public:
  GlfwApp()
  {
    // Initialize the GLFW system for creating and positioning windows
    if (!glfwInit())
    {
      FAIL("Failed to initialize GLFW");
    }
    glfwSetErrorCallback(ErrorCallback);
  }

  virtual ~GlfwApp()
  {
    if (nullptr != window)
    {
      glfwDestroyWindow(window);
    }
    glfwTerminate();
  }

  virtual int run()
  {
    preCreate();

    window = createRenderingTarget(windowSize, windowPosition);

    if (!window)
    {
      std::cout << "Unable to create OpenGL window" << std::endl;
      return -1;
    }

    postCreate();

    initGl();

    while (!glfwWindowShouldClose(window))
    {
      ++frame;
      glfwPollEvents();
      update();
      draw();
      finishFrame();
    }

    shutdownGl();

    return 0;
  }

protected:
  virtual GLFWwindow* createRenderingTarget(uvec2& size, ivec2& pos) = 0;

  virtual void draw() = 0;

  void preCreate()
  {
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  }

  void postCreate()
  {
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwMakeContextCurrent(window);

    // Initialize the OpenGL bindings
    // For some reason we have to set this experminetal flag to properly
    // init GLEW if we use a core context.
    glewExperimental = GL_TRUE;
    if (0 != glewInit())
    {
      FAIL("Failed to initialize GLEW");
    }
    glGetError();

    if (GLEW_KHR_debug)
    {
      GLint v;
      glGetIntegerv(GL_CONTEXT_FLAGS, &v);
      if (v & GL_CONTEXT_FLAG_DEBUG_BIT)
      {
        //glDebugMessageCallback(glDebugCallbackHandler, this);
      }
    }
  }

  virtual void initGl()
  {
  }

  virtual void shutdownGl()
  {
  }

  virtual void finishFrame()
  {
    glfwSwapBuffers(window);
  }

  virtual void destroyWindow()
  {
    glfwSetKeyCallback(window, nullptr);
    glfwSetMouseButtonCallback(window, nullptr);
    glfwDestroyWindow(window);
  }

  virtual void onKey(int key, int scancode, int action, int mods)
  {
    if (GLFW_PRESS != action)
    {
      return;
    }

    switch (key)
    {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, 1);
      return;
    }
  }

  virtual void update()
  {
  }

  virtual void onMouseButton(int button, int action, int mods)
  {
  }

protected:
  virtual void viewport(const ivec2& pos, const uvec2& size)
  {
    glViewport(pos.x, pos.y, size.x, size.y);
  }

private:

  static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
  {
    GlfwApp* instance = (GlfwApp *)glfwGetWindowUserPointer(window);
    instance->onKey(key, scancode, action, mods);
  }

  static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
  {
    GlfwApp* instance = (GlfwApp *)glfwGetWindowUserPointer(window);
    instance->onMouseButton(button, action, mods);
  }

  static void ErrorCallback(int error, const char* description)
  {
    FAIL(description);
  }
};

//////////////////////////////////////////////////////////////////////
//
// The Oculus VR C API provides access to information about the HMD
//


#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

// Head Tracking Modes (BUTTON B)
enum HeadTrackingMode { REGULAR, ORIENTATION_ONLY, POSITION_ONLY, NO_TRACKING };
HeadTrackingMode headMode;

namespace ovr
{
  // Convenience method for looping over each eye with a lambda
  template <typename Function>
  inline void for_each_eye(Function function)
  {
    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
         eye < ovrEyeType::ovrEye_Count;
         eye = static_cast<ovrEyeType>(eye + 1))
    {
      function(eye);
    }
  }

  inline mat4 toGlm(const ovrMatrix4f& om)
  {
    return glm::transpose(glm::make_mat4(&om.M[0][0]));
  }

  inline mat4 toGlm(const ovrFovPort& fovport, float nearPlane = 0.01f, float farPlane = 10000.0f)
  {
    return toGlm(ovrMatrix4f_Projection(fovport, nearPlane, farPlane, true));
  }

  inline vec3 toGlm(const ovrVector3f& ov)
  {
    return glm::make_vec3(&ov.x);
  }

  inline vec2 toGlm(const ovrVector2f& ov)
  {
    return glm::make_vec2(&ov.x);
  }

  inline uvec2 toGlm(const ovrSizei& ov)
  {
    return uvec2(ov.w, ov.h);
  }

  inline quat toGlm(const ovrQuatf& oq)
  {
    return glm::make_quat(&oq.x);
  }

  inline mat4 toGlm(const ovrPosef& op)
  {
    mat4 orientation = glm::mat4_cast(toGlm(op.Orientation));
    mat4 translation = glm::translate(mat4(), ovr::toGlm(op.Position));
	
	switch (headMode) {
	case REGULAR:
		break;
	case ORIENTATION_ONLY:
		translation = glm::mat4(1.0f);
		break;
	case POSITION_ONLY:
		orientation = glm::mat4(1.0f);
		break;
	case NO_TRACKING:
		translation = glm::mat4(1.0f);
		orientation = glm::mat4(1.0f);
		break;
	}
    return translation * orientation;
	
  }

  inline ovrMatrix4f fromGlm(const mat4& m)
  {
    ovrMatrix4f result;
    mat4 transposed(glm::transpose(m));
    memcpy(result.M, &(transposed[0][0]), sizeof(float) * 16);
    return result;
  }

  inline ovrVector3f fromGlm(const vec3& v)
  {
    ovrVector3f result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
  }

  inline ovrVector2f fromGlm(const vec2& v)
  {
    ovrVector2f result;
    result.x = v.x;
    result.y = v.y;
    return result;
  }

  inline ovrSizei fromGlm(const uvec2& v)
  {
    ovrSizei result;
    result.w = v.x;
    result.h = v.y;
    return result;
  }

  inline ovrQuatf fromGlm(const quat& q)
  {
    ovrQuatf result;
    result.x = q.x;
    result.y = q.y;
    result.z = q.z;
    result.w = q.w;
    return result;
  }
}

#include <queue>
#include <glm/glm.hpp>

class RiftManagerApp
{
protected:
  ovrSession _session;
  ovrHmdDesc _hmdDesc;
  ovrGraphicsLuid _luid;

public:
  RiftManagerApp()
  {
    if (!OVR_SUCCESS(ovr_Create(&_session, &_luid)))
    {
      FAIL("Unable to create HMD session");
    }

    _hmdDesc = ovr_GetHmdDesc(_session);
  }

  ~RiftManagerApp()
  {
    ovr_Destroy(_session);
    _session = nullptr;
  }
};

// Hand Tracking
double displayMidpointSeconds;
ovrTrackingState trackState;
ovrInputState  inputState;

unsigned int handStatus[2]; //Hand Status
ovrPosef handPoses[2]; // Hand Poses
ovrVector3f handPosition[2];
ovrQuatf handRotation[2];

// Head Tracking
glm::mat4 HeadPose;

// Current Eye
int currEye;

// GAME MODE
enum GameMode { PREPARE, ON, END };
GameMode gameMode;

// Prepare Mode (BUTTON B, BUTTON A, Button X, Hand Trigger, Index Trigger)
// Button B
enum DirectionMode { NONE, UP, RIGHT, DOWN, LEFT };
DirectionMode directionMode;
// Button A
enum WarshipMode { A_Ship, B_Ship, C_Ship, S_Ship, P_Ship };
WarshipMode warshipMode;
// BUTTON X
enum SelectingMode { SELECTING, DONE };
SelectingMode selectingMode;

// On Mode (Button Y)
enum PlayerMode {MY, RIVAL};
PlayerMode playerMode;

// End Mode
enum ResultMode {WIN, LOSE};
ResultMode resultMode;

class RiftApp : public GlfwApp, public RiftManagerApp {
public:

private:
	GLuint _fbo{ 0 };
	GLuint _depthBuffer{ 0 };
	ovrTextureSwapChain _eyeTexture;

	GLuint _mirrorFbo{ 0 };
	ovrMirrorTexture _mirrorTexture;

	ovrEyeRenderDesc _eyeRenderDescs[2];

	mat4 _eyeProjections[2];

	ovrLayerEyeFov _sceneLayer;
	ovrViewScaleDesc _viewScaleDesc;

	uvec2 _renderTargetSize;
	uvec2 _mirrorSize;

public:
RiftApp() {
	using namespace ovr;
	_viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

	memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
	_sceneLayer.Header.Type = ovrLayerType_EyeFov;
	_sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

	ovr::for_each_eye([&](ovrEyeType eye)
	{
		ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _hmdDesc.DefaultEyeFov[eye]);
		ovrMatrix4f ovrPerspectiveProjection =
			ovrMatrix4f_Projection(erd.Fov, 0.01f, 1000.0f, ovrProjection_ClipRangeOpenGL);
		_eyeProjections[eye] = ovr::toGlm(ovrPerspectiveProjection);
		_viewScaleDesc.HmdToEyePose[eye] = erd.HmdToEyePose;

		ovrFovPort& fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
		auto eyeSize = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
		_sceneLayer.Viewport[eye].Size = eyeSize;
		_sceneLayer.Viewport[eye].Pos = { (int)_renderTargetSize.x, 0 };

		_renderTargetSize.y = std::max(_renderTargetSize.y, (uint32_t)eyeSize.h);
		_renderTargetSize.x += eyeSize.w;
	});
	// Make the on screen window 1/4 the resolution of the render target
	_mirrorSize = _renderTargetSize;
	_mirrorSize /= 4;
}

protected:
	GLFWwindow* createRenderingTarget(uvec2& outSize, ivec2& outPosition) override {
		return glfw::createWindow(_mirrorSize);
	}

	void initGl() override {
		GlfwApp::initGl();

		// Disable the v-sync for buffer swap
		glfwSwapInterval(0);

		ovrTextureSwapChainDesc desc = {};
		desc.Type = ovrTexture_2D;
		desc.ArraySize = 1;
		desc.Width = _renderTargetSize.x;
		desc.Height = _renderTargetSize.y;
		desc.MipLevels = 1;
		desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.SampleCount = 1;
		desc.StaticImage = ovrFalse;
		ovrResult result = ovr_CreateTextureSwapChainGL(_session, &desc, &_eyeTexture);
		_sceneLayer.ColorTexture[0] = _eyeTexture;
		if (!OVR_SUCCESS(result)) {
			FAIL("Failed to create swap textures");
		}

		int length = 0;
		result = ovr_GetTextureSwapChainLength(_session, _eyeTexture, &length);
		if (!OVR_SUCCESS(result) || !length) {
			FAIL("Unable to count swap chain textures");
		}
		for (int i = 0; i < length; ++i) {
			GLuint chainTexId;
			ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, i, &chainTexId);
			glBindTexture(GL_TEXTURE_2D, chainTexId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Set up the framebuffer object
		glGenFramebuffers(1, &_fbo);
		glGenRenderbuffers(1, &_depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _renderTargetSize.x, _renderTargetSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		ovrMirrorTextureDesc mirrorDesc;
		memset(&mirrorDesc, 0, sizeof(mirrorDesc));
		mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		mirrorDesc.Width = _mirrorSize.x;
		mirrorDesc.Height = _mirrorSize.y;
		if (!OVR_SUCCESS(ovr_CreateMirrorTextureGL(_session, &mirrorDesc, &_mirrorTexture))) {
			FAIL("Could not create mirror texture");
		}
		glGenFramebuffers(1, &_mirrorFbo);
	}

	void onKey(int key, int scancode, int action, int mods) override {
		if (GLFW_PRESS == action)
			switch (key) {
			case GLFW_KEY_R:
				ovr_RecenterTrackingOrigin(_session);
				return;
			}

		GlfwApp::onKey(key, scancode, action, mods);
	}

	void draw() final override {


		// HAND TRACKING
		displayMidpointSeconds = ovr_GetPredictedDisplayTime(_session, 0);
		trackState = ovr_GetTrackingState(_session, displayMidpointSeconds, ovrTrue);
		// Hand Status
		handStatus[0] = trackState.HandStatusFlags[0];
		handStatus[1] = trackState.HandStatusFlags[1];
		// Hand Poses
		handPoses[0] = trackState.HandPoses[0].ThePose;
		handPoses[1] = trackState.HandPoses[1].ThePose;
		// Hand Position
		handPosition[0] = handPoses[0].Position;
		handPosition[1] = handPoses[1].Position;
		// Hand Rotation
		handRotation[0] = handPoses[0].Orientation;
		handRotation[1] = handPoses[1].Orientation;

		// Eye
		ovrPosef eyePoses[2];
		ovr_GetEyePoses(_session, frame, true, _viewScaleDesc.HmdToEyePose, eyePoses, &_sceneLayer.SensorSampleTime);
		// Head
		HeadPose = ovr::toGlm(eyePoses[ovrEye_Left]) + ovr::toGlm(eyePoses[ovrEye_Right]);
		HeadPose = glm::scale(HeadPose, glm::vec3(0.5f));

		int curIndex;
		ovr_GetTextureSwapChainCurrentIndex(_session, _eyeTexture, &curIndex);
		GLuint curTexId;
		ovr_GetTextureSwapChainBufferGL(_session, _eyeTexture, curIndex, &curTexId);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ovr::for_each_eye([&](ovrEyeType eye)
		{
			if (eye == ovrEye_Left) {
				currEye = 0;
			}
			else {
				currEye = 1;
			}
			const auto& vp = _sceneLayer.Viewport[eye];
			glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
			_sceneLayer.RenderPose[eye] = eyePoses[eye];
			renderScene(_eyeProjections[eye], ovr::toGlm(eyePoses[eye]));
		});
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		ovr_CommitTextureSwapChain(_session, _eyeTexture);
		ovrLayerHeader* headerList = &_sceneLayer.Header;
		ovr_SubmitFrame(_session, frame, &_viewScaleDesc, &headerList, 1);

		GLuint mirrorTextureId;
		ovr_GetMirrorTextureBufferGL(_session, _mirrorTexture, &mirrorTextureId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _mirrorFbo);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureId, 0);
		glBlitFramebuffer(0, 0, _mirrorSize.x, _mirrorSize.y, 0, _mirrorSize.y, _mirrorSize.x, 0, GL_COLOR_BUFFER_BIT,
			GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	virtual void renderScene(const glm::mat4& projection, const glm::mat4& headPose) = 0;
};


//////////////////////////////////////////////////////////////////////
//
// The remainder of this code is specific to the scene we want to 
// render.  I use glfw to render an array of cubes, but your 
// application would perform whatever rendering you want
//


#include <vector>
#include <algorithm>
#include "shader.h"
#include "Cube.h"
#include<stdio.h> 
#include "Model.h"
#include "Mesh.h"
#include "Line.h"
#include <ctime>
#include <irrKlang.h>
#include <ft2build.h>
#include FT_FREETYPE_H 
#include <glm/glm.hpp>

//Sound
irrklang::ISoundEngine *soundEngine = irrklang::createIrrKlangDevice();

// a class for building and rendering cubes
class Scene
{

  // Grid Size
  const unsigned int GRID_SIZE{ 10 };

  // Skyboxs
  std::unique_ptr<Skybox> skybox_prepare;
  std::unique_ptr<Skybox> skybox_game;
  std::unique_ptr<Skybox> skybox_end;

  // Cubes
  // My Board
  std::unique_ptr<TexturedCube> my_board_cube_normal_odd;
  std::unique_ptr<TexturedCube> my_board_cube_normal_even;
  std::unique_ptr<TexturedCube> my_board_cube_occupied;
  std::unique_ptr<TexturedCube> my_board_cube_A;
  std::unique_ptr<TexturedCube> my_board_cube_B;
  std::unique_ptr<TexturedCube> my_board_cube_C;
  std::unique_ptr<TexturedCube> my_board_cube_S;
  std::unique_ptr<TexturedCube> my_board_cube_P;

  // Rival Board
  std::unique_ptr<TexturedCube> rival_board_cube_normal;
  std::unique_ptr<TexturedCube> rival_board_cube_selected;
  

  // Both
  std::unique_ptr<TexturedCube> board_cube_shooted;
  std::unique_ptr<TexturedCube> board_cube_missed;

  // NotePad
  std::unique_ptr<TexturedCube> notePad;

  std::vector<glm::vec3> my_board_positions;
  std::vector<glm::mat4> my_board_matrices;

  std::vector<glm::vec3> rival_board_positions;
  std::vector<glm::mat4> rival_board_matrices;

  // Lines
  std::unique_ptr<Line> line;


public:
	glm::mat4 LHOrientationPosition, RHOrientationPosition;

	int x_cord, y_cord;

	// Shader Program
	GLuint lineShaderID;
	GLuint skyboxShaderID;
	GLuint highlightShaderID;
	GLuint textShaderID;
	GLuint modelShaderID;
	
	// Warships
	std::vector<std::pair<int, int>> A; // Aircraft Carrier * 1 (5*1 grid)
	std::vector<std::pair<int, int>> B; // Battleship * 1 (4*1 grid)
	std::vector<std::pair<int, int>> C; // Cruiser * 1 (3*1 grid)
	std::vector<std::pair<int, int>> S; // Destroyer * 1 (2*1 grid)
	std::vector<std::pair<int, int>> P; // Patrol Boat * 1 (2*1 grid)

	// Game Data
	int myBoard[10][10];
	int rivalBoard[10][10];

	int selectedIdx;

	int numHits = 0;
	int numDamages = 0;

	const int EMPTY = 0;
	const int MARKED = 1;
	const int MISSED = -1;
	const int SHOOTED = 2;
	
	// Text Rendering
	string shipMessage, directionMessage, endMessage;

	FT_Library ft;
	FT_Face face;

	struct Character {
		GLuint     TextureID;  // ID handle of the glyph texture
		glm::ivec2 Size;       // Size of glyph
		glm::ivec2 Bearing;    // Offset from baseline to left/top of glyph
		GLuint     Advance;    // Offset to advance to next glyph
	};

	std::map<GLchar, Character> Characters;

	glm::mat4 projection = glm::ortho(0.0f, 800.0f, -100.0f, 600.0f);
	GLuint VAO, VBO;

  Scene()
  {
    // Shader Program 
    skyboxShaderID = LoadShaders("skybox.vert", "skybox.frag");
	lineShaderID = LoadShaders("line.vert", "line.frag");
	highlightShaderID = LoadShaders("highlight.vert", "highlight.frag");
	textShaderID = LoadShaders("text.vert", "text.frag");
	modelShaderID = LoadShaders("model.vert", "model.frag");

	// Skyboxs
	skybox_prepare = std::make_unique<Skybox>("skybox_prepare");
	skybox_prepare->toWorld = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));

	skybox_game = std::make_unique<Skybox>("skybox_game");
	skybox_game->toWorld = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));

	skybox_end = std::make_unique<Skybox>("skybox_end_winner");
	skybox_end->toWorld = glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));


	// PREPARE MODE

	// My Board Squares
	my_board_cube_normal_odd = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_Normal_Square_Odd");
	my_board_cube_normal_even = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_Normal_Square_Even");
	my_board_cube_occupied = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_Occupied_Square");
	my_board_cube_A = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_A_Square");
	my_board_cube_B = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_B_Square");
	my_board_cube_C = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_C_Square");
	my_board_cube_S = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_S_Square");
	my_board_cube_P = std::make_unique<TexturedCube>("Asset/My_Board/My_Board_P_Square");
	
	// Rival Board Squares
	rival_board_cube_normal = std::make_unique<TexturedCube>("Asset/Rival_Board/Rival_Board_Normal_Square");
	rival_board_cube_selected = std::make_unique<TexturedCube>("Asset/Rival_Board/Rival_Board_Selected_Square");

	// Both
	board_cube_missed = std::make_unique<TexturedCube>("Asset/Rival_Board/Rival_Board_Missed_Square");
	board_cube_shooted = std::make_unique<TexturedCube>("Asset/Rival_Board/Rival_Board_Shooted_Square");

	// NotePad
	notePad = std::make_unique<TexturedCube>("Asset/NotePad");

	// 10*10 Board Cube Positions 
	for (unsigned int z = 0; z < GRID_SIZE; ++z) {
		for (unsigned int x = 0; x < GRID_SIZE; ++x) {

			// My Board Position
			float xpos = -1.0f + 0.2f * x;
			float ypos = -1.0f;
			float zpos = -1.0f + 0.2f * z;
			glm::vec3 relativePosition = glm::vec3(xpos, ypos, zpos);
			my_board_positions.push_back(relativePosition);
			my_board_matrices.push_back(glm::translate(glm::mat4(1.0f), relativePosition));

			// Rival Board Position
			xpos = 0.04f * x;
			ypos = 0.0f;
			zpos = 0.04f * z;
			relativePosition = glm::vec3(xpos, ypos, zpos);
			rival_board_positions.push_back(relativePosition);
			rival_board_matrices.push_back(glm::translate(glm::mat4(1.0f), relativePosition));
		}
	}

	// Line
	line = std::make_unique<Line>();

	// Message
	shipMessage = "";
	directionMessage = "";
	endMessage = "";

	// Rendering Text
	if (FT_Init_FreeType(&ft))
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
	if (FT_New_Face(ft, "fonts/arial.ttf", 0, &face))
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
	FT_Set_Pixel_Sizes(face, 0, 48);
	if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
		std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment restriction

	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}
		// Generate texture
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// Now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			face->glyph->advance.x
		};
		Characters.insert(std::pair<GLchar, Character>(c, character));
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
  }

  void RenderText( GLuint shaderID, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
  {
	  // Activate corresponding render state	
	  glUseProgram(shaderID);
	  
	  glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, false, (float*)&projection);
	  glUniform3f(glGetUniformLocation(shaderID, "textColor"), color.x, color.y, color.z);

	  glActiveTexture(GL_TEXTURE0);
	  glBindVertexArray(VAO);
	  
	 
	  // Iterate through all characters
	  std::string::const_iterator c;
	  for (c = text.begin(); c != text.end(); c++)
	  {
		  Character ch = Characters[*c];

		  GLfloat xpos = x + ch.Bearing.x * scale;
		  GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		  GLfloat w = ch.Size.x * scale;
		  GLfloat h = ch.Size.y * scale;
		  // Update VBO for each character
		  GLfloat vertices[6][4] = {
			  { xpos,     ypos + h,   0.0, 0.0 },
			  { xpos,     ypos,       0.0, 1.0 },
			  { xpos + w, ypos,       1.0, 1.0 },

			  { xpos,     ypos + h,   0.0, 0.0 },
			  { xpos + w, ypos,       1.0, 1.0 },
			  { xpos + w, ypos + h,   1.0, 0.0 }
		  };
		  // Render glyph texture over quad
		  glBindTexture(GL_TEXTURE_2D, ch.TextureID);
		  // Update content of VBO memory
		  glBindBuffer(GL_ARRAY_BUFFER, VBO);
		  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		  glBindBuffer(GL_ARRAY_BUFFER, 0);
		  // Render quad
		  glDrawArrays(GL_TRIANGLES, 0, 6);
		  // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		  x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	  }
	  glBindVertexArray(0);
	  glBindTexture(GL_TEXTURE_2D, 0);
  }

  void render(const glm::mat4& projection, const glm::mat4& view)
  {
	  // Left Hand Position and Orientation
	  // Translation
	  glm::mat4 translate = glm::translate(glm::mat4(1.0f), vec3(handPosition[ovrHand_Right].x, handPosition[ovrHand_Right].y, handPosition[ovrHand_Right].z));
	  // Rotation
	  float x = handRotation[ovrHand_Right].x;
	  float y = handRotation[ovrHand_Right].y;
	  float z = handRotation[ovrHand_Right].z;
	  float w = handRotation[ovrHand_Right].w;
	  // Quaternion to Rotation Matrix
	  glm::mat4 rotate = glm::mat4(1 - 2 * y*y - 2 * z*z, 2 * x*y + 2 * w*z, 2 * x*z - 2 * w*y, 0.0f,
		  2 * x*y - 2 * w*z, 1 - 2 * x*x - 2 * z*z, 2 * y*z + 2 * w*x, 0.0f,
		  2 * x*z + 2 * w*y, 2 * y*z - 2 * w*x, 1 - 2 * x*x - 2 * y*y, 0.0f,
		  0.0f, 0.0f, 0.0f, 1.0f);
	  // Left-Hand orientation and position
	  RHOrientationPosition = translate * rotate;

	  // Left Hand Position and Orientation
	  // Translation
	  translate = glm::translate(glm::mat4(1.0f), vec3(handPosition[ovrHand_Left].x, handPosition[ovrHand_Left].y, handPosition[ovrHand_Left].z));
	  // Rotation
	  x = handRotation[ovrHand_Left].x;
	  y = handRotation[ovrHand_Left].y;
	  z = handRotation[ovrHand_Left].z;
	  w = handRotation[ovrHand_Left].w;
	  // Quaternion to Rotation Matrix
	  rotate = glm::mat4(1 - 2 * y*y - 2 * z*z, 2 * x*y + 2 * w*z, 2 * x*z - 2 * w*y, 0.0f,
		  2 * x*y - 2 * w*z, 1 - 2 * x*x - 2 * z*z, 2 * y*z + 2 * w*x, 0.0f,
		  2 * x*z + 2 * w*y, 2 * y*z - 2 * w*x, 1 - 2 * x*x - 2 * y*y, 0.0f,
		  0.0f, 0.0f, 0.0f, 1.0f);
	  // Left-Hand orientation and position
	  LHOrientationPosition = translate * rotate;


	// Prepare Mode
	if (gameMode == PREPARE) {

		// Skybox
		skybox_prepare->draw(skyboxShaderID, projection, view);

		// NotePad
		
		notePad->toWorld = RHOrientationPosition * rival_board_matrices[0] * glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.001f, 0.078f));
		notePad->draw(skyboxShaderID, projection, view);

		// current selected grid
		int row, column;
		std::pair<int, int> curr;
		for (int i = 0; i < GRID_SIZE*GRID_SIZE; i++)
		{
			row = i % GRID_SIZE;
			column = (i - row) / GRID_SIZE;
			curr = std::make_pair(row, column);

			if (column == y_cord && row == x_cord && selectingMode == SELECTING) {
				if (warshipMode == A_Ship) {
					my_board_cube_A->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_A->draw(skyboxShaderID, projection, view);
				}
				else if (warshipMode == B_Ship) {
					my_board_cube_B->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_B->draw(skyboxShaderID, projection, view);
				}
				else if (warshipMode == C_Ship) {
					my_board_cube_C->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_C->draw(skyboxShaderID, projection, view);
				}
				else if (warshipMode == S_Ship) {
					my_board_cube_S->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_S->draw(skyboxShaderID, projection, view);
				}
				else if (warshipMode == P_Ship) {
					my_board_cube_P->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_P->draw(skyboxShaderID, projection, view);
				}
			}
			else if (std::find(A.begin(), A.end(), curr) != A.end() || std::find(B.begin(), B.end(), curr) != B.end() ||
				std::find(C.begin(), C.end(), curr) != C.end() || std::find(S.begin(), S.end(), curr) != S.end() ||
				std::find(P.begin(), P.end(), curr) != P.end()) {
				my_board_cube_occupied->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
				my_board_cube_occupied->draw(skyboxShaderID, projection, view);
			}
			else {
				if (row % 2 == 0) {
					if (column % 2 == 0) {
						my_board_cube_normal_odd->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_odd->draw(skyboxShaderID, projection, view);
					}
					else {
						my_board_cube_normal_even->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_even->draw(skyboxShaderID, projection, view);
					}
				}
				else {
					if (column % 2 == 1) {
						my_board_cube_normal_odd->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_odd->draw(skyboxShaderID, projection, view);
					}
					else {
						my_board_cube_normal_even->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_even->draw(skyboxShaderID, projection, view);
					}
				}
			}
		}
	}


	if (gameMode == ON) {

		// Skybox
		skybox_game->draw(skyboxShaderID, projection, view);

		// Line
		float minDistSq = FLT_MAX;
		float currDistSq;
		glm::vec3 endPos;
		

		for (int i = 0; i < GRID_SIZE*GRID_SIZE; i++)
		{
			glm::mat4 currToWorld = LHOrientationPosition * rival_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.005f, 0.02f));
			glm::vec3 currPos = currToWorld[3];

			currDistSq = (currPos.x - handPosition[ovrHand_Right].x)*(currPos.x - handPosition[ovrHand_Right].x) +
				(currPos.y - handPosition[ovrHand_Right].y)*(currPos.y - handPosition[ovrHand_Right].y) +
				(currPos.z - handPosition[ovrHand_Right].z)*(currPos.z - handPosition[ovrHand_Right].z);

			if (currDistSq < minDistSq) {
				minDistSq = currDistSq;
				selectedIdx = i;
				endPos = glm::vec3(currPos.x, currPos.y, currPos.z);
			}
		}

		x_cord = selectedIdx % GRID_SIZE;
		y_cord = (selectedIdx - x_cord) / GRID_SIZE;

		// Update Line
		glUseProgram(lineShaderID);
		line->color = glm::vec3(1.0f, 0.95f, 0.31f);
		line->update(vec3(handPosition[ovrHand_Right].x, handPosition[ovrHand_Right].y, handPosition[ovrHand_Right].z), endPos);
		line->draw(lineShaderID, projection, view);

		
		// Board
		int row, column;
		std::pair<int, int> curr;
		for (int i = 0; i < GRID_SIZE*GRID_SIZE; i++)
		{
			row = i % GRID_SIZE;
			column = (i - row) / GRID_SIZE;
			curr = std::make_pair(row, column);

			// My Board
			if (myBoard[row][column] == EMPTY) {
				if (row % 2 == 0) {
					if (column % 2 == 0) {
						my_board_cube_normal_odd->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_odd->draw(skyboxShaderID, projection, view);
					}
					else {
						my_board_cube_normal_even->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_even->draw(skyboxShaderID, projection, view);
					}
				}
				else {
					if (column % 2 == 1) {
						my_board_cube_normal_odd->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_odd->draw(skyboxShaderID, projection, view);
					}
					else {
						my_board_cube_normal_even->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
						my_board_cube_normal_even->draw(skyboxShaderID, projection, view);
					}
				}
			}
			else if (myBoard[row][column] == MARKED) {
				if (std::find(A.begin(), A.end(), curr) != A.end()) {
					my_board_cube_A->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_A->draw(skyboxShaderID, projection, view);
				}
				else if (std::find(B.begin(), B.end(), curr) != B.end()) {
					my_board_cube_B->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_B->draw(skyboxShaderID, projection, view);
				}
				else if (std::find(C.begin(), C.end(), curr) != C.end()) {
					my_board_cube_C->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_C->draw(skyboxShaderID, projection, view);
				}
				else if (std::find(S.begin(), S.end(), curr) != S.end()) {
					my_board_cube_S->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_S->draw(skyboxShaderID, projection, view);
				}
				else if (std::find(P.begin(), P.end(), curr) != P.end()) {
					my_board_cube_P->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
					my_board_cube_P->draw(skyboxShaderID, projection, view);
				}
			}
			else if (myBoard[row][column] == MISSED) {
				board_cube_missed->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
				board_cube_missed->draw(skyboxShaderID, projection, view);
			}
			else if (myBoard[row][column] == SHOOTED) {
				board_cube_shooted->toWorld = my_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.005f, 0.1f));
				board_cube_shooted->draw(skyboxShaderID, projection, view);
			}

			
			// Rival Board
			if (rivalBoard[row][column] == EMPTY) {
				if (i == selectedIdx && playerMode == MY) {
					rival_board_cube_selected->toWorld = LHOrientationPosition * rival_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.005f, 0.02f));
					rival_board_cube_selected->draw(skyboxShaderID, projection, view);
				}
				else {
					rival_board_cube_normal->toWorld = LHOrientationPosition * rival_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.005f, 0.02f));
					rival_board_cube_normal->draw(skyboxShaderID, projection, view);
				}
			}
			else if (rivalBoard[row][column] == MISSED) {
				board_cube_missed->toWorld = LHOrientationPosition * rival_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.005f, 0.02f));
				board_cube_missed->draw(skyboxShaderID, projection, view);
			}
			else if (rivalBoard[row][column] == SHOOTED) {
				board_cube_shooted->toWorld = LHOrientationPosition * rival_board_matrices[i] * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f, 0.005f, 0.02f));
				board_cube_shooted->draw(skyboxShaderID, projection, view);
			}

		}
	}


	if (gameMode == END) {
	
		if (resultMode == WIN) {
			skybox_end->draw(skyboxShaderID, projection, view);
			endMessage = "Congraduations! You Have Won the Battle.";
		}

		if (resultMode == LOSE) {
			skybox_prepare->draw(skyboxShaderID, projection, view);
			endMessage = "Unfortunately You Just Lost the Battle.";
		}
	}

	
	if (currEye == 0) {
		if (gameMode == PREPARE) {
			RenderText(textShaderID, directionMessage, 75.0f, 150.0f, 1.0f, glm::vec3(1.0f, 0.95f, 0.31f));
			RenderText(textShaderID, shipMessage, 75.0f, 250.0f, 1.5f, glm::vec3(1.0f, 0.95f, 0.31f));
		}
		if (gameMode == END) {
			RenderText(textShaderID, endMessage, 150.0f, 250.0f, 0.6f, glm::vec3(1.0f, 0.0f, 0.0f));
			RenderText(textShaderID, "Touch Sculpture to Start Over", 175.0f, 150.0f, 0.6f, glm::vec3(1.0f, 0.0f, 0.0f));
		}
	}

	
  }

  // Helper Methods (PREPARE)
  void placeWarship(std::vector<std::pair<int, int>> & currWarship, unsigned int size) {
	  switch (directionMode) {
	  case NONE:
		  directionMessage = "";
		  return;
	  case UP:
		  if (y_cord >= (size - 1)) {
			  currWarship.clear();
			  directionMessage = "";
			  for (unsigned int i = 0; i < size; i++) {
				  if (checkAvailability(std::make_pair(x_cord, y_cord - i))) {
					  currWarship.push_back(std::make_pair(x_cord, y_cord - i));
				  }
				  else {
					  currWarship.clear();
					  directionMessage = "Upward Unavailable";
					  break;
				  }
			  }
		  }
		  else {
			  directionMessage = "Upward Unavailable";
		  }
		  break;
	  case RIGHT:
		  if ((9 - x_cord) >= (size - 1)) {
			  currWarship.clear();
			  directionMessage = "";
			  for (unsigned int i = 0; i < size; i++) {
				  if (checkAvailability(std::make_pair(x_cord + i, y_cord))) {
					  currWarship.push_back(std::make_pair(x_cord + i, y_cord));
				  }
				  else {
					  currWarship.clear();
					  directionMessage = "Rightward Unavailable";
					  break;
				  }
			  }
		  }
		  else {
			  directionMessage = "Rightward Unavailable";
		  }
		  break;
	  case DOWN:
		  if ((9 - y_cord) >= (size - 1)) {
			  directionMessage = "";
			  currWarship.clear();
			  for (unsigned int i = 0; i < size; i++) {
				  if (checkAvailability(std::make_pair(x_cord, y_cord + i))) {
					  currWarship.push_back(std::make_pair(x_cord, y_cord + i));
				  }
				  else {
					  currWarship.clear();
					  directionMessage = "Downward Unavailable";
					  break;
				  }
			  }
		  }
		  else {
			  directionMessage = "Downward Unavailable";
		  }
		  break;
	  case LEFT:
		  if (x_cord >= (size - 1)) {
			  directionMessage = "";
			  currWarship.clear();
			  for (unsigned int i = 0; i < size; i++) {
				  if (checkAvailability(std::make_pair(x_cord - i, y_cord))) {
					  currWarship.push_back(std::make_pair(x_cord - i, y_cord));
				  }
				  else {
					  currWarship.clear();
					  directionMessage = "Leftward Unavailable";
					  break;
				  }
			  }
		  }
		  else {
			  directionMessage = "Leftward Unavailable";
		  }
		  break;
	  }
  }

  // Helper Methods (PREPARE and ON)
  bool checkAvailability(std::pair<int, int> curr) {
	  if (std::find(A.begin(), A.end(), curr) != A.end() || std::find(B.begin(), B.end(), curr) != B.end() ||
		  std::find(C.begin(), C.end(), curr) != C.end() || std::find(S.begin(), S.end(), curr) != S.end() ||
		  std::find(P.begin(), P.end(), curr) != P.end()) {
		  return false;
	  }
	  return true;
  }

  // Reset
  void reset() {
	  A.clear();
	  B.clear();
	  C.clear();
	  P.clear();
	  S.clear();
	  my_board_matrices.clear();
	  rival_board_matrices.clear();
	  my_board_positions.clear();
	  rival_board_positions.clear();
	  for (int x = 0; x < GRID_SIZE; x++) {
		  for (int y = 0; y < GRID_SIZE; y++) {
			  myBoard[x][y] = EMPTY;
			  rivalBoard[x][y] = EMPTY;
		  }
	  }

	  selectedIdx = 0;
	  numHits = 0;
	  numDamages = 0;
  }

};

// Lights
glm::vec3 pointLightColors[] = {
	glm::vec3(1.0f, 1.0, 0.0),
};

class Object {

	// Shader ID
	GLuint modelShaderID;
	GLuint textureShaderID;

	// Cursor
	std::unique_ptr<Model> model;

public:
	// Position
	glm::mat4 toWorld;

public:
	Object(string const &path) {
		modelShaderID = LoadShaders("model_Phong.vert", "model_Phong.frag");
		textureShaderID = LoadShaders("model.vert", "model.frag");
		model = std::make_unique<Model>(path);
	}

	/* Render sphere at User's Dominant Hand's Controller Position */
	void render(const glm::mat4& projection, const glm::mat4& view, bool isLight) {
		if (isLight) {
			glUseProgram(modelShaderID);
			glUniform1f(glGetUniformLocation(modelShaderID, "material.shininess"), 0.7f);

			glUniform3f(glGetUniformLocation(modelShaderID, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
			glUniform3f(glGetUniformLocation(modelShaderID, "dirLight.ambient"), 0.3f, 0.24f, 0.14f);
			glUniform3f(glGetUniformLocation(modelShaderID, "dirLight.diffuse"), 0.7f, 0.42f, 0.26f);
			glUniform3f(glGetUniformLocation(modelShaderID, "dirLight.specular"), 0.5f, 0.5f, 0.5f);

			glm::vec3 pointLightPosition;
			pointLightPosition = glm::vec3(0.0f, 1.0f, 0.0f);
			glUniform3f(glGetUniformLocation(modelShaderID, "pointLights[0].position"), pointLightPosition.x, pointLightPosition.y, pointLightPosition.z);
			glUniform3f(glGetUniformLocation(modelShaderID, "pointLights[0].ambient"), pointLightColors[0].x , pointLightColors[0].y , pointLightColors[0].z );
			glUniform3f(glGetUniformLocation(modelShaderID, "pointLights[0].diffuse"), pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
			glUniform3f(glGetUniformLocation(modelShaderID, "pointLights[0].specular"), pointLightColors[0].x, pointLightColors[0].y, pointLightColors[0].z);
			glUniform1f(glGetUniformLocation(modelShaderID, "pointLights[0].constant"), 1.0f);
			glUniform1f(glGetUniformLocation(modelShaderID, "pointLights[0].linear"), 0.09f);
			glUniform1f(glGetUniformLocation(modelShaderID, "pointLights[0].quadratic"), 0.032f);

			model->Draw(modelShaderID, projection, view, toWorld);
		}
		else {
			glUseProgram(textureShaderID);
			model->Draw(textureShaderID, projection, view, toWorld);
		}

	}

};


// An example application that renders a simple cube
class ExampleApp : public RiftApp
{
  // Scene
  std::unique_ptr<Scene> scene;

  // Server
  std::unique_ptr<ServerGame> server;

  // Buttons 
  bool buttonXPressed;
  bool buttonAPressed;
  bool buttonBPressed;
  bool buttonYPressed;

  // Model

  std::unique_ptr<Object> otherHead;
  std::unique_ptr<Object> sculpture;
  std::unique_ptr<Object> rose;

  // Triggers
  bool triggerLeftIndexClicked;
  bool triggerRightIndexClicked;
  bool triggerLeftMiddleClicked;
  bool triggerRightMiddleClicked;

public:

  ExampleApp(){}

protected:
  void initGl() override
  {
    RiftApp::initGl();
    glClearColor(0.9f, 0.9f, 0.9f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    ovr_RecenterTrackingOrigin(_session);

	// Scene
    scene = std::unique_ptr<Scene>(new Scene());
	// Server
	server = std::unique_ptr<ServerGame>(new ServerGame());

	// Model
	rose = std::unique_ptr<Object>(new Object("Asset/Model/Rose/rose.obj"));
	sculpture = std::unique_ptr<Object>(new Object("Asset/Model/Sculpture/Handler.obj"));
	otherHead = std::unique_ptr<Object>(new Object("Asset/Model/VMask/VMask.obj"));
	
	// Modes Initialization
	gameMode = PREPARE;
	playerMode = MY;

	// PREPARE
	directionMode = NONE;
	warshipMode = A_Ship;
	selectingMode = SELECTING;
	scene->x_cord = 0;
	scene->y_cord = 0;

	// Buttons
	buttonXPressed = false;
	buttonAPressed = false;
	buttonBPressed = false;
	buttonYPressed = false;

	// Triggers
	triggerLeftIndexClicked = false;
	triggerRightIndexClicked = false;
	triggerLeftMiddleClicked = false;
	triggerRightMiddleClicked = false;

	// Message
	scene->shipMessage = "Place A Ship";
	scene->directionMessage = "";

  }

  void shutdownGl() override
  {
    scene->reset();
  }

  void renderScene(const glm::mat4& projection, const glm::mat4& headPose) override
  {
	 // Model Positions
	  rose->toWorld = scene->RHOrientationPosition * glm::scale(glm::mat4(1.0f), glm::vec3(0.004f));
	  sculpture->toWorld = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.0f, -0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.002f));
	

	  if (scene->numHits == 16) {
		  scene->numHits = 0;
		  soundEngine->play2D("../audio/End.mp3", GL_FALSE); // Audio
		  gameMode = END;
		  resultMode = WIN;
	  }

	  if (scene->numDamages == 16) {
		  scene->numDamages = 0;
		  soundEngine->play2D("../audio/End.mp3", GL_FALSE); // Audio
		  gameMode = END;
		  resultMode = LOSE;
	  }

	  if (server->other_done && server->my_done && gameMode == PREPARE) {
		  soundEngine->play2D("../audio/Start.mp3", GL_FALSE); // Audio
		  gameMode = ON;
	  }

	  // PREPARE Mode
	  if (gameMode == PREPARE) {

		  if (selectingMode == SELECTING) {
			  if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState))) {

				  // The LEFT INDEX trigger reduces the X_Coordinate of Selected Grid
				  if (inputState.IndexTrigger[ovrHand_Left] > 0.5f)
				  {
					  if (!triggerLeftIndexClicked) {
						  triggerLeftIndexClicked = true;
						  if (scene->x_cord > 0) {
							  scene->x_cord--;
							  ResetCurrentWarshipPosition();
						  }
					  }
				  }
				  else { triggerLeftIndexClicked = false; }

				  // The RIGHT INDEX trigger increases the X_Coordinate of Selected Grid
				  if (inputState.IndexTrigger[ovrHand_Right] > 0.5f)
				  {
					  if (!triggerRightIndexClicked) {
						  triggerRightIndexClicked = true;
						  if (scene->x_cord < 9) {
							  scene->x_cord++;
							  ResetCurrentWarshipPosition();
						  }
					  }
				  }
				  else { triggerRightIndexClicked = false; }

				  // The LEFT MIDDLE finger trigger reduces the Y_Coordinate of Selected Grid
				  if (inputState.HandTrigger[ovrHand_Left] > 0.5f)
				  {
					  if (!triggerLeftMiddleClicked) {
						  triggerLeftMiddleClicked = true;
						  if (scene->y_cord > 0) {
							  scene->y_cord--;
							  ResetCurrentWarshipPosition();
						  }
					  }
				  }
				  else { triggerLeftMiddleClicked = false; }

				  // The RIGHT MIDDLE finger trigger increases the Y_Coordinate of Selected Grid
				  if (inputState.HandTrigger[ovrHand_Right] > 0.5f)
				  {
					  if (!triggerRightMiddleClicked) {
						  triggerRightMiddleClicked = true;
						  if (scene->y_cord < 9) {
							  scene->y_cord++;
							  ResetCurrentWarshipPosition();
						  }
					  }
				  }
				  else { triggerRightMiddleClicked = false; }

				  // Button A
				  if (inputState.Buttons & ovrButton_A)
				  {
					  if (!buttonAPressed) {
						  buttonAPressed = true;
						  UpdateButtonA();
					  }
				  }
				  else { buttonAPressed = false; }

				  // Button B
				  if (inputState.Buttons & ovrButton_B)
				  {
					  if (!buttonBPressed) {
						  buttonBPressed = true;
						  UpdateButtonB();
					  }
				  }
				  else { buttonBPressed = false; }


				  // Button X
				  if (inputState.Buttons & ovrButton_X)
				  {
					  if (!buttonXPressed) {
						  buttonXPressed = true;
						  UpdateButtonX();
					  }
				  }
				  else { buttonXPressed = false; }

				  UpdateWarshipPosition();
			  }
		  }

	  }

	  if ((gameMode == PREPARE && selectingMode == DONE) || gameMode == ON) {
		  if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState))) {
			  if (inputState.Buttons & ovrButton_A) {
				  buttonAPressed = true;
			  }
			  else if (buttonAPressed) {
				  buttonAPressed = false;
			  }
		  }

		  if (buttonAPressed) {
			  RenderWarship();
		  }
	  }


	  if (gameMode == ON) {
		  if (OVR_SUCCESS(ovr_GetInputState(_session, ovrControllerType_Touch, &inputState))) {
			  if (inputState.Buttons & ovrButton_Y)
			  {
				  if (!buttonYPressed) {
					  buttonYPressed = true;
					  UpdateButtonY();
				  }
			  }
			  else {
				  buttonYPressed = false;
			  }
		  }
		  rose->render(projection, glm::inverse(headPose),true);
	  }

	  if (gameMode == END) {
		  rose->render(projection, glm::inverse(headPose), true);
		  sculpture->render(projection, glm::inverse(headPose), true);
		  float dist = sqrt(handPosition[ovrHand_Right].x * handPosition[ovrHand_Right].x + handPosition[ovrHand_Right].y * handPosition[ovrHand_Right].y
			  + (handPosition[ovrHand_Right].z + 0.5f) * (handPosition[ovrHand_Right].z + 0.5f));
		  if (dist < 0.09f) {

			  scene->reset();

			  gameMode = PREPARE;
			  playerMode = MY;
			
			  directionMode = NONE;
			  warshipMode = A_Ship;
			  selectingMode = SELECTING;
			  scene->x_cord = 0;
			  scene->y_cord = 0;

			  buttonXPressed = false;
			  buttonAPressed = false;
			  buttonBPressed = false;
			  buttonYPressed = false;

			  triggerLeftIndexClicked = false;
			  triggerRightIndexClicked = false;
			  triggerLeftMiddleClicked = false;
			  triggerRightMiddleClicked = false;

			  scene->shipMessage = "Place A Ship";
			  scene->directionMessage = "";
		  }
	 }

	  // Render Scene
      scene->render(projection, glm::inverse(headPose));
	  

	  // Update Server
	  server->update();
	
	  if (server->other_attack.first != -1) {
		  if (scene->myBoard[server->other_attack.first][server->other_attack.second] == scene->MARKED) {
			  scene->myBoard[server->other_attack.first][server->other_attack.second] = scene->SHOOTED;
			  server->my_damage.first = server->other_attack.first;
			  server->my_damage.second = server->other_attack.second;
			  server->other_attack.first = -1;
			  server->other_attack.second = -1;
			  scene->numDamages++;
		  }
		  else {
			  scene->myBoard[server->other_attack.first][server->other_attack.second] = scene->MISSED;
		  }
	  }
	  
	  
	  if (server->other_damage.first != -1) {
		  scene->rivalBoard[server->other_damage.first][server->other_damage.second] = scene->SHOOTED;
		  server->other_damage.first = -1;
		  server->other_damage.first = -1;
		  scene->numHits++;
	  }
	  

	  if (server->game_mode) {
		  playerMode = MY;
	  }

	 
	  server->my_headPose = HeadPose;
	  otherHead->toWorld = server->other_headPose * glm::scale(glm::mat4(1.0f), glm::vec3(0.02f));
	  otherHead->render(projection, glm::inverse(headPose), true);
	 
  }

  

  void UpdateButtonX() {
	  
	  if (selectingMode == SELECTING && gameMode == PREPARE) { 
		  if (scene->A.size() == 5 && scene->B.size() == 4 && scene->C.size() == 3 && scene->S.size() == 2 && scene->P.size() == 2) {
			  selectingMode = DONE;
			  buttonAPressed = false;

			  int row, column;
			  std::pair<int, int> curr;
			  for (int i = 0; i < 100; i++)
			  {
				  row = i % 10;
				  column = (i - row) / 10;
				  curr = std::make_pair(row, column);
				  if (std::find(scene->A.begin(), scene->A.end(), curr) != scene->A.end() || std::find(scene->B.begin(), scene->B.end(), curr) != scene->B.end() ||
					  std::find(scene->C.begin(), scene->C.end(), curr) != scene->C.end() || std::find(scene->S.begin(), scene->S.end(), curr) != scene->S.end() ||
					  std::find(scene->P.begin(), scene->P.end(), curr) != scene->P.end()) {
					  scene->myBoard[row][column] = scene->MARKED;
				  }
			  }

			  scene->shipMessage = "All Warships Ready";

			  server->my_done = true;
		  }
		  else {
			  scene->shipMessage = "";
		  }
	  }
  }

  void UpdateButtonA() {
	  if (warshipMode == A_Ship) { warshipMode = B_Ship; scene->shipMessage = "Place B Ship"; }
	  else if (warshipMode == B_Ship) { warshipMode = C_Ship; scene->shipMessage = "Place C Ship"; }
	  else if (warshipMode == C_Ship) { warshipMode = S_Ship; scene->shipMessage = "Place S Ship"; }
	  else if (warshipMode == S_Ship) { warshipMode = P_Ship; scene->shipMessage = "Place P Ship";}
	  else if (warshipMode == P_Ship) { warshipMode = A_Ship; scene->shipMessage = "Place A Ship";}

	  std::pair<int, int> newPos = RandomNewPosition();
	  scene->x_cord = newPos.first;
	  scene->y_cord = newPos.second;
  }

  void UpdateButtonB() {
	  if (directionMode == NONE) { directionMode = UP;  }
	  else if (directionMode == UP) { directionMode = RIGHT; }
	  else if (directionMode == RIGHT) { directionMode = DOWN;  }
	  else if (directionMode == DOWN) { directionMode = LEFT; }
	  else if (directionMode == LEFT) { directionMode = UP;  }

	  UpdateWarshipPosition();
  }

  void UpdateButtonY() {
	  if (scene->rivalBoard[scene->x_cord][scene->y_cord] == 0 && playerMode == MY) {
		  soundEngine->play2D("../audio/Fire.mp3", GL_FALSE); // Audio
		  playerMode = RIVAL;
		  server->game_mode = false;
		  server->my_attack.first = scene->x_cord;
		  server->my_attack.second = scene->y_cord;
		  scene->rivalBoard[scene->x_cord][scene->y_cord] = scene->MISSED;
	  }
  }

  // Helper Methods (PREPARE)
  void UpdateWarshipPosition() {
	  switch (warshipMode) {
	  case A_Ship:
		  scene->placeWarship(scene->A, 5);
		  break;
	  case B_Ship:
		  scene->placeWarship(scene->B, 4);
		  break;
	  case C_Ship:
		  scene->placeWarship(scene->C, 3);
		  break;
	  case S_Ship:
		  scene->placeWarship(scene->S, 2);
		  break;
	  case P_Ship:
		  scene->placeWarship(scene->P, 2);
		  break;
	  }
  }

  std::pair<int, int> RandomNewPosition() {
	  srand(time(NULL));
	  int rand_y = rand() % 10;
	  int rand_x = rand() % 10;
	  std::pair<int, int> newPos = std::make_pair(rand_x, rand_y);
	  while (!scene->checkAvailability(newPos)) {
		  srand(time(NULL));
		  rand_y = rand() % 10;
		  rand_x = rand() % 10;
		  newPos = std::make_pair(rand_x, rand_y);
	  }

	  return newPos;
  }

  void ResetCurrentWarshipPosition() {
	  switch (warshipMode) {
	  case A_Ship:
		  scene->A.clear();
		  break;
	  case B_Ship:
		  scene->B.clear();
		  break;
	  case C_Ship:
		  scene->C.clear();
		  break;
	  case S_Ship:
		  scene->S.clear();
		  break;
	  case P_Ship:
		  scene->P.clear();
		  break;
	  }
  }

  void RenderWarship() {

  }
};

// Execute our example class
int main(int argc, char** argv)
{
  int result = -1;

  if (!OVR_SUCCESS(ovr_Initialize(nullptr)))
  {
    FAIL("Failed to initialize the Oculus SDK");
  }
  result = ExampleApp().run();

  ovr_Shutdown();
  return result;
}
