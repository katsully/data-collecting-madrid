#include "tracker.h"
#include "cinder/Rand.h"


Tracker::Tracker()
{
}

Tracker::Tracker(std::string number, cinder::vec2 pos) {
	this->serialNumber = number;
	this->position = pos;
	this->color = cinder::Color(cinder::Rand::randFloat(1.0f), cinder::Rand::randFloat(1.0f), cinder::Rand::randFloat(1.0f));
	this->name = "";
	this->selected = false;
}


Tracker::~Tracker()
{
}


void Tracker::select() {
	this->selected = true;
	this->color = cinder::Color(0, 0, 1);
}