#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

// from hellovr_opengl_main.cpp
#include <Windows.h>
#include <openvr.h>
#include "Matrices.h"
#include <stdarg.h>
#include <debugapi.h>
#include "pathtools.h"
#include "lodepng.h"

// TODO: visuals are using SDL, need to figure out cinder-solution

using namespace ci;
using namespace ci::app;
using namespace std;

void threadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}

// From hellovr_opengl_maincpp
// vr namespace comes from openvr.h

static bool g_bPrintf = true;


class RSGViveApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	// from hellovr_opengl_main.cpp
	RSGViveApp(int argc, char *argv[]);
	RSGViveApp();
	virtual ~RSGViveApp();

	bool bInit();
	bool bInitCompositor();
	
	void shutdown();

	bool handleInput();
	void printPositionData();
	vr::HmdQuaternion_t getRotation(vr::HmdMatrix34_t matrix);
	vr::HmdVector3_t getPosition(vr::HmdMatrix34_t matrix);
	void printDevicePositionalData(const char * deviceName, vr::HmdMatrix34_t posMatrix, vr::HmdVector3_t position, vr::HmdQuaternion_t quaternion);
	void processVREvent(const vr::VREvent_t & event);

	void updateHMDMatrixPose();

	Matrix4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPos);

	//float maxX = 0.0;
	//float minX = 100;
	//float maxY = 0.0;
	//float minY = 100;
	//float maxZ = 0.0;
	//float minZ = 100;

private:
	params::InterfaceGlRef mParams;

	vec2 trackerPos1 = vec2(0,0);
	vec2 trackerPos2 = vec2(0,0);
	vec2 trackerPos3 = vec2(0, 0);
	vec2 trackerPos4 = vec2(0, 0);
	float playAreaX, playAreaZ;

	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;

	vr::IVRSystem *m_pHMD;
	vr::IVRChaperone *chap;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	struct ControllerInfo_t
	{
		vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
		vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
		vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
		Matrix4 m_rmat4Pose;
	};

	enum EHand
	{
		Left = 0,
		Right = 1,
	};
	ControllerInfo_t m_rHand[2];


private: // OpenGL bookkeeping

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip;
	float m_fFarClip;

	unsigned int m_uiVertcount;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	vr::VRActionHandle_t m_actionHideCubes = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHideThisController = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionTriggerHaptic = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionAnalongInput = vr::k_ulInvalidActionHandle;

	vr::VRActionSetHandle_t m_actionsetDemo = vr::k_ulInvalidActionSetHandle;
};

void prepareSettings(RSGViveApp::Settings* settings)
{
	settings->setWindowSize(808, 480);
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a rising edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData));
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and had a falling edge
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData));
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && !actionData.bState;
}

//---------------------------------------------------------------------------------------------------------------------
// Purpose: Returns true if the action is active and its state is true
//---------------------------------------------------------------------------------------------------------------------
bool GetDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t *pDevicePath = nullptr)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData));
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf(const char *fmt, ...)
{
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	if (g_bPrintf)
		printf("%s", buffer);

	OutputDebugStringA(buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
RSGViveApp::RSGViveApp(int argc, char *argv[])
	: m_pHMD(NULL)
	, chap(NULL)
	, m_bDebugOpenGL(false)
	, m_bVerbose(false)
	, m_bPerf(false)
	, m_bVblank(false)
	, m_bGlFinishHack(true)
	, m_glControllerVertBuffer(0)
	, m_unControllerVAO(0)
	, m_iSceneVolumeInit(20)
	, m_strPoseClasses("")
{

	for (int i = 1; i < argc; i++)
	{
		if (!stricmp(argv[i], "-gldebug"))
		{
			m_bDebugOpenGL = true;
		}
		else if (!stricmp(argv[i], "-verbose"))
		{
			m_bVerbose = true;
		}
		else if (!stricmp(argv[i], "-novblank"))
		{
			m_bVblank = false;
		}
		else if (!stricmp(argv[i], "-noglfinishhack"))
		{
			m_bGlFinishHack = false;
		}
		else if (!stricmp(argv[i], "-noprintf"))
		{
			g_bPrintf = false;
		}
		else if (!stricmp(argv[i], "-cubevolume") && (argc > i + 1) && (*argv[i + 1] != '-'))
		{
			m_iSceneVolumeInit = atoi(argv[i + 1]);
			i++;
		}
	}
	// other initialization tasks are done in BInit
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
}

RSGViveApp::RSGViveApp() {

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
RSGViveApp::~RSGViveApp()
{
	// work is done in Shutdown
	dprintf("Shutdown");
}

//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool RSGViveApp::bInit()
{

	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return false;
	}

	if (!bInitCompositor())
	{
		printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}

	vr::VRInput()->SetActionManifestPath(Path_MakeAbsolute("../hellovr_actions.json", Path_StripFilename(Path_GetExecutablePath())).c_str());

	vr::VRInput()->GetActionHandle("/actions/demo/in/HideCubes", &m_actionHideCubes);
	vr::VRInput()->GetActionHandle("/actions/demo/in/HideThisController", &m_actionHideThisController);
	vr::VRInput()->GetActionHandle("/actions/demo/in/TriggerHaptic", &m_actionTriggerHaptic);
	vr::VRInput()->GetActionHandle("/actions/demo/in/AnalogInput", &m_actionAnalongInput);

	vr::VRInput()->GetActionSetHandle("/actions/demo", &m_actionsetDemo);

	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Left", &m_rHand[Left].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &m_rHand[Left].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Left", &m_rHand[Left].m_actionPose);

	vr::VRInput()->GetActionHandle("/actions/demo/out/Haptic_Right", &m_rHand[Right].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &m_rHand[Right].m_source);
	vr::VRInput()->GetActionHandle("/actions/demo/in/Hand_Right", &m_rHand[Right].m_actionPose);

	vr::VRChaperone()->GetCalibrationState();
	vr::VRChaperoneSetup()->RevertWorkingCopy();
	vr::VRChaperoneSetup()->GetWorkingPlayAreaSize(&playAreaX, &playAreaZ);
	//dprintf("PosX:%f PosZ:%f\n", playAreaX, playAreaZ);
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf("GL Error: %s\n", message);
}


//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool RSGViveApp::bInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RSGViveApp::shutdown()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool RSGViveApp::handleInput()
{
	//SDL_Event sdlEvent;
	bool bRet = false;

	// Process SteamVR events
	vr::VREvent_t event;
	while (m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		processVREvent(event);
	}

	// print position data from vive trackers
	printPositionData();

	// Process SteamVR action state
	// UpdateActionState is called each frame to update the state of the actions themselves. The application
	// controls which action sets are active with the provided array of VRActiveActionSet_t structs.
	vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = m_actionsetDemo;
	vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	vr::VRInputValueHandle_t ulHapticDevice;
	if (GetDigitalActionRisingEdge(m_actionTriggerHaptic, &ulHapticDevice))
	{
		if (ulHapticDevice == m_rHand[Left].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Left].m_actionHaptic, 0, 1, 4.f, 1.0f);
		}
		if (ulHapticDevice == m_rHand[Right].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(m_rHand[Right].m_actionHaptic, 0, 1, 4.f, 1.0f);
		}
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Prints out position (x,y,z) and rotation (qw.qx.qy.qz) into console
//-----------------------------------------------------------------------------
void RSGViveApp::printPositionData() {

	// positions(and rotations) are relative to the floor in center of the user's configured play space for the Standing tracking space.

	// Process StreamVR device states
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++) {
		if (!m_pHMD->IsTrackedDeviceConnected(unDevice)) {
			continue;
		}

		vr::VRControllerState_t state;
		if (m_pHMD->GetControllerState(unDevice, &state, sizeof(state))) {
			vr::TrackedDevicePose_t trackedDevicePose;
			vr::TrackedDevicePose_t trackedControllerPose;
			vr::VRControllerState_t controllerState;
			vr::HmdMatrix34_t poseMatrix;
			vr::HmdVector3_t position;
			vr::HmdQuaternion_t quaternion;
			vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(unDevice);

			switch (trackedDeviceClass) {
			//case vr::ETrackedDeviceClass::TrackedDeviceClass_HMD: vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, &trackedDevicePose, 1);
			//	// print position data for the HMD
			//	
			//	poseMatrix = trackedDevicePose.mDeviceToAbsoluteTracking;	// This matrix contains all positional and rotational data
			//	position = getPosition(trackedDevicePose.mDeviceToAbsoluteTracking);
			//	quaternion = getRotation(trackedDevicePose.mDeviceToAbsoluteTracking);

			//	printDevicePositionalData("HMD", poseMatrix, position, quaternion);


			//	break;

			case vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker: vr::VRSystem()->GetControllerStateWithPose(vr::TrackingUniverseStanding, unDevice, &controllerState, sizeof(controllerState), &trackedDevicePose);
				// print position data for a general vive tracker

				poseMatrix = trackedDevicePose.mDeviceToAbsoluteTracking;	// This matrix contains all positional and rotational data
				position = getPosition(trackedDevicePose.mDeviceToAbsoluteTracking);
				quaternion = getRotation(trackedDevicePose.mDeviceToAbsoluteTracking);

				char serialNumber[1024];
				vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, vr::Prop_SerialNumber_String, serialNumber, sizeof(serialNumber));

				//dprintf("\nSerial number: %s ", serialNumber);

				
				if (strcmp(serialNumber, "LHR-9F44C576") == 0) {
					printDevicePositionalData("tracker1", poseMatrix, position, quaternion);
				}
				else if (strcmp(serialNumber, "LHR-60182D91") == 0) {
					printDevicePositionalData("tracker2", poseMatrix, position, quaternion);
				}
				break;

			case vr::ETrackedDeviceClass::TrackedDeviceClass_Controller: vr::VRSystem()->GetControllerStateWithPose(vr::TrackingUniverseStanding, unDevice, &controllerState, sizeof(controllerState), &trackedControllerPose);
				// print position data for hand controllers
				poseMatrix = trackedControllerPose.mDeviceToAbsoluteTracking;	// This matrix contains all positional and rotational data
				position = getPosition(trackedControllerPose.mDeviceToAbsoluteTracking);
				quaternion = getRotation(trackedControllerPose.mDeviceToAbsoluteTracking);

				auto trackedControllerRole = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unDevice);
				std::string whichHand = "";
				if (trackedControllerRole == vr::TrackedControllerRole_LeftHand) {
					whichHand = "LeftHand";
				}
				else if (trackedControllerRole == vr::TrackedControllerRole_RightHand) {
					whichHand = "RightHand";
				}

				switch (trackedControllerRole)
				{

				case vr::TrackedControllerRole_Invalid:
					// invalid
					break;

				case vr::TrackedControllerRole_LeftHand:
				case vr::TrackedControllerRole_RightHand:
					printDevicePositionalData(whichHand.c_str(), poseMatrix, position, quaternion);

					break;

				}

				break;
			}
		}
	}

}

vr::HmdQuaternion_t RSGViveApp::getRotation(vr::HmdMatrix34_t matrix) {
	vr::HmdQuaternion_t q;

	q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
	q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
	q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);
	return q;
}

vr::HmdVector3_t RSGViveApp::getPosition(vr::HmdMatrix34_t matrix) {
	vr::HmdVector3_t vector;

	vector.v[0] = matrix.m[0][3];
	vector.v[1] = matrix.m[1][3];
	vector.v[2] = matrix.m[2][3];

	return vector;
}

void RSGViveApp::printDevicePositionalData(const char * deviceName, vr::HmdMatrix34_t posMatrix, vr::HmdVector3_t position, vr::HmdQuaternion_t quaternion) {
	LARGE_INTEGER qpc; // Query Performance Counter for Acquiring high-resolution time stamps.
					   // From MSDN: "QPC is typically the best method to use to time-stamp events and 
					   // measure small time intervals that occur on the same system or virtual machine.
	QueryPerformanceCounter(&qpc);

	// x axis is left-right, y axis is up-down, z axis is forward-back


	 // sPrint position and quaternion (rotation).
	/*dprintf("\n%lld, %s, x = %.5f, y = %.5f, z = %.5f, qw = %.5f, qx = %.5f, qy = %.5f, qz = %.5f",
		qpc.QuadPart, deviceName,
		position.v[0], position.v[1], position.v[2],
		quaternion.w, quaternion.x, quaternion.y, quaternion.z);
*/
	if (strcmp(deviceName, "LeftHand") == 0) {
		float newX = 808 * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = 460 * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos1 = vec2(newX, newZ);

	} else if (strcmp(deviceName, "RightHand") == 0) {
		float newX = 808 * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = 460 * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos2 = vec2(newX, newZ);
	}
	else if (strcmp(deviceName, "tracker1") == 0) {
		float newX = 808 * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = 460 * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos3 = vec2(newX, newZ);
	}
	else if (strcmp(deviceName, "tracker2") == 0) {
		float newX = 808 * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = 460 * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos4 = vec2(newX, newZ);
	}

	//dprintf("Min X:%f Max X:%f Min Z:%f Max Z:%f\n", minX, maxX, minZ, maxZ);


	// Uncomment this if you want to print entire transform matrix that contains both position and rotation matrix.
	//dprintf("\n%lld,%s,%.5f,%.5f,%.5f,x: %.5f,%.5f,%.5f,%.5f,y: %.5f,%.5f,%.5f,%.5f,z: %.5f,qw: %.5f,qx: %.5f,qy: %.5f,qz: %.5f",
	//    qpc.QuadPart, whichHand.c_str(),
	//    posMatrix.m[0][0], posMatrix.m[0][1], posMatrix.m[0][2], posMatrix.m[0][3],
	//    posMatrix.m[1][0], posMatrix.m[1][1], posMatrix.m[1][2], posMatrix.m[1][3],
	//    posMatrix.m[2][0], posMatrix.m[2][1], posMatrix.m[2][2], posMatrix.m[2][3],
	//    quaternion.w, quaternion.x, quaternion.y, quaternion.z);
}

//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void RSGViveApp::processVREvent(const vr::VREvent_t & event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		dprintf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		dprintf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void RSGViveApp::updateHMDMatrixPose()
{
	if (!m_pHMD)
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_rmat4DevicePose[nDevice] = convertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 RSGViveApp::convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}




void RSGViveApp::setup()
{
	 m_pHMD=NULL;
	 chap = NULL;
	 m_bDebugOpenGL=false;
	 m_bVerbose=false;
	 m_bPerf=false;
	 m_bVblank=false;
	 m_bGlFinishHack=true;
	 m_glControllerVertBuffer=0;
	 m_unControllerVAO=0;
	 m_iSceneVolumeInit=20;
	 m_strPoseClasses="";

	 bInit();
	
	 // set up parameters
	 // Create the interface and give it a name
	 mParams = params::InterfaceGl::create(getWindow(), "Ready Set Go", toPixels(ivec2(200, 200)));
	 mParams->addText("TA DA");

}

void RSGViveApp::mouseDown( MouseEvent event )
{
}

void RSGViveApp::update()
{
	bool bQuit = false;

	bQuit = handleInput();

	if (bQuit) {
		shutdown();
	}
}

void RSGViveApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 

	gl::color(Color(1, 0, 0));
	gl::drawSolidCircle(trackerPos1, 15);
	gl::color(Color(0,0,1));
	gl::drawSolidCircle(trackerPos2, 15);
	gl::color(Color(0, 1, 0));
	gl::drawSolidRect(Rectf(trackerPos3.x, trackerPos3.y, trackerPos3.x + 15, trackerPos3.y + 15));
	gl::color(Color::white());
	gl::drawSolidRect(Rectf(trackerPos4.x, trackerPos4.y, trackerPos4.x + 15, trackerPos4.y + 15));

	mParams->draw();
}

CINDER_APP( RSGViveApp, RendererGl, prepareSettings )
