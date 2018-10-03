#include "tracker.h"
#include "cinder/Rand.h"


Tracker::Tracker()
{
}

Tracker::Tracker(std::string number, cinder::vec2 pos) {
	this->serialNumber = number;
	this->position = pos;
	// TODO: alpha at 55%
	this->color = cinder::ColorA(cinder::Rand::randFloat(1.0f), cinder::Rand::randFloat(1.0f), cinder::Rand::randFloat(1.0f), 0.55f);
	this->name = "";
	this->selected = false;
	this->textureIndex = -1;
	this->actor = false;
}


Tracker::~Tracker()
{
}


void Tracker::select() {
	this->selected = true;
	this->color = cinder::ColorA(0, 0, 1, .45);
}