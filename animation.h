//	Animation controllers
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
#pragma once

#include <vsg/all.h>
#include "mstsroute.h"

struct AnimModelInfo {
	vsg::ref_ptr<vsg::Node> model;
	vsg::ref_ptr<vsg::Animation> animation;
	std::set<vsg::MatrixTransform*> animatedTransforms;
	std::vector<vsg::MatrixTransform*> signalTransforms;
	AnimModelInfo(vsg::ref_ptr<vsg::Node> m, vsg::ref_ptr<vsg::Animation> anim,
	  std::set<vsg::MatrixTransform*> animated) {
		model= m;
		animation= anim;
		animatedTransforms= animated;
	}
	vsg::ref_ptr<vsg::Node> cloneModel(vsg::Animation* animation, MSTSSignal* signal=nullptr);
	void addSignal(MSTSSignal* signal);
};

class TwoStateAnimation : public vsg::Inherit<vsg::Animation, TwoStateAnimation>
{
 public:
	TwoStateAnimation();
	bool update(double simulationTime) override;
};
