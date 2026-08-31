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

void parseSignalLock(int lever, string& locks)
{
	while (locks.size() > 0) {
		auto lock= locks;
		auto i= locks.find(",");
		if (i == string::npos) {
			locks= "";
		} else {
			lock= locks.substr(0,i-1);
			locks= locks.substr(i);
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

typedef std::vector<std::pair<int,int> > Conditions;

void addSignalLocking(int lever, Track::Vertex* v, Track::Edge* e, int depth, Conditions& when)
{
	while (v->type!=Track::VT_SWITCH && e) {
		if (depth>1 && e->signals.size() > 0) {
			for (auto s: e->signals) {
				for (int i=0; i<interlocking->getNumLevers(); i++) {
					if (s == interlocking->getSignal(i)) {
//						for (int j=0; j<depth; j++)
//							std::cerr<<" ";
//						std::cerr<<"found signal "<<(i+1);
						if (lever < i+1) {
//							std::cerr<<" lock when";
							interlocking->addInterlock(lever-1,Interlocking::REVERSE,
							  i,Interlocking::NORMAL);
							for (int j=0; j<when.size(); j++) {
								interlocking->addCondition(when[j].first,
								  when[j].second);
//								std::cerr<<" "<<when[j].first+1<<" "<<when[j].second;
							}
						}
//						std::cerr<<"\n";
					}
				}
			}
			return;
		}
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
	if (!timeTable)
		timeTable= new tt::TimeTable();
	interlocking= new Interlocking(0);
	map<int,string> signalLocks;
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
			std::cerr<<"camera "<<u<<" "<<v<<" "<<wl.coord[2]<<" "<<y<<"\n";
			myCameraController->setHome(vsg::dvec3(u,v,wl.coord[2]+y));
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
			s->setDistance(o.getInt("column"));
			timeTable->addRow(s);
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
			if (lever == 0)
				continue;
//			std::cerr<<"signal "<<lever<<" "<<u<<" "<<v<<"\n";
			interlocking->setColor(lever-1,1,0,0);
			Track::Location loc;
			track->findLocation(u,v,&loc);
			loc.rev= 1-o.getInt("direction");
			if (loc.edge) {
//				std::cerr<<" "<<loc.offset<<" "<<loc.edge->length<<" "<<loc.rev<<"\n";
				auto signal= new Signal;
				signal->addTrack(&loc);
				loc.edge->signals.push_back(signal);
				interlocking->setSignal(lever-1,signal);
				auto lock= o.getString("lock");
				if (lock.size() > 0)
					signalLocks[lever]= lock;
			}
		}
	}
	for (auto i: signalLocks) {
//		std::cerr<<"signallock "<<i.first<<" "<<i.second<<"\n";
		if (i.second != "calc") {
			parseSignalLock(i.first,i.second);
		} else {
			auto sig= interlocking->getSignal(i.first-1);
//			std::cerr<<"signal "<<i.first<<" "<<sig<<"\n";
			for (int j=0; j<sig->getNumTracks(); j++) {
				auto loc= sig->getTrack(j);
				Conditions when;
				if (loc.rev)
					addSignalLocking(i.first,loc.edge->v1,loc.edge,1,when);
				else
					addSignalLocking(i.first,loc.edge->v2,loc.edge,1,when);
			}
		}
	}
	for (auto i: track->switchMap)
		i.second->hasInterlocking= 1;
	auto consists= top.getObject("consists");
	auto trains= top.getArray("trains");
	for (int i=0; i<trains.size(); i++) {
		auto t= trains.getObject(i);
		Train* train= new Train;
		train->name= t.getString("name");
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
			cerr<<"empty train\n";
			delete train;
			continue;
		}
		auto entrance= t.getString("entrance");
		auto exit= t.getString("exit");
		track->findLocation(entrance,&train->location);
		Track::Location endLoc;
		track->findLocation(exit,&endLoc);
		track->findSPT(endLoc,true);
		auto e= train->location.edge;
		train->location.rev= e->v1->dist < e->v2->dist;
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
		auto startTime= t.getString("startTime");
		auto ttTrain= timeTable->addTrain(train->name);
		ttTrain->canThrowSwitches= false;
		ttTrain->route= entrance.substr(0,1)+"-"+exit.substr(0,1);
		auto s1= timeTable->findStation(entrance);
		auto s1Time= parseTime(startTime);
		auto s2= timeTable->findStation(exit);
		auto s2Time= s1Time+60;
		auto s3Time= 0;
		tt::Station* s3= nullptr;
		auto stops= t.getArray("stops");
		if (stops.size() > 0) {
			auto stop= stops.getObject(0);
			s3= timeTable->findStation(stop.getString("stop"));
			s3Time= parseTime(stop.getString("stopTime"));
			s2Time= s3Time+60;
		}
		if (s1->getDistance() < s2->getDistance()) {
			ttTrain->setReadDown(true);
			ttTrain->setSchedTime(s1,s1Time-60,s1Time,0);
			if (s3)
				ttTrain->setSchedTime(s3,s3Time-30,s3Time,0);
			ttTrain->setSchedTime(s2,s2Time,s2Time+30,0);
		} else {
			ttTrain->setReadDown(false);
			ttTrain->setSchedTime(s2,s2Time,s2Time+30,0);
			if (s3)
				ttTrain->setSchedTime(s3,s3Time-30,s3Time,0);
			ttTrain->setSchedTime(s1,s1Time-60,s1Time,0);
		}
	}
}
