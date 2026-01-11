//	classes for rail car parameters and state
//
/*
Copyright © 2021 Doug Jones

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
#ifndef RAILCAR_H
#define RAILCAR_H

#include <string>
#include <vector>
#include <list>
#include <vsg/all.h>

#include "airbrake.h"
#include "locoeng.h"

struct HeadLight {
	float x;
	float y;
	float z;
	float radius;
	int unit;
	unsigned int color;
	HeadLight(float x, float y, float z, float r, int u, unsigned int c) {
		this->x= x;
		this->y= y;
		this->z= z;
		radius= r;
		unit= u;
		color= c;
	};
};

struct RailCarPart {
	float xoffset;		// distance forward of center
	float zoffset;		// distance above rail head
	vsg::ref_ptr<vsg::Node> model;
	int parent;
	RailCarPart(int p, float xo, float zo) {
		xoffset= xo;
		zoffset= zo;
		parent= p;
	};
};

struct RailCarSmoke {
	vsg::vec3 position;
	vsg::vec3 normal;
	float size;
	float minRate;
	float maxRate;
	float minSpeed;
	float maxSpeed;
	RailCarSmoke() {
	};
};

struct RailCarInside {
	vsg::vec3 position;
	float angle;
	float vAngle;
	vsg::Data* image;
	RailCarInside(float px, float py, float pz, float a, float va,
	  vsg::Data* img) {
		position= vsg::vec3(px,py,pz);
		angle= a;
		vAngle= va;
//		image= img;
//		if (image)
//			image->ref();
	};
};

struct RailCarDef {
	std::string name;
	std::string soundFile;
	std::string brakeValve;
	int axles;
	float mass0;		// empty static mass
	float mass1;		// loaded static mass
	float mass2;		// rotating mass equivalent
	float drag0a;
	float drag0b;
	float drag1;
	float drag2;
	float drag2a;
	float area;
	float width;		// bounding box width
	float length;
	float offset;		// offset of model center from car center
	float maxSlack;
	float couplerGap;
	std::vector<RailCarPart> parts;
	std::vector<RailCarSmoke> smoke;
	float maxBForce;
	float soundGain;
	LocoEngine* engine;
	std::vector<RailCarInside> inside;
	std::list<HeadLight> headlights;
	RailCarDef();
	void copy(RailCarDef* other);
	void copyWheels(RailCarDef* other);
	vsg::ref_ptr<vsg::Animation> rodAnimation;
	int nInst;
	std::set<vsg::MatrixTransform*> animatedTransforms;
};

struct Waybill {
	std::string destination;
	vsg::vec3 color;
	vsg::Switch* label;
	int priority;
	Waybill() { label= NULL; priority= 100; };
};

struct RailCarWheel {
	Track::Location location;
	float state;
	float mult;
	RailCarWheel(float radius);
	void move(float distance, int rev);
};

struct RailCarInst {
	struct LinReg {
		double sw;
		double so;
		double soo;
		double sx;
		double sy;
		double sz;
		double sxo;
		double syo;
		double szo;
		double ax;
		double ay;
		double az;
		double bx;
		double by;
		double bz;
		vsg::vec3 up;
		void init();
		void calc();
		void sum(double w, double o, double x, double y, double z,
		  vsg::vec3& u);
	};
	RailCarDef* def;
	LocoEngine* engine;
	std::vector<RailCarWheel> wheels;
	vsg::ref_ptr<vsg::Switch> modelSw;
	vsg::ref_ptr<vsg::MatrixTransform> model;
	std::vector<LinReg*> linReg;
	vsg::ref_ptr<vsg::Animation> rodAnimation;
	std::vector<vsg::ref_ptr<vsg::TransformSampler> > partSamplers;
	int mainWheel;
	float mass;
	float massInv;
	float drag0;
	float drag1;
	float grade;
	float curvature;
	float speed;
	float force;
	float drag;
	float slack;
	float maxSlack;
	float couplerState[2];
	float cA;
	float cB;
	float cC;
	float cR;
	float cU;
	float cG;
	int rev;
	float distance;
	AirBrake* airBrake;
	float handBControl;
	RailCarInst* next;
	RailCarInst* prev;
	Waybill* waybill;
	int animState;
	RailCarInst(RailCarDef* def, vsg::Group* group, float maxEqRes,
	  std::string brakeValve="");
	~RailCarInst();
	void move(float distance);
	void setLocation(float offset, Track::Location* loc);
	void calcForce(float tControl, float dControl, float engBMult,
	  float dt);
	void setLoad(float f);
	float calcBrakes(float dt, int n);
	void setModelsOn() { modelSw->setAllChildren(true); };
	void setModelsOff() { modelSw->setAllChildren(false); };
	void decHandBrakes() {
		handBControl-= .25;
		if (handBControl < 0)
			handBControl= 0;
	}
	void incHandBrakes() {
		handBControl+= .25;
		if (handBControl > 1)
			handBControl= 1;
	}
	void addWaybill(std::string& dest, float r, float g, float b, int p);
	float getMainWheelState() {
		return wheels[mainWheel].state;
	};
	float getMainWheelRadius() {
		return def->parts[mainWheel].zoffset;
	};
	float getCouplerState(bool front) {
		if (rev)
			front= !front;
		return couplerState[front?0:1];
	};
	void addSmoke();
	int getAnimState(int index) {
		if (index<0)
			return 1;
		return (animState&(1<<index))!=0;
	};
	void setHeadLight(int unit, bool rev, bool on);
};

#if 0
struct HeadLightVisitor : public osg::NodeVisitor {
	RailCarDef* model;
	int unit;
	bool rev;
	bool on;
	HeadLightVisitor(RailCarDef* m, int u, bool r, bool o) :
	  osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {
		model= m;
		unit= u;
		rev= r;
		on= o;
	};
	virtual void apply(osg::Node& node);
};
#endif

typedef std::map<std::string,RailCarDef*> RailCarDefMap;
extern RailCarDefMap railCarDefMap;
extern RailCarDef* findRailCarDef(std::string name, bool random);

#endif
