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
#include <vsg/all.h>

#include "camerac.h"

CameraController::CameraController(vsg::ref_ptr<vsg::Camera> cam, vsg::ref_ptr<vsg::Node> node) :
  scene(node),
  camera(cam),
  lookAt(cam->viewMatrix.cast<vsg::LookAt>())
{
	auto dist= vsg::length(lookAt->eye - lookAt->center);
	zoom1Dist= 1.4;
	auto z= std::log(dist)/std::log(zoom1Dist);
	maxZoom= zoom= (int)z;
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
	} else {
		double dist= std::pow(zoom1Dist,zoom);
		lookAt->eye= lookAt->center - dist*lookDir;
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
		incPitch(-5);
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
	auto lookV= lookAt->eye - lookAt->center;
	lookAt->center= intersector->intersections.front()->worldIntersection + vsg::dvec3(0,0,1.7);
	lookAt->eye= lookAt->center + lookV;
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
