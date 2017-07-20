#include "HandList.h"
#include "Hand.h"
#include "HandModel.h"
#include "ScreenInfo.h"
#include "SelectedScreen.h"

#include "Leap.h"

#define FIST 0
#define OPEN 1
#define POINTING 2

using namespace OVR;
extern float screen_size;

inline Vector4f Mul(Matrix4f A, Leap::Vector B) {
	Vector4f temp;
	temp.x = A.M[0][0] * B.x
		+ A.M[0][1] * B.y
		+ A.M[0][2] * B.z
		+ A.M[0][3];
	temp.y = A.M[1][0] * B.x
		+ A.M[1][1] * B.y
		+ A.M[1][2] * B.z
		+ A.M[1][3];
	temp.z = A.M[2][0] * B.x
		+ A.M[2][1] * B.y
		+ A.M[2][2] * B.z
		+ A.M[2][3];
	temp.w = A.M[3][0] * B.x
		+ A.M[3][1] * B.y
		+ A.M[3][2] * B.z
		+ A.M[3][3];

	return temp;
}

Leap::Listener listener;
Leap::Controller controller;

HandList::HandList() {
	model = NULL;
	selScreen = NULL;
	screenPos = NULL;

	event = 0;
	handNum = 0;
	onZoom = false;
	basis = 0.0f;
	preScale = 1.0f;

	//controller.removeListener(listener);
	controller.addListener(listener);
	controller.setPolicy(Leap::Controller::POLICY_OPTIMIZE_HMD);
	controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
}

int HandList::getHands() {
	//Leap::Frame frame = controller.frame();
	//Leap::HandList leapHands = frame.hands();

	//handNum = leapHands.count();

	//std::vector<Hand>::iterator it;
	//for (it = hands.begin(); it != hands.end(); it++)
	//	it->valid = false;
	//
	//bool isSameHand;
	//for (Leap::HandList::const_iterator hi = leapHands.begin(); hi != leapHands.end(); hi++){
	//	Leap::Hand pHand = (*hi);
	//	isSameHand = false;
	//	for (it = hands.begin(); it != hands.end(); it++) {
	//		if (it->getId() == pHand.id()) {
	//			isSameHand = false;
	//			it->valid = true;
	//			it->setHand(&(pHand));
	//			break;
	//		}
	//	}

	//	Hand temp;
	//	temp.valid = true;
	//	temp.setHand(&(pHand));
	//	hands.push_back(temp);
	//}

	//for (it = hands.begin(); it != hands.end(); it++)
	//	if (!it->valid)
	//		it = hands.erase(it);

	return handNum;
}

Vector3f HandList::setHandPoints(Leap::Hand hand, Matrix4f combined) {
	Leap::Vector handCenter = hand.palmPosition();
	model->setHandPos(handCenter.x, handCenter.y, handCenter.z);

	Leap::FingerList fingers = hand.fingers();
	for (int i = 0; i < fingers.count(); i++) {
		// Get finger bones
		Leap::Vector boneStart;
		for (int b = 0; b < 4; ++b) {
			Leap::Bone::Type boneType = static_cast<Leap::Bone::Type>(b);
			Leap::Bone bone = fingers[i].bone(boneType);
			boneStart = bone.prevJoint();
			model->setHandPos(boneStart.x, boneStart.y, boneStart.z);
		}
		model->setHandPos(fingers[i].tipPosition().x, fingers[i].tipPosition().y, fingers[i].tipPosition().z);
	}

	Leap::Vector index = hand.fingers()[1].bone(static_cast<Leap::Bone::Type>(3)).prevJoint();
	Vector4f projectedHand = Mul(combined * model->GetMatrix(), index);
	projectedHand /= projectedHand.w;

	return Vector3f(projectedHand.x, projectedHand.y, projectedHand.z);
}

void HandList::setProgram(unsigned int program) {
	if (model != NULL) delete model;
	model = new HandModel(Vector3f(0, 0, 0), program);
	model->initScreen(0, 0, 0, 0, 0);

	if (selScreen != NULL) delete selScreen;
	selScreen = new SelectedScreen(Vector3f(0, 0, 0), program);
	selScreen->initScreen(0, 0, 0.0f, 0.64f * screen_size, 0.48f*screen_size);
	selScreen->setColor(1.0f, 0.0f, 0.0f, 1.0f);
}

void HandList::setScreenPos(std::vector<ScreenInfo> *screenPoses) {
	this->screenPos = screenPoses;
}

int HandList::onScreen(Vector3f projectedHand) {
	int i;

	printf("Hand: %3.3f %3.3f %3.3f\n", projectedHand.x, projectedHand.y, projectedHand.z);
	//printf("Hand: %3.3f %3.3f %3.3f\n", projectedHand.x, projectedHand.y, projectedHand.z);
	for (i = 0; i < screenPos->size(); i++) {
		Vector3f *projected = (*screenPos)[i].Projected;
		printf("%d screen: %3.3f %3.3f %3.3f\n", i, projected[0].x, projected[0].y, projected[0].z);
		printf("%d screen: %3.3f %3.3f %3.3f\n", i, projected[1].x, projected[1].y, projected[1].z);
		if (projected[0].x / projected[0].z >= projectedHand.x / projectedHand.z &&
			projected[1].x / projected[1].z <= projectedHand.x / projectedHand.z &&
			projected[0].y / projected[0].z <= projectedHand.y / projectedHand.z &&
			projected[1].y / projected[1].z >= projectedHand.y / projectedHand.z) 
		{		
			//printf("screen %d is selected\n", i + 1);
			return i + 1;
		}
	}

	return 0;
}

void HandList::displacement(Matrix4f view, Matrix4f proj) {
	for (std::vector<Hand>::iterator hi = hands.begin(); hi != hands.end(); ++hi) {
		if (hi->getScreenId()) {
			Vector3f curPos = hi->getCurPos();
			Vector3f Displace = hi->getDisplacement();
			printf("Hand %d: Displacement: %3.3f %3.3f %3.3f\n", hi->getId(), Displace.x, Displace.y, Displace.z);
			printf("Hand %d: Yaw change: %3.3f\n", hi->getId(), Displace.x / curPos.z);
			//(*screenPos)[hi->getScreenId() - 1].Angle.y += Displace.x / curPos.z;
			//(*screenPos)[hi->getScreenId() - 1].Pos.y += (*screenPos)[hi->getScreenId() - 1].Pos.z * (Displace.y / curPos.z);
			(*screenPos)[hi->getScreenId() - 1].Pos.x += (Displace.x / curPos.z) * (*screenPos)[hi->getScreenId() - 1].Pos.z;
			(*screenPos)[hi->getScreenId() - 1].Pos.y += (Displace.y / curPos.z) * (*screenPos)[hi->getScreenId() - 1].Pos.z;
			(*screenPos)[hi->getScreenId() - 1].Pos.z += (Displace.z / curPos.z) * (*screenPos)[hi->getScreenId() - 1].Pos.z;
			//(*screenPos)[hi->getScreenId() - 1].Pos += Displace;
			hi->setPrePos(curPos);

			selScreen->sM = &(*screenPos)[hi->getScreenId() - 1];
			selScreen->initScreen(0, 0, 0.0f, 0.64f * screen_size, 0.48f*screen_size);
			selScreen->setColor(1.0f, 0.0f, 0.0f, 1.0f); // displacement color
			selScreen->Render(view, proj);
		}
	}
}

void HandList::Render(Matrix4f stillview, Matrix4f view, Matrix4f proj) {
	this->view = view;
	this->proj = proj;

	Leap::Frame frame = controller.frame();
	Leap::HandList leapHands = frame.hands();

	handNum = leapHands.count();
	bool reset = true;
	std::vector<Hand>::iterator it;
	for (it = hands.begin(); it != hands.end(); it++) {
		reset = reset & (it->getDelay() >= 1.0f);
		it->valid = false;
	}

	if (reset)
		event = 0;

	char mat[50];
	bool isSameHand;
	for (int i = 0; i < handNum; i++) {
		Leap::Hand pHand = leapHands[i];
		isSameHand = false;
		for (it = hands.begin(); it != hands.end(); ++it) {
			if (it->getId() == pHand.id()) {
				isSameHand = true;
				it->valid = true;
				it->setHandId(pHand.id());
				projHand = setHandPoints(pHand, proj*stillview);
				it->onScreen = onScreen(projHand);

				if (isOpen(&pHand))
					event = event | isSwipe(&pHand, &(*it));

				//printf("%3.3f %3.3f %3.3f %3.3f\n", stillview.M[0][0], stillview.M[0][1], stillview.M[0][2], stillview.M[0][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", stillview.M[1][0], stillview.M[1][1], stillview.M[1][2], stillview.M[1][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", stillview.M[2][0], stillview.M[2][1], stillview.M[2][2], stillview.M[2][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", stillview.M[3][0], stillview.M[3][1], stillview.M[3][2], stillview.M[3][3]);

				//printf("%3.3f %3.3f %3.3f %3.3f\n", proj.M[0][0], proj.M[0][1], proj.M[0][2], proj.M[0][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", proj.M[1][0], proj.M[1][1], proj.M[1][2], proj.M[1][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", proj.M[2][0], proj.M[2][1], proj.M[2][2], proj.M[2][3]);
				//printf("%3.3f %3.3f %3.3f %3.3f\n", proj.M[3][0], proj.M[3][1], proj.M[3][2], proj.M[3][3]);

				printf("Hand %d: %3.3f %3.3f %3.3f\n", it->getId(), projHand.x, projHand.y, projHand.z);
				model->Render(stillview, proj);
				break;
			}
		}
		
		if (!isSameHand) {
			Hand temp;
			temp.valid = true;
			temp.setHandId(pHand.id());
			projHand = setHandPoints(pHand, proj*stillview);
			temp.onScreen = onScreen(projHand);

			if (isOpen(&pHand))
				event = event | isSwipe(&pHand, &temp);

			printf("Hand %d: %3.3f %3.3f %3.3f\n", temp.getId(), projHand.x, projHand.y, projHand.z);
			model->Render(stillview, proj);
			hands.push_back(temp);
		}
	}

	for (it = hands.begin(); it != hands.end(); )
		if (!it->valid)
			it = hands.erase(it);
		else
			it++;
}

void HandList::analyze() {
	Leap::Frame frame = controller.frame();

	std::vector<Hand>::iterator it;
	for (it = hands.begin(); it != hands.end(); it++)
		it->setState(analyze(&(frame.hand(it->getId())), &(*it)));

	if (hands.size() == 2) {
		if ((hands[0].getScreenId() == 0) ^ (hands[1].getScreenId() == 0)) {
			float angle = hands[0].getDirect().Angle(hands[1].getDirect());
			printf("Zoom action: angle = %3.3f\n", angle);
			if (angle >= 0.70f && angle <= 1.5) {
				if (!onZoom) {
					onZoom = true;
					start = std::chrono::system_clock::now();
				}

				std::chrono::duration<float> duration = std::chrono::system_clock::now() - start;

				if (duration.count() <= 1.0f)
					basis = hands[0].getCurPos().Distance(hands[1].getCurPos());

				if (duration.count() >= 1.0f) {
					int index;
					if (hands[0].getScreenId())
						index = hands[0].getScreenId() - 1;
					else
						index = hands[1].getScreenId() - 1;

					printf("distance = %3.3f\n", hands[0].getCurPos().Distance(hands[1].getCurPos()));
					(*screenPos)[index].scale = preScale + (hands[0].getCurPos().Distance(hands[1].getCurPos()) - basis)*1.5f;
					printf("Scale difference = %3.3f\n", (*screenPos)[index].scale);
					selScreen->sM = &(*screenPos)[index];
					selScreen->setColor(0.0f, 0.0f, 1.0f, 1.0f);
					selScreen->Render(view, proj);

					return;
				}
				else {
					for (it = hands.begin(); it != hands.end(); ++it) {
						if (it->getScreenId())
							preScale = (*screenPos)[it->getScreenId() - 1].scale;

						if (isPointing(&(*it)))
							displacement(view, proj);
					}

					for (std::vector<Hand>::iterator hi = hands.begin(); hi != hands.end(); ++hi) {
						if (hi->getScreenId()) {
							selScreen->sM = &(*screenPos)[hi->getScreenId() - 1];
							selScreen->setColor(1.0f, 0.0f, 0.0f, 1.0f);
							selScreen->Render(view, proj);
						}
					}
					return;
				}
			}
		}
	}

	onZoom = false;
	for (it = hands.begin(); it != hands.end(); ++it) {
		if (it->getScreenId())
			preScale = (*screenPos)[it->getScreenId() - 1].scale;

		if (isPointing(&(*it)))
			displacement(view, proj);
	}

	for (std::vector<Hand>::iterator hi = hands.begin(); hi != hands.end(); ++hi) {
		if (hi->getScreenId()) {
			selScreen->sM = &(*screenPos)[hi->getScreenId() - 1];
			selScreen->setColor(1.0f, 0.0f, 0.0f, 1.0f); // Click color setting
			selScreen->Render(view, proj);
		}
	}
}

int HandList::analyze(Leap::Hand *hand, Hand *hi) {
	int state = 0;
	if (isFist(hand)) {
		state = 1;
		//hi->setScreenId(0);
	}

	//if (isPinch(hand)) {
	//	printf("Hand %d: isPinch\n", hand->id());
	//	state += 2;
	//}
	
	if (isClick(hand)) {
		if (!hi->getScreenId() && hi->onScreen) {
			//printf("Hand %d is on screen %d\n", hi->getId(), hi->onScreen);
			if (hi->onScreen) {
				hi->setScreenId();
				preScale = (*screenPos)[hi->onScreen - 1].scale;

				Leap::Vector base = hand->fingers()[1].bone(static_cast<Leap::Bone::Type>(0)).prevJoint();
				Vector4f temp = Mul(model->GetMatrix(), base);
				temp /= temp.w;
				hi->setCurPos(Vector3f(temp.x, temp.y, temp.z));
			}
			printf("\n\n\n\nclick\n\n\n\n\n");
		}
		else {
			hi->setScreenId(0);
		}
	}

	Leap::Vector base = hand->fingers()[1].bone(static_cast<Leap::Bone::Type>(0)).prevJoint();
	Vector4f temp = Mul(model->GetMatrix(), base);
	temp /= temp.w;
	hi->setCurPos(Vector3f(temp.x, temp.y, temp.z));
	Leap::Vector direct = hand->fingers()[1].direction();
	hi->setDirect(Vector3f(direct.x, direct.y, direct.z));

	return state;
}

bool HandList::isOpen(Leap::Hand *hand) {
	int check = 0;
	Leap::FingerList fingers = hand->fingers();
	check += 1 == fingers[0].isExtended();
	check += 1 == fingers[1].isExtended();
	check += 1 == fingers[2].isExtended();
	check += 1 == fingers[3].isExtended();
	check += 1 == fingers[4].isExtended();

	if (check >= 4) 
		return true;
	else
		return false;
}

bool HandList::isFist(Leap::Hand *hand) {
	int check = 0;
	Leap::FingerList fingers = hand->fingers();
	check += 0 == fingers[0].isExtended();
	check += 0 == fingers[1].isExtended();
	check += 0 == fingers[2].isExtended();
	check += 0 == fingers[3].isExtended();
	check += 0 == fingers[4].isExtended();

	if (check == 5)
		return true;
	else
		return false;
}

bool HandList::isFist() {
	if (hands.size() == 2)
		if (hands[0].isFist() && hands[1].isFist())
			return 1;

	return 0;
}

bool HandList::isPointing(Hand *hand) {
	Leap::Frame frame = controller.frame();
	int check = 0;
	Leap::FingerList fingers = frame.hand(hand->getId()).fingers();
	check += 0 == fingers[0].isExtended();
	check += fingers[2].isExtended() || fingers[1].isExtended();
	check += 0 == fingers[3].isExtended();
	check += 0 == fingers[4].isExtended();

	if (check == 4) 
		return true;
	else
		return false;
}

//bool HandList::isZoom(Leap::Hand *hand) {
//	int check = 0;
//	Leap::FingerList fingers = hand->fingers();
//	check += 1 == fingers[0].isExtended();
//	check += 1 == fingers[1].isExtended();
//
//	printf("Zoom check is %d\n", check);
//	if (check == 2)
//		return true;
//	else
//		return false;
//}

//bool HandList::isPinch(Leap::Hand *hand) {
//	Leap::Vector thumb = hand->fingers()[0].tipPosition();
//	Leap::Vector index = hand->fingers()[1].tipPosition();
//	float dist = thumb.distanceTo(index);
//
//	int check = 0;
//	check += hand->fingers()[2].isExtended();
//	check += hand->fingers()[3].isExtended();
//	check += hand->fingers()[4].isExtended();
//
//	if (check >= 2 && dist <= 20.0f)
//		return true;
//	else
//		return false;
//}

int HandList::isClick(Leap::Hand *hand) {
	Leap::Vector fingerTip = hand->fingers()[1].tipVelocity();
	//Leap::Vector fingerTip2 = hand->fingers()[2].tipVelocity();
	Leap::Vector base = hand->palmVelocity();

	//printf("pointing action, position- x: %3.3f, y: %3.3f\n", projHand.x, projHand.y);
	//printf("pointing action, velocity- x: %3.3f, y: %3.3f, z: %3.3f\n", fingers[1].tipVelocity().x, fingers[1].tipVelocity().y, fingers[1].tipVelocity().z);
	//printf("pointing action, velocity- x: %3.3f, y: %3.3f, z: %3.3f\n", fingers[2].tipVelocity().x, fingers[2].tipVelocity().y, fingers[2].tipVelocity().z);
	//printf("Velocity difference: %3.3f\n", fingerTip.magnitude() - base.magnitude());
	//printf("Velocity difference2: %3.3f\n", fingerTip2.magnitude() - base.magnitude());
	if (fingerTip.magnitude() - base.magnitude() >= 500.0f) {
	//if (fingerTip.magnitude() - base.magnitude() >= 500.0f || fingerTip2.magnitude() - base.magnitude() >= 500.0f) {
		if (base.x >= 250 || base.x <= -250)
			return 0;
		if (base.y >= 250 || base.y <= -250)
			return 0;
		if (base.z >= 250 || base.z <= -250)
			return 0;

		//std::chrono::duration<float> duration = (std::chrono::system_clock::now() - start);
		//if (duration.count() <= 0.50f)
		//	return 0;

		//start = std::chrono::system_clock::now();
		return 1;
	}

	//if (duration.count() >= 0.50f && pM->sM != NULL && fingers[0].isExtended()) {
	//	pM->sM->scale = preScale + 3 * (hands[num].direction().x - baseDirection);
	//	printf("Pointing: chaned scale = %3.3f\n", pM->sM->scale);
	//}
	//printf("Pointing: direction= %3.3f %3.3f %3.3f\n", hands[num].direction().x, hands[num].direction().y, hands[num].direction().z);

	return 0;
}

int HandList::isSwipe(Leap::Hand *hand, Hand *hi) {
	int mode = 0;

	Leap::Vector normal = hand->palmNormal();
	//printf("Open. normal = %3.3f, %3.3f, %3.3f\n", normal.x, normal.y, normal.z);
	if (hand->isRight() && normal.x >= 0.65f && normal.y <= 0.3f && normal.z <= 0.3f) {
		Leap::Vector velocity = hand->palmVelocity();
		//printf("Swift ready. Vecolity = %3.3f, %3.3f, %3.3f\n", velocity.x, velocity.y, velocity.z);

		printf("Hand %d: delay = %3.3f\n", hi->getId(), hi->getDelay());
		if ((velocity.x >= 1000.0f) & (hi->getDelay() >= 0.30f)) {
			hi->setDelay();
			//printf("\n\n\nHand %d: swipe action!\n\n\n\n", hand->id());
			mode = 4;
		}

		if ((velocity.x <= -800.0f) & (hi->getDelay() >= 0.30f)) {
			hi->setDelay();
			//printf("\n\n\nHand %d: swipe action!\n\n\n\n", hand->id());
			mode = 8;
		}
	}

	if (hand->isLeft() && normal.x <= -0.65f && normal.y <= 0.3f && normal.z <= 0.3f) {
		Leap::Vector velocity = hand->palmVelocity();
		//printf("Swift ready. Vecolity = %3.3f, %3.3f, %3.3f\n", velocity.x, velocity.y, velocity.z);

		printf("Hand %d: delay = %3.3f\n", hi->getId(), hi->getDelay());
		if ((velocity.x >= 800.0f) & (hi->getDelay() >= 0.30f)) {
			hi->setDelay();
			//printf("\n\n\nHand %d: swipe action!\n\n\n\n", hand->id());
			mode = 1;
		}

		if ((velocity.x <= -1000.0f) & (hi->getDelay() >= 0.30f)) {
			hi->setDelay();
			//printf("\n\n\nHand %d: swipe action!\n\n\n\n", hand->id());
			mode = 2;
		}
	}

	return mode;
}

bool HandList::isConnected() {
	return controller.isConnected();
}