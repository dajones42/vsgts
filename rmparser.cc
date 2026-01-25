//	file parser for rmsim
//	should be rewritten to use command reader for each class
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

#include <time.h>
#include <string>
#include <vector>
#include <set>
using namespace std;

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

struct RMParser : public Parser {
	void parseRailCarDef(RailCarDef* def);
	void parseTrain();
	void parseMSTSRoute(string& dir, string& route);
	void parseFile(const char* path);
	Track* findTrack(string& s);
	vsg::Group* rootNode;
	vsg::dvec3 startLocation;
};

void RMParser::parseMSTSRoute(string& dir, string& route)
{
	mstsRoute= new MSTSRoute(dir.c_str(),route.c_str());
	int saveTerrain= 0;
	int activityFlags= -1;
	float ssOffset= 0;
	float ssZOffset= 0;
	float ssPOffset= 0;
	float createWaterLevel= -1e10;
//	osg::Node* ssModel= NULL;
	string pathFile;
	while (getCommand()) {
		try {
			const char* cmd= tokens[0].c_str();
			if (strcasecmp(cmd,"end") == 0)
				break;
			if (strcasecmp(cmd,"filename") == 0) {
				mstsRoute->fileName= tokens[1];
			} else if (strcasecmp(cmd,"center") == 0) {
				mstsRoute->centerTX= getInt(1,-16384,16384);
				mstsRoute->centerTZ= getInt(2,-16384,16384);
				mstsRoute->centerLat= getDouble(3,-90,90);
				mstsRoute->centerLong= getDouble(4,-180,180);
			} else if (strcasecmp(cmd,"saveTerrain") == 0) {
				saveTerrain= 1;
			} else if (strcasecmp(cmd,"ignoreHiddenTerrain") == 0) {
				mstsRoute->ignoreHiddenTerrain= true;
			} else if (strcasecmp(cmd,"signalswitchstands") == 0) {
				mstsRoute->signalSwitchStands= true;
			} else if (strcasecmp(cmd,"createsignals") == 0) {
				mstsRoute->createSignals= true;
//			} else if (strcasecmp(cmd,"trackoverride") == 0) {
//				mstsRoute->overrideTrackModel(
//				  tokens[1],tokens[2]);
			} else if (strcasecmp(cmd,"drawwater") == 0) {
				mstsRoute->drawWater= getInt(1,0,1);
			} else if (strcasecmp(cmd,"waterleveldelta") == 0) {
				mstsRoute->waterLevelDelta=
				  getDouble(1,-100,100);
			} else if (strcasecmp(cmd,"createwater") == 0) {
				createWaterLevel= getDouble(1,-100,1e10);
			} else if (strcasecmp(cmd,"activity") == 0) {
				mstsRoute->activityName= tokens[1];
				activityFlags= getInt(2,-1,8,-1);
			} else if (strcasecmp(cmd,"consist") == 0) {
				mstsRoute->consistName= tokens[1];
			} else if (strcasecmp(cmd,"switchstand") == 0) {
				ssOffset= getDouble(1,-10,10);
				ssZOffset= getDouble(2,-10,10);
				ssPOffset= getDouble(4,-10,10,0);
//				ssModel= find3DModel(tokens[3]);
			} else if (strcasecmp(cmd,"berm") == 0) {
				mstsRoute->bermHeight= getDouble(1,0,100);
			} else if (strcasecmp(cmd,"bridge") == 0) {
				mstsRoute->bridgeBase= getInt(1,0,1);
			} else if (strcasecmp(cmd,"scalerail") == 0) {
				mstsRoute->srDynTrack= getInt(1,0,1);
			} else if (strcasecmp(cmd,"ustracks") == 0) {
				mstsRoute->ustDynTrack= getInt(1,0,1);
			} else if (strcasecmp(cmd,"wire") == 0) {
				mstsRoute->wireHeight= getDouble(1,0,10);
				mstsRoute->wireModelsDir= tokens[2];
			} else if (strcasecmp(cmd,"path") == 0) {
				pathFile= tokens[1];
			} else if (strcasecmp(cmd,"ignorepolygon") == 0) {
				for (int i=1; i<tokens.size(); i++)
					mstsRoute->ignorePolygon.push_back(
					  getDouble(i,-1e10,1e10));
				mstsRoute->ignorePolygon.push_back(
				  getDouble(1,-1e10,1e10));
				mstsRoute->ignorePolygon.push_back(
				  getDouble(2,-1e10,1e10));
			} else if (strcasecmp(cmd,"ignoreshape") == 0) {
				mstsRoute->ignoreShapeMap.insert(
				  make_pair(tokens[1],vsg::dvec3(
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  getDouble(4,-1e10,1e10))));
			} else {
				throw std::invalid_argument("unknown command");
			}
		} catch (const char* message) {
			printError(message);
		} catch (const std::exception& error) {
			printError(error.what());
		}
	}
	mstsRoute->readTiles();
//	mstsRoute->adjustWater(saveTerrain);
	mstsRoute->makeTrack();
	if (mstsRoute->activityName.size()>0 || mstsRoute->consistName.size()>0)
		mstsRoute->loadActivity(rootNode,activityFlags);
//	if (ssModel)
//		mstsRoute->addSwitchStands(ssOffset,ssZOffset,ssModel,rootNode,
//		  ssPOffset);
	if (pathFile.size() > 0)
		mstsRoute->loadPath(pathFile,true);
//	if (createWaterLevel > -1e5)
//		mstsRoute->createWater(createWaterLevel);
	if (symbols.find("wire") != symbols.end())
		mstsRoute->wireTerrain= true;
	mstsRoute->vsgOptions= vsg::Options::create();
	mstsRoute->makeTileMap(rootNode);
	rootNode->addChild(mstsRoute->createTrackLines());
	if (mstsRoute->createSkyBox())
		rootNode->addChild(mstsRoute->skyBox);
}

void RMParser::parseFile(const char* path)
{
	try {
		pushFile(path,0);
	} catch (const char* message) {
		fprintf(stderr,"%s %s\n",message,path);
		return;
	} catch (const std::exception& error) {
		fprintf(stderr,"%s %s\n",error.what(),path);
		return;
	}
	while (getCommand()) {
		try {
			const char* cmd= tokens[0].c_str();
			if (strcasecmp(cmd,"mstsroute") == 0) {
				parseMSTSRoute(tokens[1],tokens[2]);
			} else if (strcasecmp(cmd,"timetable") == 0) {
				timeTable= new TimeTable();
				timeTable->parse((CommandReader&)*this);
			} else if (strcasecmp(cmd,"signal") == 0) {
				SignalParser sparser;
				parseBlock((CommandBlockHandler*)&sparser);
#if 0
			} else if (strcasecmp(cmd,"interlocking") == 0) {
				InterlockingParser iparser;
				parseBlock((CommandBlockHandler*)&iparser);
				interlocking= iparser.interlocking;
			} else if (strcasecmp(cmd,"useroscallsign") == 0) {
				userOSCallSign= tokens[1];
#endif
			} else if (strcasecmp(cmd,"railcar") == 0) {
				if (tokens.size() < 2)
					throw std::invalid_argument(
					  "name missing");
				RailCarDef* def= new RailCarDef();
				railCarDefMap[tokens[1]]= def;
				def->name= string(tokens[1]);
				parseRailCarDef(def);
			} else if (strcasecmp(cmd,"wag") == 0) {
				if (tokens.size() < 3)
					throw "dir and file expected";
				RailCarDef* def= readMSTSWag(
				  tokens[1].c_str(),tokens[2].c_str(),true);
				if (def == NULL)
					throw "cannot find car definition";
				railCarDefMap[tokens[2]]= def;
				parseRailCarDef(def);
			} else if (strcasecmp(cmd,"train") == 0) {
				parseTrain();
			} else if (strcasecmp(cmd,"model3d") == 0) {
				//parseModel3D();
				while (getCommand()) {
					if (tokens[0] == "end")
						break;
				}
#if 0
			} else if (strcasecmp(cmd,"scenery") == 0) {
				osg::Node* model= find3DModel(tokens[1]);
				osg::PositionAttitudeTransform* pat=
				  new osg::PositionAttitudeTransform;
				pat->setPosition(
				  osg::Vec3(getDouble(2,-1e10,1e10),
				    getDouble(3,-1e10,1e10),
				    getDouble(4,-1e10,1e10)));
				pat->setAttitude(
				  osg::Quat(getDouble(5,0,360,0)*M_PI/180,
				  osg::Vec3(0,0,1)));
				pat->addChild(model);
				rootNode->addChild(pat);
				model->setNodeMask(0x3);
			} else if (strcasecmp(cmd,"interlockingmodel") == 0) {
				interlockingModel= find3DModel(tokens[1]);
			} else if (strcasecmp(cmd,"switchstand") == 0) {
				TrackMap::iterator i= trackMap.find(tokens[1]);
				if (i == trackMap.end())
					throw "cannot find track";
				i->second->addSwitchStand(getInt(2,-1,1000000),
				  getDouble(3,-10,10),getDouble(4,-10,10),
				  find3DModel(tokens[5]),rootNode);
#endif
			} else if (strcasecmp(cmd,"throwswitch") == 0) {
				TrackMap::iterator i= trackMap.find(tokens[1]);
				if (i == trackMap.end())
					throw "cannot find track";
				i->second->throwSwitch(
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  getDouble(4,-1e10,1e10));
			} else if (strcasecmp(cmd,"lockswitch") == 0) {
				TrackMap::iterator i= trackMap.find(tokens[1]);
				if (i == trackMap.end())
					throw "cannot find track";
				i->second->lockSwitch(
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  getDouble(4,-1e10,1e10));
			} else if (strcasecmp(cmd,"starttime") == 0) {
				simTime=
				  3600*getInt(1,0,23)+60*getInt(2,0,59);
//			} else if (strcasecmp(cmd,"endtime") == 0) {
//				endTime=
//				  3600*getInt(1,0,1000)+60*getInt(2,0,59);
			} else if (strcasecmp(cmd,"randomize") == 0) {
				long t= time(NULL);
				if (tokens.size() > 1)
					t= atol(tokens[1].c_str());
				fprintf(stderr,"seed %ld\n",t);
				srand48(t);
			} else if (strcasecmp(cmd,"morse") == 0) {
				listener.getMorseConverter()->parse(
				  (CommandReader&)*this);
			} else if (strcasecmp(cmd,"railfan") == 0 ||
			  strcasecmp(cmd,"person") == 0) {
//				fprintf(stderr,"newperson %d %d\n",
//				  Person::stack.size(),Person::stackIndex);
				startLocation= vsg::dvec3(
				  getDouble(1,-1e10,1e10),
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10));
#if 0
				currentPerson.setLocation(currentPerson.location);
				currentPerson.setAngle(getDouble(4,0,360,0));
				Person::stack.push_back(currentPerson);
				Person::stackIndex= Person::stack.size()-1;
#endif
//				fprintf(stderr,"newperson2 %d %d\n",
//				  Person::stack.size(),Person::stackIndex);
			} else if (strcasecmp(cmd,"railfanpath") == 0 ||
			  strcasecmp(cmd,"personpath") == 0) {
//				parsePersonPath();
			} else if (strcasecmp(cmd,"pickwaybills") == 0) {
				selectWaybills(getInt(1,0,100),
				  getInt(2,0,100,0),tokens[3]);
			} else if (strcasecmp(cmd,"location") == 0) {
				TrackMap::iterator j= trackMap.find(tokens[5]);
				if (j == trackMap.end())
					throw "cannot find track";
				j->second->saveLocation(
				  getDouble(1,-1e10,1e10),
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  tokens[4]);
			} else if (strcasecmp(cmd,"align") == 0) {
				Track* t= findTrack(tokens[1]);
				if (t == NULL)
					throw "cannot find track";
				t->alignSwitches(tokens[2],tokens[3]);
			} else {
				throw std::invalid_argument("unknown command");
			}
		} catch (const char* message) {
			printError(message);
		} catch (const std::exception& error) {
			printError(error.what());
		}
	}
}

vsg::dvec3 parseFile(const char* path, vsg::Group* root, int argc, char** argv)
{
	RMParser parser;
	parser.rootNode= root;
	for (int i=1; i<argc; i++) {
		parser.symbols.insert(argv[i]);
//		fprintf(stderr,"symbol %s\n",argv[i]);
	}
	char* p= strrchr((char*)path,'/');
	if (p)
		parser.dir= string(path,p-path);
	parser.parseFile(path);
	if (timeTable) {
		auto start= ttoSim.init(false);
		if (simTime == 0)
			simTime= start;
		timeTable->printHorz(stderr);
	} else if (simTime == 0) {
		simTime= 12*3600;
	}
	return parser.startLocation;
}

void RMParser::parseRailCarDef(RailCarDef* def)
{
	while (getCommand()) {
		try {
			const char* cmd= tokens[0].c_str();
			if (strcasecmp(cmd,"end") == 0)
				break;
			if (strcasecmp(cmd,"axles") == 0) {
				def->axles= getInt(1,2,100);
			} else if (strcasecmp(cmd,"mass")==0 ||
			  strcasecmp(cmd,"mass1")==0) {
				def->mass0= 1e3*getDouble(1,.1,10000);
				def->mass1= 1e3*getDouble(2,.1,10000,
				  def->mass0);
			} else if (strcasecmp(cmd,"rmass")==0 ||
			  strcasecmp(cmd,"mass2")==0) {
				def->mass2= 1e3*getDouble(1,.1,10000);
			} else if (strcasecmp(cmd,"drag0a") == 0) {
				def->drag0a= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"drag0b") == 0) {
				def->drag0b= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"drag1") == 0) {
				def->drag1= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"drag2") == 0) {
				def->drag2= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"drag2a") == 0) {
				def->drag2a= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"area") == 0) {
				def->area= getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"length") == 0) {
				def->length= getDouble(1,.1,1000);
			} else if (strcasecmp(cmd,"offset") == 0) {
				def->offset= getDouble(1,-def->length,
				  def->length);
			} else if (strcasecmp(cmd,"width") == 0) {
				def->width= getDouble(1,.1,1000);
			} else if (strcasecmp(cmd,"maxbforce") == 0) {
				def->maxBForce= 1e3*getDouble(1,0,10000);
			} else if (strcasecmp(cmd,"steamengine") == 0) {
				if (def->engine == NULL) {
					SteamEngine* e= new SteamEngine;
					def->engine= e;
				}
				parseBlock((CommandBlockHandler*)
				  (SteamEngine*)def->engine);
			} else if (strcasecmp(cmd,"dieselengine") == 0) {
				if (def->engine == NULL) {
					DieselEngine* e= new DieselEngine;
					def->engine= e;
				}
				parseBlock((CommandBlockHandler*)
				  (DieselEngine*)def->engine);
			} else if (strcasecmp(cmd,"electricengine") == 0) {
				if (def->engine == NULL) {
					ElectricEngine* e= new ElectricEngine;
					def->engine= e;
				}
				parseBlock((CommandBlockHandler*)
				  (ElectricEngine*)def->engine);
			} else if (strcasecmp(cmd,"part") == 0) {
				def->parts.push_back(
				  RailCarPart(getInt(1,-1,100),
				    getDouble(2,-10000,10000),
				    getDouble(3,-100,100)));
			} else if (strcasecmp(cmd,"insertpart") == 0) {
				int i= getInt(1,0,100);
				int sz= def->parts.size();
				def->parts.push_back(RailCarPart(-1,0,0));
				for (int j=sz-1; j>=0; j--) {
					RailCarPart& p= def->parts[j];
					if (p.parent >= i)
						p.parent++;
					if (j >= i)
						def->parts[j+1]= p;
				}
				def->parts[i]= RailCarPart(getInt(2,-1,100),
				    getDouble(3,-10000,10000),
				    getDouble(4,-100,100));
				if (i < def->axles)
					def->axles++;
			} else if (strcasecmp(cmd,"smoke") == 0) {
#if 0
				RailCarSmoke smoke;
				smoke.position=
				  osg::Vec3f(getDouble(1,-100,100),
				  getDouble(2,-100,100),getDouble(3,-100,100));
				smoke.normal=
				  osg::Vec3f(getDouble(4,-100,100,0),
				  getDouble(5,-100,100,0),
				  getDouble(6,-100,100,1));
				smoke.size= getDouble(7,.001,1,.1);
				smoke.minRate= getDouble(8,0,100,20);
				smoke.maxRate= getDouble(9,0,1000,200);
				smoke.minSpeed= getDouble(10,0,100,1);
				smoke.maxSpeed= getDouble(11,0,100,5);
				def->smoke.push_back(smoke);
#endif
			} else if (strcasecmp(cmd,"model") == 0) {
#if 0
				int i= getInt(1,0,def->parts.size()-1);
				if (def->parts[i].model) {
					osg::Node* model=
					  find3DModel(tokens[2]);
					osg::MatrixTransform* mt=
					  (osg::MatrixTransform*)
					  def->parts[i].model;
					mt->addChild(model);
					fprintf(stderr,"add to part %d %p %p\n",
					  i,model,mt);
				} else {
					def->parts[i].model=
					  find3DModel(tokens[2]);
				}
#endif
			} else if (strcasecmp(cmd,"copy") == 0) {
				RailCarDefMap::iterator i=
				  railCarDefMap.find(tokens[1]);
				if (i == railCarDefMap.end())
					throw "cannot find car definition";
				def->copy(i->second);
			} else if (strcasecmp(cmd,"mstsshape") == 0) {
				MSTSShape shape;
				shape.readFile(makePath().c_str());
				if (tokens.size() > 2)
					shape.printSubobjects();
				shape.createRailCar(def);
			} else if (strcasecmp(cmd,"inside") == 0) {
#if 0
				osg::Image* img= NULL;
				if (tokens.size() > 6)
					img= osgDB::readImageFile(tokens[6]);
#endif
				def->inside.push_back(RailCarInside(
				  getDouble(1,-100,100),getDouble(2,-100,100),
				  getDouble(3,-100,100),
				  getDouble(4,-180,180,0),
				  getDouble(5,-180,180,0),nullptr));
				//fprintf(stderr,"inside img %p\n",img);
			} else if (strcasecmp(cmd,"sound") == 0) {
				def->soundFile= tokens[1];
				def->soundGain= getDouble(2,0,1,1);
			} else if (strcasecmp(cmd,"brakevalve") == 0) {
				def->brakeValve= tokens[1];
			} else if (strcasecmp(cmd,"headlight") == 0) {
				def->headlights.push_back(HeadLight(
				  getDouble(1,-100,100),getDouble(2,-100,100),
				  getDouble(3,-100,100),
				  getDouble(4,0,1,0),
				  getInt(5,0,3,2),
				  getInt(6,0x80000000,0x7fffffff,0xffffffff)));
			} else if (strcasecmp(cmd,"remove") == 0) {
			} else if (strcasecmp(cmd,"copy") == 0) {
			} else if (strcasecmp(cmd,"copypart") == 0) {
			} else if (strcasecmp(cmd,"copywheels") == 0) {
			} else {
				throw std::invalid_argument("unknown command");
			}
		} catch (const char* message) {
			printError(message);
		} catch (const std::exception& error) {
			printError(error.what());
		}
	}
}

#if 0
void RMParser::parsePersonPath()
{
	currentPerson.path.clear();
	while (getCommand()) {
		try {
			const char* cmd= tokens[0].c_str();
			if (strcasecmp(cmd,"end") == 0)
				break;
			currentPerson.path.push_back(osg::Vec3d(
			  getDouble(0,-1e10,1e10),
			  getDouble(1,-1e10,1e10),
			  getDouble(2,-1e10,1e10)-1.7));
		} catch (const char* message) {
			printError(message);
		} catch (const std::exception& error) {
			printError(error.what());
		}
	}
	currentPerson.setLocation(currentPerson.path[0]);
	Person::stack.push_back(currentPerson);
	Person::stackIndex= Person::stack.size()-1;
	if (currentPerson.path.size() > 1) {
		osg::Vec3f dir= currentPerson.path[1]-currentPerson.path[0];
		dir.normalize();
		currentPerson.setAngle(atan2(dir[1],dir[0])*180/3.14159);
	}
}

osg::Node* RMParser::find3DModel(string& s)
{
	ModelMap::iterator i= modelMap.find(s);
	if (i == modelMap.end())
		throw "cannot find model";
	return i->second;
}
#endif

Track* RMParser::findTrack(string& s)
{
	TrackMap::iterator i= trackMap.find(s);
	if (i == trackMap.end())
		throw "cannot find track";
	return i->second;
}

void RMParser::parseTrain()
{
	Train* train= new Train;
	if (tokens.size() >= 2) {
		train->name= tokens[1];
		trainMap[tokens[1]]= train;
	}
	trainList.push_back(train);
	float initAux= 50;
	float initCyl= 50;
	float initEqRes= 0;
	float maxEqRes= 70;
	string brakeValve;
	while (getCommand()) {
		try {
			const char* cmd= tokens[0].c_str();
			if (strcasecmp(cmd,"end") == 0)
				break;
			if (strcasecmp(cmd,"car") == 0) {
				RailCarDefMap::iterator i=
				  railCarDefMap.find(tokens[1]);
				if (i == railCarDefMap.end())
					throw "cannot find car definition";
				RailCarInst* car= new
				  RailCarInst(i->second,rootNode,maxEqRes,
				  brakeValve);
				car->setLoad(getDouble(2,0,1,0));
				car->prev= train->lastCar;
				if (train->lastCar == NULL)
					train->firstCar= car;
				else
					train->lastCar->next= car;
				train->lastCar= car;
				car->rev= getInt(3,0,1,0);
			} else if (strcasecmp(cmd,"wag") == 0) {
				if (tokens.size() < 3)
					throw "dir and file expected";
				RailCarDef* def= readMSTSWag(
				  tokens[1].c_str(),tokens[2].c_str());
				if (def == NULL)
					throw "cannot find car definition";
				RailCarInst* car=
				  new RailCarInst(def,rootNode,maxEqRes,
				  brakeValve);
				car->setLoad(0);
				car->prev= train->lastCar;
				if (train->lastCar == NULL)
					train->firstCar= car;
				else
					train->lastCar->next= car;
				train->lastCar= car;
				car->rev= getInt(3,0,1,0);
			} else if (strcasecmp(cmd,"waybill") == 0) {
				if (train->lastCar == NULL)
					throw "no car for waybill";
				if (1-getDouble(1,0,1) > drand48())
					continue;
				train->lastCar->addWaybill(tokens[5],
				  getDouble(2,0,1),getDouble(3,0,1),
				  getDouble(4,0,1),
				  getInt(6,0,100000,100));
			} else if (strcasecmp(cmd,"xyz") == 0) {
				findTrackLocation(getDouble(1,-1e10,1e10),
				  getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  &train->location);
				train->location.rev= 0;
			} else if (strcasecmp(cmd,"txyz") == 0) {
				if (tokens.size() < 2)
					throw "track name missing";
				TrackMap::iterator i= trackMap.find(tokens[1]);
				if (i == trackMap.end())
					throw "cannot find track";
				i->second->findLocation(getDouble(2,-1e10,1e10),
				  getDouble(3,-1e10,1e10),
				  getDouble(4,-1e10,1e10),
				  &train->location);
				train->location.rev= 0;
			} else if (strcasecmp(cmd,"loc") == 0) {
				if (tokens.size() < 2)
					throw "location name missing";
				bool found= false;
				for (TrackMap::iterator i=trackMap.begin();
				  i!=trackMap.end(); ++i) {
					if (i->second->findLocation(tokens[1],
					  &train->location)) {
						found= true;
						break;
					}
				}
				if (!found)
					throw "cannot find location";
			} else if (strcasecmp(cmd,"reverse") == 0) {
				train->location.rev= 1;
			} else if (strcasecmp(cmd,"solid") == 0) {
				train->modelCouplerSlack= 0;
				train->bControl= 1;
			} else if (strcasecmp(cmd,"speed") == 0) {
				train->modelCouplerSlack= 0;
				train->targetSpeed= getDouble(1,-100,100);
			} else if (strcasecmp(cmd,"decelmult") == 0) {
				train->decelMult= getDouble(1,.01,1);
			} else if (strcasecmp(cmd,"accelmult") == 0) {
				train->accelMult= getDouble(1,.01,1);
			} else if (strcasecmp(cmd,"move") == 0) {
				train->location.move(
				  getDouble(1,-1e10,1e10),0,0);
			} else if (strcasecmp(cmd,"pick") == 0) {
				train->selectRandomCars(getInt(1,1,100),
				  getInt(4,0,100,0),getInt(2,0,100,0),
				  getInt(3,0,100,0));
			} else if (strcasecmp(cmd,"brakes") == 0) {
				maxEqRes= getDouble(1,0,110);
				initEqRes= getDouble(2,0,110);
				initAux= getDouble(3,0,110);
				initCyl= getDouble(4,0,110);
				if (tokens.size() > 5)
					brakeValve= tokens[5];
			} else {
				throw std::invalid_argument("unknown command");
			}
		} catch (const char* message) {
			printError(message);
		} catch (const std::exception& error) {
			printError(error.what());
		}
	}
	float len= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next)
		len+= car->def->length;
	train->location.move(len/2,1,0);
	train->location.edge->occupied++;
	train->endLocation= train->location;
	train->endLocation.move(-len,1,-1);
	train->connectAirHoses();
	if (train->engAirBrake != NULL)
		train->engAirBrake->setEqResPressure(initEqRes);
	if (initCyl > initAux)
		initCyl= initAux;
	float x= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next) {
		car->setLocation(x-car->def->length/2,&train->location);
		x-= car->def->length;
		if (car->airBrake != NULL) {
			car->airBrake->setCylPressure(initCyl);
			car->airBrake->setAuxResPressure(initAux);
			car->airBrake->setPipePressure(initEqRes);
			if (initEqRes > 0)
				car->airBrake->setEmergResPressure(maxEqRes);
			else
				car->airBrake->setEmergResPressure(initEqRes);
		}
		if (initCyl == 0)
			car->handBControl= 1;
	}
	train->calcPerf();
	WLocation loc;
	train->location.getWLocation(&loc);
	fprintf(stderr,"train %s at %lf %lf %f\n",
	  train->name.c_str(),loc.coord[0],loc.coord[1],loc.coord[2]);
}
