#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "../vc2015/tracker.h"
//#include "cinder/qtime/AvfWriter.h

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
	void update() override;
	void draw() override;
	void mouseUp(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag( MouseEvent event );
	void setTrackerName(std::string name);
	void setTrackerColor();
	void render();
	void button();
	void clear();
	void fullScreen();

	// from hellovr_opengl_main.cpp
	RSGViveApp(int argc, char *argv[]);
	RSGViveApp();
	virtual ~RSGViveApp();

	bool bInit();
	
	void shutdown();

	bool handleInput();
	void printPositionData();
	vr::HmdQuaternion_t getRotation(vr::HmdMatrix34_t matrix);
	vr::HmdVector3_t getPosition(vr::HmdMatrix34_t matrix);
	void printDevicePositionalData(const char * deviceName, vr::HmdMatrix34_t posMatrix, vr::HmdVector3_t position, vr::HmdQuaternion_t quaternion);
	void processVREvent(const vr::VREvent_t & event);

	void updateHMDMatrixPose();

	Matrix4 convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPos);

	int mTrailLimit;
	int mPageNum;
	bool mRecord;
	bool mFullScreen = true;

private:
	params::InterfaceGlRef mParams;

	vec2 trackerPos1 = vec2(0,0);
	vec2 trackerPos2 = vec2(0,0);
	vec2 trackerPos3 = vec2(0, 0);
	vec2 trackerPos4 = vec2(0, 0);
	vec2 trackerPos5 = vec2(0, 0);
	//vector<pair<string, vec2>> trackers;
	vector<Tracker> trackers;
	Tracker activeTracker;
	float playAreaX, playAreaZ;
	vector<vector<vec2>> mTrails;

	// quicktime
	//qtime::MovieWriterRef mMovieExporter;
	//qtime::MovieWriter::Format format;

	// for the page number text box
	gl::TextureRef mTextTexture;
	vec2 mSize;
	Font mFont;

	// For the initizaltion text
	gl::TextureRef mTextTextureInit;
	vec2 mSizeInit;
	Font mFontInit;
	bool init = true;
	int actorNum = 1;
	string actorNames[10] = { "", "", "", "", "", "", "", "", "", "" };
	Color colors[5] = { Color(1,0,0), Color(0,1,0), Color(0,0,1), Color(1,1,0), Color(0,1,1) };
	bool addActor = false;

	vec2 startHighlightBox = vec2(0,0);
	vec2 endHighlightBox = vec2(0,0);

	gl::TextureRef mTable;
	gl::TextureRef mIsland;

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


private: 

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	float m_fScaleSpacing;
	float m_fScale;

	unsigned int m_uiVertcount;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

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
	, m_strPoseClasses("")
{

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

void RSGViveApp::setup()
{
	m_pHMD = NULL;
	chap = NULL;
	m_strPoseClasses = "";
	mPageNum = 1;

	setFullScreen(mFullScreen);

	// load floor plan image
	try {
		mTable = gl::Texture::create(loadImage(loadAsset("HoH_RSG_Table_V001_transparent.png")));
		mIsland = gl::Texture::create(loadImage(loadAsset("HoH_RSG_Waves_02_Island_V001.png")));
	}
	catch (...) {
		dprintf("unable to load the texture file!");
	}

	// quicktime
	// fs::path path = getSaveFilePath();
	// if (!path.empty()) {
		//format = qtime::MovieWriter::Format().codec(qtime::MovieWriter::H2364).fileType(qtime::movieWriter::QUICK_TIME_MOVIE).setTimeScale(250);

		//mMovieExporter = qtime::MovieWriter::create(path, getWindowWidth(), getWindowHeight(), format);
	// }

	// setting up the text box
#if defined (CINDER_COCOA)
	mFont = Font("Helvetica", 24);
	mFontInit = Font("Helvetica", 24);
#else
	mFont = Font("Times New Roman", 32);
	mFontInit = Font("Times New Roman", 64);
#endif
	mSize = vec2(100, 100);
	mSizeInit = vec2(600, 300);

	render();

	mTrailLimit = 100;
	vector<vec2> v = { vec2(0,0) };
	for (int i = 0; i < 5; i++) {
		mTrails.push_back(v);
	}
	mRecord = false;

	bInit();

	// set up parameters
	// Create the interface and give it a name
	mParams = params::InterfaceGl::create(getWindow(), "Ready Set Go", toPixels(ivec2(200, 200)));
	mParams->addParam("Trails", &mTrailLimit);
	mParams->addParam("Recording", &mRecord);
	mParams->addButton("Next Page", bind(&RSGViveApp::button, this));
	mParams->addButton("Clear Trails", bind(&RSGViveApp::clear, this));
	mParams->addButton("Toggle Full Screen", bind(&RSGViveApp::fullScreen, this));
}

void RSGViveApp::fullScreen() {
	mFullScreen = !mFullScreen;
	setFullScreen(mFullScreen);
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
				//dprintf("\nNumber of trackers: %i ", trackers.size());
				printDevicePositionalData(serialNumber, poseMatrix, position, quaternion);

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
	if (strcmp(deviceName, "LeftHand") == 0) {
		float newX = getWindowWidth() * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = getWindowHeight() * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos1 = vec2(newX, newZ);

	} else if (strcmp(deviceName, "RightHand") == 0) {
		float newX = getWindowWidth() * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = getWindowHeight() * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));
		trackerPos2 = vec2(newX, newZ);
	}
	else {
		float newX = getWindowWidth() * ((position.v[0] - -playAreaX) / (playAreaX - -playAreaX));
		float newZ = getWindowHeight() * ((position.v[2] - -playAreaZ) / (playAreaZ - -playAreaZ));

		// find pair in vector by serial number
		auto it = find_if(trackers.begin(), trackers.end(), [&deviceName](const Tracker& element) { return element.serialNumber == deviceName; });
		// item exists in map
		if (it != trackers.end()) {
			// update key in map
			it->position = vec2(newX, newZ);
		}
		// item does not exist
		else {
			// not found, insert in map
			trackers.push_back(Tracker(deviceName, vec2(newX, newZ)));
		}
	}

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


void RSGViveApp::render() {
	string txt1 = "Page " + std::to_string(mPageNum);
	TextBox tbox = TextBox().alignment(TextBox::LEFT).font(mFont).size(ivec2(mSize.x, TextBox::GROW)).text(txt1);
	tbox.setColor(Color::white());
	mTextTexture = gl::Texture2d::create(tbox.render());

	if (init) {
		string txt2 = "Click and Drag to highlight the two trackers that represent Actor " + std::to_string(actorNum);
		TextBox tbox = TextBox().alignment(TextBox::LEFT).font(mFontInit).size(ivec2(mSizeInit.x, TextBox::GROW)).text(txt2);
		tbox.setColor(Color(0,0,1));
		mTextTextureInit = gl::Texture2d::create(tbox.render());
	}
}

void RSGViveApp::mouseDown(MouseEvent event) {
	startHighlightBox = event.getPos();
}

void RSGViveApp::mouseDrag(MouseEvent event) {
	endHighlightBox = event.getPos();
	Rectf rect = Rectf(startHighlightBox, endHighlightBox);
	for (Tracker &tracker : trackers) {
		// check if tracker in square
		if (rect.contains(tracker.position)) {
			tracker.select();
			activeTracker = tracker;
			addActor = true;
		}
	}

}

void RSGViveApp::button() {
	mPageNum++;
	render();
}

void RSGViveApp::mouseUp(MouseEvent event) {
	startHighlightBox = vec2(0, 0);
	endHighlightBox = vec2(0, 0);
	if (addActor) {
		mParams->addParam("Actor " + std::to_string(actorNum), &actorNames[actorNum-1]).updateFn([this] { setTrackerName(actorNames[actorNum-1]); render(); });
		// TODO: get fixed colors to choose from
		mParams->addParam("Color for Actor " + std::to_string(actorNum), &trackers[actorNum-1].color).updateFn([this] {setTrackerColor(); });
		actorNum++;
	}
	addActor = false;

}

void RSGViveApp::setTrackerName(std::string name) {
	for (Tracker &tracker : trackers) {
		// check if tracker in square
		if (tracker.selected) {
			tracker.name = name;
		}
	}
}


void RSGViveApp::setTrackerColor() {
	for (Tracker &tracker : trackers) {
		// check if tracker in square
		if (tracker.selected) {
			tracker.color = activeTracker.color;
			tracker.selected = false;
		}
	}
}

void RSGViveApp::clear() {
	for (vector<vec2> t : mTrails) {
		t.clear();
	}
}

void RSGViveApp::update()
{
	bool bQuit = false;

	bQuit = handleInput();

	if (bQuit) {
		shutdown();
	}

	// quicktime rendering
	/*if (mMovieExporter && getElapsedFrames() > 1 && getElapsedFrames() < 100000) {
		mMovieExporter->addFrame(copyWindowSurface());
	}
	else if (mMovieExporter && getElapsedFrames() >= 1000000) {
		mMovieExporter->finish();
		mMovieExporter.reset();
	}*/


}

void RSGViveApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::color(Color::white());

	gl::draw(mIsland, Rectf(0, 0, getWindowWidth(), getWindowHeight()));
	float newX = trackerPos2.x - ((float)getWindowWidth() * .27);
	float newY = trackerPos2.y - ((float)getWindowHeight() * .68);
	gl::draw(mTable, Rectf(newX, newY, newX+getWindowWidth(), newY+getWindowHeight()));

	gl::color(Color(0, 0, 1));

	// init stage
	if (init) {
		if (mTextTextureInit) {
			gl::draw(mTextTextureInit, Rectf(getWindowWidth()*.4, getWindowHeight()*.2, getWindowWidth() *.4 + getWindowWidth() * .3, getWindowHeight() *.2 + 300));
		}
	}

	gl::drawStrokedRect(Rectf(startHighlightBox.x, startHighlightBox.y, endHighlightBox.x, endHighlightBox.y));

	gl::color(Color::black());
	// draw page number
	if (mTextTexture) {
		gl::draw(mTextTexture, Rectf(getWindowWidth() - 130, 0, getWindowWidth(), 60));
	}


	// draw trails
	//for (vector<vec2> trails : mTrails) {
	//	for (vec2& t : trails) {
	//		gl::drawSolidCircle(t, 5);
	//	}
	//}

	for (Tracker &tracker : trackers) {
		gl::color(tracker.color);
		// TODO: png not circle
		gl::drawSolidCircle(tracker.position, 25);
	}

	/*gl::color(Color(1, 0, 0));
	gl::drawSolidCircle(trackerPos1, 15);
	if (mTrails[0].size() == 0 || mTrails[0].back() != trackerPos1) {
		mTrails[0].push_back(trackerPos1);
	}
	gl::color(Color(0,0,1));
	gl::drawSolidCircle(trackerPos2, 15);
	if (mTrails[1].size() == 0 || mTrails[1].back() != trackerPos2) {
		mTrails[1].push_back(trackerPos2);
	}	
	gl::color(Color(0, 1, 0));
	gl::drawSolidRect(Rectf(trackerPos3.x, trackerPos3.y, trackerPos3.x + 15, trackerPos3.y + 15));
	if (mTrails[2].size() == 0 || mTrails[2].back() != trackerPos3) {
		mTrails[2].push_back(trackerPos3);
	}	gl::color(Color::white());
	gl::drawSolidRect(Rectf(trackerPos4.x, trackerPos4.y, trackerPos4.x + 15, trackerPos4.y + 15));
	if (mTrails[3].size() == 0 || mTrails[3].back() != trackerPos4) {
		mTrails[3].push_back(trackerPos4);
	}	gl::color(Color(1, 0, 1));
	gl::drawSolidRect(Rectf(trackerPos5.x, trackerPos5.y, trackerPos5.x + 15, trackerPos5.y + 15));
	if (mTrails[4].size() == 0 || mTrails[4].back() != trackerPos5) {
		mTrails[4].push_back(trackerPos5);
	}*/

	//for (vector<vec2>& trails : mTrails) {
	//	if (trails.size() > mTrailLimit && trails.size() > 1) {
	//		int n = trails.size() - mTrailLimit;
	//		trails.erase(trails.begin(), trails.begin() + n);
	//	}
	//}


	mParams->draw();
}

CINDER_APP( RSGViveApp, RendererGl, prepareSettings )
