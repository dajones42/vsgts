//	code to read sssimjs json file
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

#include <time.h>
#include <string>
#include <vector>
#include <set>
#include <map>
using namespace std;
#include <iostream>

#include <vsg/all.h>

#include "parser.h"
#include "track.h"
#include "mstsshape.h"
#include "mstsroute.h"
#include "railcar.h"
#include "mstswag.h"
#include "train.h"
#include "timetable.h"
#include "ttosim.h"
#include "signal.h"
#include "listener.h"
#include "interlocking.h"
#include "camerac.h"
#include "vsgjson.h"
#include "mstsace.h"

typedef std::vector<std::pair<int,int> > Conditions;

static void addSignalLocking(int lever, Track::Vertex* v, Track::Edge* e, int depth, Conditions& when)
{
	if (depth > 50) {
		std::cerr<<"addSignalLocking depth exceeded\n";
		return;
	}
	double length= 0;
	while (v->type!=Track::VT_SWITCH && e) {
		if (depth>1 && e->signals.size() > 0) {
			for (auto s: e->signals) {
				for (int i=0; i<interlocking->getNumLevers(); i++) {
					if (s == interlocking->getSignal(i)) {
						for (int j=0; j<depth; j++)
							std::cerr<<" ";
						std::cerr<<"found signal "<<(i+1);
						if (lever < i+1) {
							std::cerr<<" lock when";
							interlocking->addInterlock(lever-1,Interlocking::REVERSE,
							  i,Interlocking::NORMAL);
							for (int j=0; j<when.size(); j++) {
								interlocking->addCondition(when[j].first,
								  when[j].second);
								std::cerr<<" "<<when[j].first+1<<" "<<when[j].second;
							}
						}
						std::cerr<<"\n";
					}
				}
			}
			return;
		}
		length+= e->length;
		if (length> 2e3)
			return;
		e= v->nextEdge(e);
		if (!e)
			break;
		v= e->otherV(v);
	}
	if (!e) {
		for (int i=0; i<depth; i++)
			std::cerr<<" ";
		std::cerr<<"path with no exit signal\n";
		return;
	}
	auto sw= (Track::SwVertex*)v;
	if (sw->edge1 == e) {
		auto sz= when.size();
		auto swLever= sw->hasInterlocking&0xff;
		auto swLock= (sw->hasInterlocking&0xff00)>>8;
		if (swLock > 0) {
			interlocking->addInterlock(lever-1,Interlocking::REVERSE,swLock-1,Interlocking::REVERSE);
			for (int j=0; j<when.size(); j++)
				interlocking->addCondition(when[j].first,when[j].second);
		}
//		for (int i=0; i<depth; i++)
//			std::cerr<<" ";
//		std::cerr<<"facing "<<swLever<<" normal\n";
		when.push_back(pair(swLever-1,Interlocking::NORMAL));
		e= sw->swEdges[sw->mainEdge];
		addSignalLocking(lever,e->otherV(v),e,depth+1,when);
//		for (int i=0; i<depth; i++)
//			std::cerr<<" ";
//		std::cerr<<"facing "<<swLever<<" reverse\n";
		when[sz]= pair(swLever-1,Interlocking::REVERSE);
		e= sw->swEdges[1-sw->mainEdge];
		addSignalLocking(lever,e->otherV(v),e,depth+1,when);
		when.resize(sz);
	} else {
		auto swLever= sw->hasInterlocking&0xff;
//		for (int i=0; i<depth; i++)
//			std::cerr<<" ";
//		std::cerr<<"trailing "<<swLever<<"\n";
		interlocking->addInterlock(lever-1,Interlocking::REVERSE,swLever-1,
		  e==sw->swEdges[sw->mainEdge]?Interlocking::NORMAL:Interlocking::REVERSE);
		for (int j=0; j<when.size(); j++)
			interlocking->addCondition(when[j].first,when[j].second);
		e= sw->edge1;
		addSignalLocking(lever,e->otherV(v),e,depth+1,when);
	}
}

static void parseSignalLock(int lever, string& locks)
{
	std::cerr<<"signallock "<<lever<<" "<<locks<<"\n";
	if (locks == "calc") {
		auto sig= interlocking->getSignal(lever-1);
//		std::cerr<<"signal "<<lever<<" "<<sig<<"\n";
		for (int j=0; j<sig->getNumTracks(); j++) {
			auto loc= sig->getTrack(j);
			Conditions when;
			if (loc.rev)
				addSignalLocking(lever,loc.edge->v1,loc.edge,1,when);
			else
				addSignalLocking(lever,loc.edge->v2,loc.edge,1,when);
		}
	} else {
		while (locks.size() > 0) {
			auto lock= locks;
			auto i= locks.find(",");
			if (i == string::npos) {
				locks= "";
			} else {
				lock= locks.substr(0,i);
				locks= locks.substr(i+1);
			}
			auto lever2= atoi(lock.c_str());
			if (lever2<1 || lever2>interlocking->getNumLevers())
				continue;
			int state= Interlocking::NORMAL | Interlocking::REVERSE;
			if (lock.find("N") != string::npos)
				state= Interlocking::NORMAL;
			else if (lock.find("R") != string::npos)
				state= Interlocking::REVERSE;
			interlocking->addInterlock(lever-1,Interlocking::REVERSE,lever2-1,state);
		}
	}
}

static void createTrains(JsonArray& trains, JsonObject& consists, Track* track, vsg::Group* root)
{
	for (int i=0; i<trains.size(); i++) {
		auto t= trains.getObject(i);
		auto startTime= t.getString("startTime");
		auto entrance= t.getString("entrance");
		auto exit= t.getString("exit");
		auto stops= t.getArray("stops");
		auto s1= timeTable->findStation(entrance);
		auto s2= timeTable->findStation(exit);
		bool readDown= s1->getNumTracks()==2;
		auto name= t.getString("name");
		if (startTime.find(":") != string::npos) {
			Train* train= new Train;
			train->name= name;
			auto consist= consists.getArray(t.getString("consist"));
			for (int j=0; j<consist.size(); j++) {
				auto s= consist.getString(j);
				bool rev= s[0]=='^';
				if (rev)
					s= s.substr(1);
				auto k= s.find("/");
				if (k == string::npos)
					continue;
				string dir= mstsRoute->trainsetDir+mstsRoute->dirSep+s.substr(0,k);
				string file= s.substr(k+1);
				auto def= mstsRoute->loadRailCarDef(dir,file);
				if (!def)
					continue;
				RailCarInst* car= new RailCarInst(def,root,0,def->brakeValve);
				car->setLoad(0);
				car->prev= train->lastCar;
				car->rev= rev;
				if (train->lastCar == NULL)
					train->firstCar= car;
				else
					train->lastCar->next= car;
				train->lastCar= car;
			}
			if (train->firstCar == NULL) {
				cerr<<"empty train "<<name<<"\n";
				delete train;
				continue;
			}
			track->findLocation(entrance,&train->location);
			Track::Location endLoc;
			track->findLocation(exit,&endLoc);
			track->findSPT(endLoc,true);
			auto e= train->location.edge;
			train->location.rev= e->v1->dist < e->v2->dist;
			auto d1= train->location.getDist();
			if (readDown) {
				auto d= d1 + s1->getDistance();
				if (s2->getDistance() < d)
					s2->setDistance(d);
				for (int j=0; j<stops.size(); j++) {
					auto stop= stops.getObject(j).getString("stop");
					auto s= timeTable->findStation(stop);
					if (s) {
						Track::Location loc;
						track->findLocation(stop,&loc);
						d= d1 - loc.getDist() + s1->getDistance();
						if (s->getDistance() < d)
							s->setDistance(d);
					}
				}
			} else {
				auto d= d1 + s2->getDistance();
				if (s1->getDistance() < d)
					s1->setDistance(d);
				for (int j=0; j<stops.size(); j++) {
					auto stop= stops.getObject(j).getString("stop");
					auto s= timeTable->findStation(stop);
					if (s) {
						Track::Location loc;
						track->findLocation(stop,&loc);
						d= loc.getDist() + s2->getDistance();
						if (s->getDistance() < d)
							s->setDistance(d);
					}
				}
			}
			train->setModelsOff();
			train->calcPerf();
			float len= 0;
			for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next)
				len+= car->def->length;
			train->endLocation= train->location;
			train->location.move(len,1,0);
			float x= 0;
			for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next) {
				car->setLocation(x-car->def->length/2,&train->location);
				x-= car->def->length;
			}
			train->targetSpeed= t.getDouble("maxSpeed")/2.23693;
			trainList.push_back(train);
			if (train->name.size() > 0)
				trainMap[train->name]= train;
			train->setOccupied();
		}
	}
}

static void scheduleTrains(JsonArray& trains)
{
	for (int i=0; i<trains.size(); i++) {
		auto t= trains.getObject(i);
		auto startTime= t.getString("startTime");
		auto entrance= t.getString("entrance");
		auto exit= t.getString("exit");
		auto stops= t.getArray("stops");
		auto s1= timeTable->findStation(entrance);
		auto s2= timeTable->findStation(exit);
		bool readDown= s1->getNumTracks()==2;
		auto name= t.getString("name");
		auto ttTrain= timeTable->addTrain(name);
		ttTrain->canThrowSwitches= false;
		ttTrain->route= entrance.substr(0,1)+"-"+exit.substr(0,1);
		auto s1Time= parseTime(startTime);
		if (startTime.find(":") == string::npos) {
			auto prevTrain= timeTable->findTrain(startTime);
			if (!prevTrain)
				continue;
			ttTrain->setPrevTrain(prevTrain);
			s1Time= 0;
			for (int i=0; i<timeTable->getNumRows(); i++) {
				auto time= prevTrain->getSchedLv(i) + prevTrain->getWait(i) + 60;
				if (s1Time < time)
					s1Time= time;
			}
		}
		auto s2Time= s1Time+60;
		int s3Time= 0;
		int s3Wait= 30;
		tt::Station* s3= nullptr;
		int s4Time= 0;
		int s4Wait= 30;
		tt::Station* s4= nullptr;
		if (stops.size() > 0) {
			auto stop= stops.getObject(0);
			s3= timeTable->findStation(stop.getString("stop"));
			s3Time= parseTime(stop.getString("stopTime"));
			s3Wait= stop.getInt("stopWait",30);
			s2Time= s3Time+60;
			stop= stops.getObject(1);
			if (stop.getString("stop").size() > 0) {
				s4= timeTable->findStation(stop.getString("stop"));
				s4Time= parseTime(stop.getString("stopTime"));
				s4Wait= stop.getInt("stopWait",30);
				s2Time= s4Time+60;
			}
		}
		if (readDown) {
			ttTrain->setReadDown(true);
			ttTrain->setSchedTime(s1,s1Time-60,s1Time,0);
			if (s3)
				ttTrain->setSchedTime(s3,s3Time-s3Wait,s3Time,0);
			if (s4)
				ttTrain->setSchedTime(s4,s4Time-s4Wait,s4Time,0);
			ttTrain->setSchedTime(s2,s2Time,s2Time+30,0);
		} else {
			ttTrain->setReadDown(false);
			ttTrain->setSchedTime(s2,s2Time,s2Time+30,0);
			if (s4)
				ttTrain->setSchedTime(s4,s4Time-s4Wait,s4Time,0);
			if (s3)
				ttTrain->setSchedTime(s3,s3Time-s3Wait,s3Time,0);
			ttTrain->setSchedTime(s1,s1Time-60,s1Time,0);
		}
	}
}

struct ModelBoardLight {
	int lever= 0;
	TrackCircuit* trackCircuit= nullptr;
	vsg::Switch* lightSwitch= nullptr;
	ModelBoardLight(int l, TrackCircuit* tc, vsg::Switch* sw) {
		lever= l;
		trackCircuit= tc;
		lightSwitch= sw;
	}
};
static std::vector<ModelBoardLight> modelBoardLights;

static void makeModelBoard(JsonObject mb, vsg::dvec3 center, double angle, vsg::Group* root)
{
	if (!mb.object)
		return;
	string path= mstsRoute->routeDir+mstsRoute->dirSep+"sssim"+mstsRoute->dirSep+mb.getString("image");
	std::cerr<<"modelboard path "<<path<<"\n";
	auto image= readCacheACEFile(path.c_str());
	if (!image)
		return;
	auto builder= vsg::Builder::create();
	vsg::StateInfo state;
	state.image= image;
	state.lighting= false;
	state.two_sided= true;
	vsg::GeometryInfo geom;
	auto width= mb.getDouble("width");
	auto height= mb.getDouble("height");
	auto vOffset= mb.getDouble("vOffset");
	geom.position.set(0,0,vOffset);
	geom.dx.set(0,-width,0);
	geom.dy.set(0,0,-height);
//	geom.color= vsg::vec4(1,1,1,1);
	std::cerr<<"pos "<<geom.position<<"\n";
	std::cerr<<"dx "<<geom.dx<<"\n";
	std::cerr<<"dy "<<geom.dy<<"\n";
	auto quad= builder->createQuad(geom, state);
	auto dist= -mb.getDouble("distance");
	auto mt= vsg::MatrixTransform::create();
	if (dist < 0) {
		angle+= 180;
		dist= -dist;
	}
	mt->matrix= vsg::translate(center) *
	  vsg::rotate(vsg::radians(angle),vsg::dvec3(0,0,1)) * vsg::translate(vsg::dvec3(dist,0,0));
	mt->addChild(quad);
	root->addChild(mt);
	std::cerr<<"matrix "<<mt->matrix<<"\n";
	std::cerr<<"offset "<<dist<<" "<<
	  (vsg::rotate(vsg::radians(angle),vsg::dvec3(0,0,1)) * vsg::translate(vsg::dvec3(dist,0,0)))<<"\n";
	auto lmt= vsg::MatrixTransform::create();
	lmt->matrix= vsg::translate(vsg::dvec3(0,0,vOffset)) * vsg::rotate(vsg::radians(90.),vsg::dvec3(0,1,0));
	mt->addChild(lmt);
	auto lights= mb.getArray("lights");
	for (int i=0; i<lights.size(); i++) {
		auto light= lights.getObject(i);
		auto radius= light.getDouble("radius");
		auto x= light.getDouble("x");
		auto y= light.getDouble("y");
		auto colorHexS= light.getString("color");
		auto lever= light.getInt("lever");
		auto trackCircuit= light.getString("trackCircuit");
		auto colorHex= strtol(colorHexS.c_str(),nullptr,16);
		auto red= ((colorHex&0xff0000)>>16)/255.;
		auto green= ((colorHex&0xff00)>>8)/255.;
		auto blue= (colorHex&0xff)/255.;
		auto lightPoint= createLightPoint(vsg::vec3(-y,-x,-.01),vsg::vec4(red,green,blue,1),radius,
		  mstsRoute->vsgOptions);
		auto sw= vsg::Switch::create();
		sw->addChild(false,lightPoint);
		lmt->addChild(sw);
		if (lever > 0)
			modelBoardLights.push_back(ModelBoardLight(lever,nullptr,sw));
		if (trackCircuit.size() > 0)
			modelBoardLights.push_back(ModelBoardLight(0,TrackCircuit::get(trackCircuit),sw));
	}
}

void updateModelBoardLights()
{
	if (!interlocking)
		return;
	for (auto& mbl: modelBoardLights) {
		if (mbl.lever > 0) {
			auto signal= interlocking->getSignal(mbl.lever-1);
			if (signal)
				mbl.lightSwitch->setAllChildren(signal->getIndication()!=Signal::STOP);
		}
		if (mbl.trackCircuit)
			mbl.lightSwitch->setAllChildren(mbl.trackCircuit->occupied>0);
	}
}

void makeTrackCircuit(Signal* signal, string name)
{
	auto flip= name.substr(0,1) == "^";
	if (flip)
		name= name.substr(1);
	auto tc= TrackCircuit::get(name);
	for (int i=0; i<signal->getNumTracks(); i++) {
		auto loc= signal->getTrack(i);
		auto e= loc.edge;
		auto v= (flip?!loc.rev:loc.rev) ? e->v1 : e->v2;
		while (e) {
			e->trackCircuit= tc;
			std::cerr<<"tc "<<name<<" "<<e<<"\n";
			e= v->nextEdge(e);
			if (!e || e->signals.size()>0)
				break;
			v= e->otherV(v);
		}
	}
}

void loadSSsim(vsg::ref_ptr<vsg::Object> topobj, vsg::Group* root)
{
	JsonObject top(topobj);
	auto centerTX= top.getInt("centerTX");
	auto centerTZ= top.getInt("centerTZ");
	auto du= 2048*(centerTX-mstsRoute->centerTX);
	auto dv= 2048*(centerTZ-mstsRoute->centerTZ);
	std::cerr<<"msts center "<<mstsRoute->centerTX<<" "<<mstsRoute->centerTZ<<"\n";
	std::cerr<<"sssim center "<<centerTX<<" "<<centerTZ<<" "<<du<<" "<<dv<<"\n";
	Track* track= trackMap[mstsRoute->routeID];
	for (auto e: track->edgeList)
		e->signals.clear();
	if (!timeTable)
		timeTable= new tt::TimeTable();
	interlocking= new Interlocking(0);
	map<int,string> signalLocks;
	vector<tt::Station*> stations;
	map<Signal*,string> trackCircuitMap;
	auto mapObjects= top.getArray("mapObjects");
	for (int i=0; i<mapObjects.size(); i++) {
		auto o= mapObjects.getObject(i);
		auto type= o.getString("type");
		auto u= o.getDouble("u")+du;
		auto v= o.getDouble("v")+dv;
		if (type == "camera") {
			Track::Location loc;
			track->findLocation(u,v,&loc);
			WLocation wl;
			loc.getWLocation(&wl);
			auto y= o.getDouble("y");
			auto dx= wl.coord[0] - u;
			auto dy= wl.coord[1] - v;
			auto angle= atan2(dy,dx) + M_PI;
			angle= o.getDouble("angle",angle*180/M_PI);
			std::cerr<<"camera "<<u<<" "<<v<<" "<<wl.coord[2]<<" "<<y<<" "<<angle<<"\n";
			auto center= vsg::dvec3(u,v,wl.coord[2]+y);
			myCameraController->setHome(center,angle-180);
			makeModelBoard(o.getObject("modelBoard"),center,angle-180,root);
		} else if (type == "location") {
			auto name= o.getString("name");
			track->saveLocation(u,v,-1,name);
			std::cerr<<"location "<<name<<" "<<u<<" "<<v<<"\n";
			Track::Location loc;
			track->findLocation(name,&loc);
			WLocation wl;
			loc.getWLocation(&wl);
			std::cerr<<" wloc "<<wl.coord<<" "<<loc.offset<<" "<<loc.edge<<"\n";
			auto s= timeTable->addStation(name);
			s->setCallSign(name.substr(0,2));
			s->setNumTracks(2+o.getInt("column"));
			s->setDistance(0);
			stations.push_back(s);
		} else if (type == "switch") {
			auto sw= track->findSwitch(u,v,0);
			auto lever= o.getInt("lever");
			if (!sw || lever==0)
				continue;
			interlocking->addSwitch(lever-1,sw,0);
			interlocking->setColor(lever-1,0,0,0);
			sw->hasInterlocking= lever;
			auto lock= atoi(o.getString("lock").c_str());
			if (lock > 0) {
				interlocking->setColor(lock-1,0,0,1);
				interlocking->addInterlock(lock-1,Interlocking::REVERSE,
				  lever-1,Interlocking::NORMAL|Interlocking::REVERSE);
				sw->hasInterlocking+= lock<<8;
			}
		} else if (type == "signal") {
			auto lever= o.getInt("lever");
//			std::cerr<<"signal "<<lever<<" "<<u<<" "<<v<<"\n";
			Track::Location loc;
			track->findLocation(u,v,&loc);
			loc.rev= 1-o.getInt("direction");
			if (loc.edge) {
//				std::cerr<<" "<<loc.offset<<" "<<loc.edge->length<<" "<<loc.rev<<"\n";
				auto signal= new Signal(lever?Signal::STOP:Signal::CLEAR);
				signal->addTrack(&loc);
				loc.edge->signals.push_back(signal);
				if (lever) {
					interlocking->setColor(lever-1,1,0,0);
					interlocking->setSignal(lever-1,signal);
					auto lock= o.getString("lock");
					if (lock.size() > 0)
						signalLocks[lever]= lock;
				}
				auto trackCircuit= o.getString("trackCircuit");
				if (trackCircuit.size() > 0)
					trackCircuitMap[signal]= trackCircuit;
			}
		}
	}
	for (auto i: signalLocks)
		parseSignalLock(i.first,i.second);
	for (auto i: trackCircuitMap)
		makeTrackCircuit(i.first,i.second);
	for (auto i: track->switchMap)
		i.second->hasInterlocking= 1;
	auto consists= top.getObject("consists");
	auto trains= top.getArray("trains");
	createTrains(trains,consists,track,root);
	multimap<double,tt::Station*> sortMap;
	for (auto s: stations)
		sortMap.insert(pair(s->getDistance(),s));
	for (auto i: sortMap) {
		auto s= i.second;
		timeTable->addRow(s);
		std::cerr<<"station "<<s->getName()<<" "<<s->getDistance()<<" "<<s->getNumTracks()<<"\n";
	}
	scheduleTrains(trains);
}
