#pragma once
#include <vector>
#include <chrono>
#include "Extras/OVR_Math.h"

using namespace OVR;

class Hand;
class HandModel;
class ScreenInfo;
class SelectedScreen;

namespace Leap {
	class Hand;
}

class HandList {
private:
	int event;
	int handNum;

	bool onZoom;
	float basis;
	float preScale;

	Matrix4f view, proj;
	Vector3f projHand;
	HandModel *model;
	std::vector<Hand> hands;
	SelectedScreen *selScreen;
	std::vector<ScreenInfo> *screenPos;
	std::chrono::system_clock::time_point start;

protected:
	int getHands();
	int analyze(Leap::Hand *hand, Hand *hi);

	//Shape
	bool isOpen(Leap::Hand *hand);
	bool isFist(Leap::Hand *hand);
	bool isPointing(Hand *hand);
	//bool isZoom(Leap::Hand *hand);
	//bool isPinch(Leap::Hand *hand);

	//Events
	int isClick(Leap::Hand *hand);
	int isSwipe(Leap::Hand *hand, Hand *hi);

	int onScreen(Vector3f projectedHand);
	void displacement(Matrix4f view, Matrix4f proj);

public:
	HandList();

	bool isFist();

	int getEvent() { return event; };
	
	Vector3f setHandPoints(Leap::Hand hand, Matrix4f combined);
	void setProgram(unsigned int program);
	void setScreenPos(std::vector<ScreenInfo> *screenPoses);

	void analyze();
	void Render(Matrix4f stillview, Matrix4f view, Matrix4f proj);

	bool isConnected();
};