// Kat Sullivan
// Ready Set Go

#include "cinder/app/App.h"

#pragma once
class Tracker
{
public:
	Tracker();
	Tracker(std::string number, cinder::vec2 pos);
	~Tracker();

	void select();

	std::string serialNumber;
	cinder::vec2 position;
	cinder::Color color;
	std::string name;
	bool selected;

	
};

