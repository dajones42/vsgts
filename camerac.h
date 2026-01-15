//	camera controller
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
#ifndef CAMERAC_H
#define CAMERAC_H

#include <vsg/all.h>

class CameraController : public vsg::Inherit<vsg::Visitor, CameraController>
{
 public:
	CameraController(vsg::ref_ptr<vsg::Camera> cam, vsg::ref_ptr<vsg::Node> node);
	void apply(vsg::KeyPressEvent& keyPress) override;
	void apply(vsg::ButtonPressEvent& buttonPress) override;
	void apply(vsg::ScrollWheelEvent& ScrollPress) override;
	void apply(vsg::FrameEvent& frame) override;
	vsg::ref_ptr<vsg::Node> scene;
	vsg::ref_ptr<vsg::Camera> camera;
	vsg::ref_ptr<vsg::LookAt> lookAt;
	vsg::ref_ptr<vsg::Perspective> perspective;
	vsg::ref_ptr<const vsg::MatrixTransform> follow;
	vsg::dvec3 followOffset;
	vsg::dquat prevRotation;
	int zoom;
	int maxZoom;
	double zoom1Dist;
	void setZoom(int z);
	void incZoom(int dz) { setZoom(zoom+dz); }
	void incHeading(double degrees);
	void incPitch(double degrees);
	void setPitch(double degrees);
	void updateListener();
};
extern vsg::dvec3 clickLocation;

#endif
