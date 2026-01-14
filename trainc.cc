//	train controller
//
/*
Copyright © 2026 Doug Jones

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <iostream>
#include <cmath>
#include <numbers>
#include <list>
#include <vsg/all.h>

#include "mstsroute.h"
#include "train.h"
#include "trainc.h"
#include "tsgui.h"
#include "camerac.h"

TrainController::TrainController()
{
}

void TrainController::apply(vsg::KeyPressEvent& keyPress)
{
	if (keyPress.handled || !myTrain)
		return;
	if (keyPress.keyBase == 'a') {
		myTrain->decThrottle();
		keyPress.handled= true;
	} else if (keyPress.keyBase == 'd') {
		myTrain->incThrottle();
		keyPress.handled= true;
	} else if (keyPress.keyBase == 's') {
		myTrain->decReverser();
		keyPress.handled= true;
	} else if (keyPress.keyBase == 'w') {
		myTrain->incReverser();
		keyPress.handled= true;
	} else if (keyPress.keyBase == ';') {
		myTrain->decBrakes();
		keyPress.handled= true;
	} else if (keyPress.keyBase == '\'') {
		myTrain->incBrakes();
		keyPress.handled= true;
	} else if (keyPress.keyBase == '[') {
		myTrain->decEngBrakes();
		keyPress.handled= true;
	} else if (keyPress.keyBase == ']') {
		myTrain->incEngBrakes();
		keyPress.handled= true;
	} else if (keyPress.keyBase == '/') {
		myTrain->bailOff();
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_F5) {
		TSGuiData::instance().showStatus= !TSGuiData::instance().showStatus;
		keyPress.handled= true;
	} else if (keyPress.keyBase == '<') {
		myTrain->nextStopDist= myTrain->coupleDistance(true);
		if (myTrain->targetSpeed < -myTrain->speed)
			myTrain->targetSpeed= -myTrain->speed;
		keyPress.handled= true;
	} else if (keyPress.keyBase == '>') {
		myTrain->nextStopDist= myTrain->coupleDistance(false);
		if (myTrain->targetSpeed < myTrain->speed)
			myTrain->targetSpeed= myTrain->speed;
		keyPress.handled= true;
	} else if (keyPress.keyBase == '^') {
		myTrain->targetSpeed*= 1.4;
		keyPress.handled= true;
	} else if (keyPress.keyBase == ',') {
		myTrain->targetSpeed/= 1.4;
		keyPress.handled= true;
	} else if (keyPress.keyBase == '.') {
		float d= .5*myTrain->speed*myTrain->speed/(myTrain->decelMult*myTrain->decelMult);
		float mind= .5*myTrain->speed*myTrain->speed/myTrain->maxDecel;
		if (myTrain->speed < 0) {
			myTrain->nextStopDist= (myTrain->nextStopDist-mind)/2;
			if (myTrain->nextStopDist < -d)
				myTrain->nextStopDist= -d;
		}
		if (myTrain->speed > 0) {
			myTrain->nextStopDist= (myTrain->nextStopDist+mind)/2;
			if (myTrain->nextStopDist > d)
				myTrain->nextStopDist= d;
		}
		myTrain->targetSpeed/= 1.4;
		keyPress.handled= true;
	} else if (keyPress.keyBase == 'g') {
		myTrain->throwSwitch(keyPress.keyModifier==vsg::MODKEY_Shift);
		keyPress.handled= true;
	} else if (keyPress.keyBase=='c') {// && keyPress.keyModifier==vsg::MODKEY_Shift) {
		myTrain->connectAirHoses();
		keyPress.handled= true;
	} else if (keyPress.keyBase=='u') {
		myTrain->uncouple(clickLocation);
		keyPress.handled= true;
	}
}
