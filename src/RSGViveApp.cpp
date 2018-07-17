#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class RSGViveApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
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
