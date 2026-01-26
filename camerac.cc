//	camera controller for view from ground
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

#include "camerac.h"
#include "mstsroute.h"
#include "train.h"
#include "listener.h"

vsg::LookAt* myLookAt= nullptr;

CameraController::CameraController(vsg::ref_ptr<vsg::Camera> cam, vsg::ref_ptr<vsg::Node> node) :
  scene(node),
  camera(cam),
  lookAt(cam->viewMatrix.cast<vsg::LookAt>())
{
	perspective= dynamic_cast<vsg::Perspective*>(cam->projectionMatrix.get());
	auto dist= vsg::length(lookAt->eye - lookAt->center);
	zoom1Dist= 1.4;
	auto z= std::log(dist)/std::log(zoom1Dist);
	maxZoom= zoom= (int)z;
	if (!myLookAt)
		myLookAt= lookAt.get();
}

void CameraController::incHeading(double degrees)
{
	auto rot= vsg::rotate(vsg::radians(degrees),vsg::dvec3(0,0,1));
	auto lookV= lookAt->eye - lookAt->center;
	lookAt->eye= lookAt->center + rot*lookV;
	lookAt->up= rot*lookAt->up;
}

void CameraController::incPitch(double degrees)
{
	auto lookV= lookAt->center - lookAt->eye;
	auto lookDir= vsg::normalize(lookV);
	auto side= vsg::cross(lookDir,lookAt->up);
	auto rot= vsg::rotate(vsg::radians(degrees),side);
	lookAt->eye= lookAt->center - rot*lookV;
	lookAt->up= rot*lookAt->up;
}

void CameraController::setPitch(double degrees)
{
	auto lookV= lookAt->center - lookAt->eye;
	auto lookDir= vsg::normalize(lookV);
	auto side= vsg::cross(lookDir,lookAt->up);
	auto dot= vsg::dot(lookDir,vsg::dvec3(0,0,1));
	auto radians= vsg::radians(90-degrees) - std::acos(dot);
	auto rot= vsg::rotate(-radians,side);
	lookAt->eye= lookAt->center - rot*lookV;
	lookAt->up= rot*lookAt->up;
}

void CameraController::setZoom(int z)
{
	if (z < 0)
		z= 0;
	if (z > maxZoom)
		z= maxZoom;
	zoom= z;
	auto lookV= lookAt->center - lookAt->eye;
	auto lookDir= vsg::normalize(lookV);
	if (zoom == 0) {
		lookAt->eye= lookAt->center - .1*lookDir;
		perspective->nearDistance= .2;
		perspective->farDistance= 4000;
	} else {
		double dist= std::pow(zoom1Dist,zoom);
		lookAt->eye= lookAt->center - dist*lookDir;
		if (dist < 2000) {
			perspective->nearDistance= .2;
			perspective->farDistance= 4000;
		} else {
			perspective->nearDistance= .0001*dist;
			perspective->farDistance= 2*dist;
		}
	}
	if (mstsRoute && mstsRoute->trackLines) {
		if (zoom > 20)
			mstsRoute->trackLines->setAllChildren(true);
		else
			mstsRoute->trackLines->setAllChildren(false);
	}
}

void CameraController::apply(vsg::KeyPressEvent& keyPress)
{
	if (keyPress.handled)
		return;
	if (keyPress.keyBase == vsg::KEY_Up) {
		incPitch(5);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Down) {
		if (fabs(lookAt->up.z) < .01) {
			setPitch(-10);
			if (zoom > 10)
				setZoom(10);
			lookAt->up= vsg::dvec3(0,0,1);
		} else {
			incPitch(-5);
		}
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Left) {
		incHeading(5);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Right) {
		incHeading(-5);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Page_Up) {
		incZoom(-1);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Page_Down) {
		incZoom(1);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_Home) {
		setPitch(0);
		setZoom(0);
		lookAt->up= vsg::dvec3(0,0,1);
		keyPress.handled= true;
	} else if (keyPress.keyBase == vsg::KEY_End) {
		if (zoom < 15)
			setZoom(15);
		setPitch(-90);
		keyPress.handled= true;
	} else if (keyPress.keyBase=='1' && myRailCar && myRailCar->def->inside.size()>0) {
		follow= myRailCar->model;
		auto& inside= myRailCar->def->inside[0];
		followOffset= inside.position;
		vsg::dvec3 position;
		vsg::dquat rotation;
		vsg::dvec3 scale;
		vsg::decompose(follow->matrix,position,rotation,scale);
		prevRotation= rotation;
		auto dir= rotation*vsg::dvec3(1,0,0);
		dir= vsg::dquat(vsg::radians(inside.angle),vsg::dvec3(0,0,1))*dir;
		setZoom(0);
		lookAt->eye= lookAt->center - .1*dir;
		setPitch(inside.vAngle);
		lookAt->up= vsg::dvec3(0,0,1);
		selectedTrain= myTrain;
		selectedRailCar= myRailCar;
		keyPress.handled= true;
	} else if (keyPress.keyBase=='2' && (myTrain || selectedTrain)) {
		if (myTrain) {
			follow= myTrain->firstCar->model;
			selectedTrain= myTrain;
			selectedRailCar= myTrain->firstCar;
		} else {
			follow= selectedTrain->firstCar->model;
			selectedRailCar= selectedTrain->firstCar;
		}
		followOffset= vsg::dvec3(0,0,1.6);
		setZoom(9);
		setPitch(-15);
		lookAt->up= vsg::dvec3(0,0,1);
		keyPress.handled= true;
	} else if (keyPress.keyBase=='3' && (myTrain || selectedTrain)) {
		if (myTrain) {
			follow= myTrain->lastCar->model;
			selectedTrain= myTrain;
			selectedRailCar= myTrain->lastCar;
		} else {
			follow= selectedTrain->lastCar->model;
			selectedRailCar= selectedTrain->lastCar;
		}
		followOffset= vsg::dvec3(0,0,1.6);
		setZoom(9);
		setPitch(-15);
		lookAt->up= vsg::dvec3(0,0,1);
		keyPress.handled= true;
	}
}

void CameraController::apply(vsg::ButtonPressEvent& buttonPress)
{
	if (buttonPress.handled || buttonPress.mask!=vsg::BUTTON_MASK_3)
		return;
	buttonPress.handled= true;
	auto intersector= vsg::LineSegmentIntersector::create(*camera,buttonPress.x,buttonPress.y);
	scene->accept(*intersector);
	if (intersector->intersections.empty())
		return;
	std::sort(intersector->intersections.begin(),intersector->intersections.end(),
	  [](auto& lhs, auto& rhs) { return lhs->ratio < rhs->ratio; });
	auto& intersection= intersector->intersections.front();
	follow= nullptr;
	selectedTrain= nullptr;
	selectedRailCar= nullptr;
	for (auto train: trainList) {
		for (auto car= train->firstCar; car; car=car->next) {
			for (auto& node: intersection->nodePath) {
				auto mt= dynamic_cast<const vsg::MatrixTransform*>(node);
				if (car->model == mt) {
					follow= mt;
					selectedTrain= train;
					selectedRailCar= car;
				}
			}
		}
	}
	auto lookV= lookAt->eye - lookAt->center;
	lookAt->center= intersection->worldIntersection + vsg::dvec3(0,0,1.6);
	if (follow) {
		vsg::dvec3 position;
		vsg::dquat rotation;
		vsg::dvec3 scale;
		vsg::decompose(follow->matrix,position,rotation,scale);
		followOffset= (-rotation)*(lookAt->center-position);
		prevRotation= rotation;
	}
	lookAt->eye= lookAt->center + lookV;
	updateListener();
}

void CameraController::apply(vsg::ScrollWheelEvent& scroll)
{
	if (scroll.handled)
		return;
	scroll.handled= true;
	if (scroll.delta.y < 0)
		incZoom(1);
	if (scroll.delta.y > 0)
		incZoom(-1);
}

void CameraController::apply(vsg::FrameEvent& frame)
{
	if (follow) {
		auto lookV= lookAt->eye - lookAt->center;
		vsg::dvec3 position;
		vsg::dquat rotation;
		vsg::dvec3 scale;
		vsg::decompose(follow->matrix,position,rotation,scale);
		lookAt->center= position + rotation*followOffset;
		lookV= rotation*((-prevRotation)*lookV);
		lookAt->eye= lookAt->center + lookV;
		prevRotation= rotation;
	}
	updateListener();
}

void CameraController::updateListener()
{
	auto lookV= lookAt->center - lookAt->eye;
	auto dir= vsg::vec2(lookV.x,lookV.y);
	auto len= vsg::length(dir);
	if (len > 0)
		listener.update(lookAt->center,dir.x/len,dir.y/len);
	else
		listener.update(lookAt->center,1,0);
}
