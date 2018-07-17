#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// from hellovr_opengl_main.cpp
//#include <SDL.h>
#include <openvr.h>

using namespace ci;
using namespace ci::app;
using namespace std;

// From hellovr_opengl_maincpp
// vr namespace comes from openvr.h
class CGLRenderModel {
	CGLRenderModel(const std::string & srenderModelName);
	~CGLRenderModel();

	bool bInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
	void Cleanup();
	void Draw();
	const std::string & getName() const { return m_sModelName; }

private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVectexCount;
	std::string m_sModelName;
};

static bool g_bPrintf = true;


class RSGViveApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	// from hellovr_opengl_main.cpp
	RSGViveApp(int argc, char *argv[]);
	virtual ~RSGViveApp();

	bool bInit();
	bool bInitGL();
	bool bInitCompositor();
	
	void shutdown();

	void runMainLoop();

};


void RSGViveApp::setup()
{
}

void RSGViveApp::mouseDown( MouseEvent event )
{
}

void RSGViveApp::update()
{
}

void RSGViveApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( RSGViveApp, RendererGl )
