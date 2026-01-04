//	code for loading MSTS world files
//
/*
Copyright © 2021,2025 Doug Jones

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
#include <vsg/all.h>
#include <string>
#include <vector>
using namespace std;

#include "mstsroute.h"
#include "mstsace.h"
#include "mstsfile.h"
#include "mstsbfile.h"
#include "mstsshape.h"
#include "trackshape.h"

//	loads the models for a tile
void MSTSRoute::loadModels(Tile* tile)
{
	if (tile->models)
		return;
	scoped_lock lock {loadMutex};
	tile->models= vsg::Group::create();
	float x0= 2048*(float)(tile->x-centerTX);
	float z0= 2048*(float)(tile->z-centerTZ);
	char buf[100];
	sprintf(buf,"w%+6.6d%+6.6d.w",tile->x,tile->z);
	string path= worldDir+dirSep+buf;
//	fprintf(stderr,"loadModels from %s %f %f\n",path.c_str(),x0,z0);
	if (readBinWFile(path.c_str(),tile,x0,z0) == 0) {
	try {
		MSTSFile file;
		file.readFile(path.c_str());
//		fprintf(stderr,"file read\n");
		MSTSFileNode* wf= file.find("Tr_Worldfile");
		if (!wf) {
			fprintf(stderr,"%s not a MSTS world file?",
			  path.c_str());
			return;
		}
		for (MSTSFileNode* node=wf->children; node!=NULL;
		  node=node->next) {
//			fprintf(stderr," %p %p %p\n",
//			  node,node->value,node->next);
			if (node->value==NULL ||
			  (*(node->value)!="TrackObj" &&
			   *(node->value)!="Static" &&
			   *(node->value)!="Transfer" &&
			   *(node->value)!="Forest" &&
			   *(node->value)!="Dyntrack" &&
			   *(node->value)!="Signal" &&
			   *(node->value)!="LevelCr" &&
			   *(node->value)!="Speedpost" &&
			   *(node->value)!="Hazard" &&
			   *(node->value)!="CollideObject" &&
			   *(node->value)!="Gantry")) {
#if 0
				MSTSFileNode* file= node->next ?
				  node->next->children->find("FileName") : NULL;
				if (node->value!=NULL)
					fprintf(stderr,"unhandled %s %s\n",
					  node->value->c_str(),
					  file&&file->getChild(0)->value?
					  file->getChild(0)->value->c_str():"");
#endif
				continue;
			}
			MSTSFileNode* next= node->next;
			MSTSFileNode* file= next->children->find("FileName");
			MSTSFileNode* pos= next->children->find("Position");
			MSTSFileNode* qdir= next->children->find("QDirection");
			if (pos==NULL || qdir==NULL)
				continue;
			vsg::ref_ptr<vsg::Node> model;
			if (*(node->value)=="TrackObj" && file!=NULL) {
				Track::SwVertex* swVertex= NULL;
				if (next->children->find("JNodePosn") != NULL) {
					MSTSFileNode* uid=
					  next->children->find("UiD");
					if (uid != NULL) {
						int swid=
						  atoi(uid->getChild(0)->
						  value->c_str());
						SwVertexMap::iterator i=
						  tile->swVertexMap.find(swid);
						if (i !=
						  tile->swVertexMap.end()) {
							swVertex= i->second;
							//fprintf(stderr,
							//  "swid %p %d %p\n",
							//  tile,swid,swVertex);
						}
					}
				}
				model= loadTrackModel(file->getChild(0)->value,
				  swVertex);
			} else if (*(node->value)=="Dyntrack") {
				model= makeDynTrack(next);
			} else if (*(node->value)=="Transfer") {
//				model= makeTransfer(next,
//				  file->getChild(0)->value,tile,pos,qdir);
			} else if (*(node->value)=="Forest") {
				model= makeForest(next,tile,pos,qdir);
			} else if (*(node->value)=="Hazard" && file!=NULL) {
				model=
				  loadHazardModel(file->getChild(0)->value);
#if 0
			} else if (*(node->value)=="Signal") {
				MSTSSignal* signal= findSignalInfo(next);
				model=
				  loadStaticModel(file->getChild(0)->value,
				  signal);
				if (model && signalSwitchStands)
				  model= attachSwitchStand(tile,model,
				    x0+atof(pos->getChild(0)->value->c_str()),
				    z0+atof(pos->getChild(2)->value->c_str()),
				    atof(pos->getChild(1)->value->c_str()));
#endif
			} else if (file != NULL) {
				model=
				  loadStaticModel(file->getChild(0)->value);
			}
			if (!model)
				continue;
#if 0
			if (file && file->getChild(0)->value &&
			  (ignorePolygon.size()>0 || ignoreShapeMap.size()>0) &&
			  ignoreShape(file->getChild(0)->value,
			  x0+atof(pos->getChild(0)->value->c_str()),
			  z0+atof(pos->getChild(2)->value->c_str()),
			  atof(pos->getChild(1)->value->c_str())))
				continue;
#endif
			vsg::dquat q(
			  -atof(qdir->getChild(0)->value->c_str()),
			  -atof(qdir->getChild(1)->value->c_str()),
			  -atof(qdir->getChild(2)->value->c_str()),
			  atof(qdir->getChild(3)->value->c_str()));
			double x= x0+atof(pos->getChild(0)->value->c_str());
			double y= z0+atof(pos->getChild(2)->value->c_str());
			double z= atof(pos->getChild(1)->value->c_str());
			vsg::ref_ptr<vsg::MatrixTransform> mt=
			  vsg::MatrixTransform::create();
			mt->matrix= vsg::dmat4(1,0,0,0, 0,0,1,0, 0,1,0,0, x,y,z,1) * vsg::rotate(q);
#if 0
			if (file && file->getChild(0)->value)
				fprintf(stderr,"%s %lf %lf %lf\n",
				  file->getChild(0)->value->c_str(),x,y,z);
			fprintf(stderr,"quat %lf %lf %lf %lf\n",
			  q.x,q.y,q.z,q.w);
			fprintf(stderr,"mat0 %lf %lf %lf %lf\n",
			  mt->matrix[0][0],mt->matrix[0][1],
			  mt->matrix[0][2],mt->matrix[0][3]);
			fprintf(stderr,"mat1 %lf %lf %lf %lf\n",
			  mt->matrix[1][0],mt->matrix[1][1],
			  mt->matrix[1][2],mt->matrix[1][3]);
			fprintf(stderr,"mat2 %lf %lf %lf %lf\n",
			  mt->matrix[2][0],mt->matrix[2][1],
			  mt->matrix[2][2],mt->matrix[2][3]);
			fprintf(stderr,"mat3 %lf %lf %lf %lf\n",
			  mt->matrix[3][0],mt->matrix[3][1],
			  mt->matrix[3][2],mt->matrix[3][3]);
#endif
			mt->addChild(model);
//			if (*(node->value)=="Static" &&
//			  file && file->getChild(0)->value) {
//				mt->setName(*file->getChild(0)->value);
//				mt->setNodeMask(0x10);
//			}
			tile->models->addChild(mt);
		}
	} catch (const char* msg) {
		fprintf(stderr,"loadModels caught %s %s\n",msg,path.c_str());
	} catch (const std::exception& error) {
		fprintf(stderr,"loadModels caught %s %s\n",
		  error.what(),path.c_str());
	}
	}
//	makeWater(tile,waterLevelDelta-1,"waterbot.ace",0);
//	makeWater(tile,waterLevelDelta-.5,"watermid.ace",1);
//	makeWater(tile,waterLevelDelta,"watertop.ace",2);
//	fprintf(stderr,"tile models %d\n",tile->models->getNumChildren());
//	cleanStaticModelMap();
//	fprintf(stderr,"cleanStatic\n");
//	cleanACECache();
//	fprintf(stderr,"cleanACE\n");
}

//	reads a binary world file
int MSTSRoute::readBinWFile(const char* wfilename, Tile* tile,
  float x0, float z0)
{
	MSTSBFile reader;
	if (reader.open(wfilename))
		return 0;
//	fprintf(stderr,"%s is binary %d\n",wfilename,reader.compressed);
#if 0
	Byte buf[16];
	reader.getBytes(buf,16);
	int prevCode= 0;
	int remainingBytes= 0;
	float posX,posY,posZ,qDirX,qDirY,qDirZ,qDirW;
	string filename;
	int uid= 0;
	bool print= false;
	bool visible= false;
	TrackSections trackSections;
	float width= 0;
	float height= 0;
	for (;;) {
		int code= reader.getShort();
		int flags= reader.getShort();
		int len= reader.getInt();
		if (code == 0)
			break;
		//fprintf(stderr,"%d %d\n",code,len);
		switch (code) {
		 case 75: // world_file
			reader.getString();
			break;
		 case 6: // dyntrack
		 case 3: // static
		 case 56: // gantry?
		 case 63: // transfer
			prevCode= code;
			remainingBytes= 2*len+8 -1 - reader.getString().size();
			filename= "";
			//fprintf(stderr,"static %d %d %x %d\n",code,len,flags,
			//  remainingBytes);
			//reader.getBytes(NULL,len);
			break;
		 case 5: // trackobj
			prevCode= code;
			remainingBytes= 2*len+8 -1 - reader.getString().size();
			filename= "";
			//fprintf(stderr,"trackobj %d %d %x %d\n",
			//code,len,flags,remainingBytes);
			//reader.getBytes(NULL,len);
			break;
		 case 8: // forest
//			fprintf(stderr,"forest %d %d %x\n",code,len,flags);
			reader.getBytes(NULL,len);
			remainingBytes= len+8;
			break;
		 case 11: // collide
			//fprintf(stderr,"collide %d %d %x\n",code,len,flags);
			reader.getBytes(NULL,len);
			remainingBytes= len+8;
			break;
		 case 17: // signal
			prevCode= code;
			remainingBytes= 2*len+8 -1 - reader.getString().size();
			//fprintf(stderr,"signal %d %d %x\n",code,len,flags);
			//reader.getBytes(NULL,len);
			break;
		 case 60: // platform
			//fprintf(stderr,"platform %d %d %x\n",code,len,flags);
			reader.getBytes(NULL,len);
			remainingBytes= len+8;
			break;
		 case 61: // siding?
			//fprintf(stderr,"platform %d %d %x\n",code,len,flags);
			reader.getBytes(NULL,len);
			remainingBytes= len+8;
			break;
		 case 62: // levelcr
			prevCode= code;
			remainingBytes= 2*len+8 -1 - reader.getString().size();
			//print= true;
			//fprintf(stderr,"levelcr %d %d %x\n",code,len,flags);
			//reader.getBytes(NULL,len);
			break;
		 case 64: // speedpost
			prevCode= code;
			remainingBytes= 2*len+8 -1 - reader.getString().size();
			//fprintf(stderr,"speedpost %d %d %x\n",code,len,flags);
			//reader.getBytes(NULL,len);
			break;
		 case 65: // hazard
			//fprintf(stderr,"hazard %d %d %x\n",code,len,flags);
			reader.getBytes(NULL,len);
			remainingBytes= len+8;
			break;
		 case 76: // watermark
			//fprintf(stderr,"watermark %d %d %x\n",code,len,flags);
			remainingBytes= len+8;
			reader.getBytes(NULL,len);
			break;
		 case 95: // filename
			reader.getString();
			{
				int n= reader.getShort();
				filename= reader.getString(n);
				if (print)
					fprintf(stderr,"  filename %d %ld %s\n",
					  n,filename.size(),
					  filename.c_str());
			}
			break;
		 case 97: // position
			reader.getString();
			posX= reader.getFloat();
			posY= reader.getFloat();
			posZ= reader.getFloat();
			if (print)
				fprintf(stderr,"  position %f %f %f\n",
				  posX,posY,posZ);
			break;
		 case 645: // qdirection
			reader.getString();
			qDirX= reader.getFloat();
			qDirY= reader.getFloat();
			qDirZ= reader.getFloat();
			qDirW= reader.getFloat();
			if (print)
				fprintf(stderr,"  qdir %f %f %f %f\n",
				  qDirX,qDirY,qDirZ,qDirW);
			break;
		 case 279: // viewdbsphere
		 case 284: // vdbidcount
		 case 280: // radius
		 case 283: // vdbid
			reader.getBytes(NULL,len);
			break;
		 case 101: // static detaillevel
		 case 104: // static flags
		 case 105: // collide flags
		 case 119: // sectionidx
			//reader.getString();
			//reader.getInt();
			//fprintf(stderr," %d %x\n",code,reader.getInt());
			reader.getBytes(NULL,len);
			break;
		 case 193: // elevation
			//reader.getString();
			//reader.getFloat();
			//fprintf(stderr," %d %f\n",code,reader.getFloat());
			reader.getBytes(NULL,len);
			break;
		 case 108: // uid
			reader.getString();
			uid= reader.getInt();
			break;
		 case 109: // tracksections
			reader.getString();
			trackSections.clear();
			remainingBytes+= len-1;
			break;
		 case 110: // tracksection
			if (print)
				fprintf(stderr,"tracksection %d %d %x\n",
				  code,len,flags);
			if (len == 26) {
				reader.getString();
				reader.getShort();//section curve code
				reader.getShort();//section curve flags
				reader.getInt();//section curve length
				reader.getString();
				int flag= reader.getInt();//section curve flag
				int id= reader.getInt();//section id
				float d= reader.getFloat();
				float r= reader.getFloat();
				if (d!=0 || r!=0)
					trackSections.push_back(
					  TrackSection(d,r));
				if (print)
				fprintf(stderr," %f %f %d %d %x\n",d,r,
				  (int)trackSections.size(),id,flag);
			} else {
				reader.getBytes(NULL,len);
			}
			break;
		 case 852: // width
			if (len == 5) {
				reader.getString();
				width= reader.getFloat();
			} else {
				reader.getBytes(NULL,len);
			}
			break;
		 case 853: // height
			if (len == 5) {
				reader.getString();
				height= reader.getFloat();
			} else {
				reader.getBytes(NULL,len);
			}
			break;
		 case 823:  // levelCrData
			reader.getString();
			visible= ((reader.getInt()&0x1) == 0);
			reader.getInt();
			//fprintf(stderr," %d %x %x\n",code,reader.getInt(),
			//  reader.getInt());
			break;
		 default:
//			if (print || remainingBytes<0)
//				fprintf(stderr,"unknown %d %d %x\n",
//				  code,len,flags);
			reader.getBytes(NULL,len);
			break;
		}
		remainingBytes-= len+8;
		if (print)
			fprintf(stderr," %d remaining %d %d %x\n",
			  remainingBytes,code,len,flags);
		if (remainingBytes == 0) {
			if (print)
				fprintf(stderr,"prev %d\n",prevCode);
			print= false;
			vsg::ref_ptr<vsg::Node> model;
			switch (prevCode) {
			  case 62: // levelcr
				//fprintf(stderr,"levelcr %d\n",visible);
				if (visible)
					model= loadStaticModel(&filename);
				break;
			  case 3: // static
			  case 56: // gantry
			  case 17: // signal
			  case 64: // speedpost
				model= loadStaticModel(&filename);
				if (prevCode==3 && model)
					model->setNodeMask(1);
				if (prevCode==17 && model)
					model= attachSwitchStand(tile,model,
					  x0+posX,z0+posZ,posY);
				break;
			  case 5: // trackobj
				{
					Track::SwVertex* swVertex= NULL;
					SwVertexMap::iterator i=
					  tile->swVertexMap.find(uid);
					if (i != tile->swVertexMap.end())
						swVertex= i->second;
					model= loadTrackModel(&filename,
					  swVertex);
				}
				break;
			  case 6: // trackobj
				model= makeDynTrack(trackSections,false);
				break;
			  case 63: // transfer
				model= makeTransfer(&filename,tile,
				  vsg::dvec3(posX,posY,posZ),
				  vsg::quat(qDirX,qDirY,qDirZ,qDirW),
				  width,height);
			  default:
				break;
			}
			if (model != NULL) {
				double a,x,y,z;
				vsg::quat(qDirX,qDirY,qDirZ,qDirW).getRotate(
				  a,x,y,z);
				vsg::quat q;
				q.makeRotate(-a,x,y,z);
				vsg::MatrixTransform* mt=
				  vsg::MatrixTransform::create();
				mt->setMatrix(vsg::dmat4(q)*
				 vsg::dmat4(1,0,0,0, 0,0,1,0, 0,1,0,0,
				  0,0,0,1) *
				 vsg::dmat4(1,0,0,0, 0,1,0,0, 0,0,1,0,
				  x0+posX,z0+posZ,posY,1) );
				mt->addChild(model);
				tile->models->addChild(mt);
			}
			remainingBytes= -1;
			visible= false;
			prevCode= 0;
		}
	}
#endif
	return 1;
}

//	loads a static model
//	just returns a pointer if already loaded
vsg::ref_ptr<vsg::Node> MSTSRoute::loadStaticModel(string* filename,
  MSTSSignal* signal)
{
	if (filename == NULL)
		return {};
	int idx= filename->rfind("\\");
	if (idx != string::npos) {
		filename->erase(0,idx+1);
//		fprintf(stderr,"remove path %s\n",filename->c_str());
	}
	ModelMap::iterator i= staticModelMap.find(*filename);
	if (i != staticModelMap.end() && i->second) {
		return i->second;
#if 0
		vsg::Node* model= i->second;
		if (signal) {
			model= (vsg::Node*)
			  model->clone(vsg::CopyOp::DEEP_COPY_NODES);
			SetSignalVisitor visitor(signal);
			model->accept(visitor);
		}
		return vsg::ref_ptr(model);
#endif
	}
	string path= rShapesDir+dirSep+*filename;
//	fprintf(stderr,"loading static model %s\n",path.c_str());
	MSTSShape shape;
	shape.vsgOptions= vsgOptions;
#if 0
	if (signal && strncasecmp(filename->c_str(),"hsuq",4)==0)
		shape.signalLightOffset= new vsg::dvec3(.23,-.23,-.1);
	else if (signal)
		shape.signalLightOffset= new vsg::dvec3(0,0,-.15);
	int tid,pi,pj;
	if (sscanf(filename->c_str(),"t-%x_%d_%d.s",&tid,&pi,&pj) == 3) {
		string tname= filename->substr(1,9);
		int pindex= pi*16+pj;
		TerrainTileMap::iterator i= terrainTileMap.find(tname);
		if (i != terrainTileMap.end()) {
			Tile* tile= i->second;
			Patch* patch= &tile->patches[pindex];
			osg::StateSet* ss=
			  tile->terrModel->getChild(pindex)->getStateSet();
			shape.mtStateSet= ss;
			shape.mtUVMult= tile->microTexUVMult;
			shape.patchU0= .5;//patch->u0 - patch->dudx*8;
			shape.patchV0= .5;//patch->v0 + patch->dvdz*8;
			shape.patchDuDx= patch->dudx*16/128;
			shape.patchDvDz= -patch->dvdz*16/128;
		}
	}
#endif
	try {
		shape.readFile(path.c_str(),rTexturesDir.c_str());
	} catch (const char* msg) {
		try {
			path= gShapesDir+dirSep+*filename;
			shape.readFile(path.c_str(),rTexturesDir.c_str(),
			  gTexturesDir.c_str());
		} catch (const char* msg) {
			fprintf(stderr,"loadStaticModel caught %s for %s\n",
			  msg,filename->c_str());
			return {};
		}
	}
	try {
		shape.fixTop();
		auto model= shape.createModel(0);
		if (!model) {
			fprintf(stderr,"failed to load static model %s\n",
			  path.c_str());
			return {};
		}
//		if (signal) {
//			SetSignalVisitor visitor(signal);
//			model->accept(visitor);
//		}
		staticModelMap[*filename]= model;
		return model;
	} catch (const char* msg) {
		fprintf(stderr,"loadStaticModel caught %s for %s\n",
		  msg,filename->c_str());
		return {};
	} catch (const std::exception& error) {
		fprintf(stderr,"loadStaticModel caught %s for %s\n",
		  error.what(),filename->c_str());
		return {};
	}
}

//	loads a track model and attaches it to the Track data so it
//	can be animated
vsg::ref_ptr<vsg::Node> MSTSRoute::loadTrackModel(string* filename,
  Track::SwVertex* swVertex)
{
	if (filename == NULL)
		return {};
	int idx= filename->rfind("\\");
	if (idx != string::npos) {
		filename->erase(0,idx+1);
//		fprintf(stderr,"remove tm path %s\n",filename->c_str());
	}
	ModelMap::iterator i= trackModelMap.find(*filename);
	if (i != trackModelMap.end() && i->second) {
		return i->second;
#if 0
		osg::Node* model= i->second;
		if (swVertex) {
			model= (osg::Node*)
			  model->clone(osg::CopyOp::DEEP_COPY_NODES);
			SetSwVertexVisitor visitor(swVertex);
			model->accept(visitor);
		}
		return model;
#endif
	}
	string path= idx != string::npos ?
	  rShapesDir+dirSep+*filename : gShapesDir+dirSep+*filename;
//	fprintf(stderr,"loading track model %s %p\n",path.c_str(),swVertex);
	MSTSShape shape;
	shape.vsgOptions= vsgOptions;
	try {
		shape.readFile(path.c_str(),rTexturesDir.c_str(),
		  gTexturesDir.c_str());
		shape.fixTop();
		auto model= shape.createModel(0,10,false,true);
#if 0
		if (wireHeight > 0) {
			string path= wireModelsDir+dirSep+*filename+".osg";
			osg::Node* wire= osgDB::readNodeFile(path);
			osg::Group* g= new osg::Group;
			g->addChild(wire);
			g->addChild(model);
			model= g;
		}
		if (swVertex) {
			SetSwVertexVisitor visitor(swVertex);
			model->accept(visitor);
		}
#endif
		trackModelMap[*filename]= model;
		return model;
	} catch (const char* msg) {
		fprintf(stderr,"loadTrackModel caught %s\n",msg);
		return {};
	} catch (const std::exception& error) {
		fprintf(stderr,"loadTrackModel caught %s\n",error.what());
		return {};
	}
}

vsg::ref_ptr<vsg::Node> MSTSRoute::loadHazardModel(string* filename)
{
	if (filename == NULL)
		return {};
//	fprintf(stderr,"hazard %s\n",filename->c_str());
	string path= routeDir+dirSep+*filename;
	MSTSFile file;
	file.readFile(path.c_str());
	MSTSFileNode* wf= file.find("Tr_Worldfile");
	if (wf == NULL) {
		fprintf(stderr,"%s not a MSTS hazard file?",
		  path.c_str());
		return {};
	}
	MSTSFileNode* fileNm= wf->children->find("FileName");
	if (fileNm == NULL)
		return {};
//	fprintf(stderr,"load hazard %s\n",fileNm->getChild(0)->value->c_str());
	auto model= loadStaticModel(fileNm->getChild(0)->value);
//	fprintf(stderr,"load hazard %p\n",model);
	return model;
}

//	makes 3D models for dynamic track
vsg::ref_ptr<vsg::Node> MSTSRoute::makeDynTrack(MSTSFileNode* dynTrack)
{
	MSTSFileNode* sections= dynTrack->children->find("TrackSections");
	if (sections == NULL)
		return NULL;
	bool bridge= false;
	MSTSFileNode* staticFlags= dynTrack->children->find("StaticFlags");
	if (staticFlags != NULL) {
		MSTSFileNode* p= staticFlags->getChild(0);
		if (p && p->value &&
		  strtol(p->value->c_str(),NULL,16)>0x10000000)
			bridge= true;
//		fprintf(stderr,"bridge %d\n",bridge);
	}
	TrackSections trackSections;
	for (MSTSFileNode* node=sections->children; node!=NULL;
	  node=node->next) {
		if (node->value == NULL)
			continue;
		MSTSFileNode* p= node->next->getChild(2);
		if (p==NULL || p->value==NULL)
			continue;
		int id= atoi(p->value->c_str());
		if (id < 0)
			continue;
		p= node->next->getChild(3);
		if (p==NULL || p->value==NULL)
			continue;
		float d= atof(p->value->c_str());
		if (d == 0)
			continue;
		p= node->next->getChild(4);
		if (p==NULL || p->value==NULL)
			continue;
		float r= atof(p->value->c_str());
		trackSections.push_back(TrackSection(d,r));
	}
	return makeDynTrack(trackSections,bridge);
}

vsg::ref_ptr<vsg::Node> MSTSRoute::makeDynTrack(TrackSections& trackSections, bool bridge)
{
	if (dynTrackBase == NULL && srDynTrack)
		makeSRDynTrackShapes();
	if (dynTrackBase == NULL && ustDynTrack)
		makeUSTDynTrackShapes();
	if (dynTrackBase==NULL && makeDynTrackShapes())
		;
	else if (dynTrackBase==NULL && makeUSTDynTrackShapes())
		;
	Track track;
	Track::Vertex* pv= track.addVertex(Track::VT_SIMPLE,0,0,0);
	float x= 0;
	float y= 0;
	float dx= 1;
	float dy= 0;
	for (int n=0; n<trackSections.size(); n++) {
		float d= trackSections[n].dist;
		float r= trackSections[n].radius;
		if (r == 0) {
			x+= dx*d;
			y+= dy*d;
		} else {
			float x0= dx;
			float y0= dy;
			float cs= .9998477;
			float sn= -.0174524;
			//float sn= d>0 ? .0174524 : -.0174524;
			float sr= d>0 ? r : -r;
			int m= (int)(abs(d/sn));
			for (int i=0; i<m; i++) {
				float x1= cs*x0 + sn*y0;
				float y1= cs*y0 - sn*x0;
				Track::Vertex* v=
				  track.addVertex(Track::VT_SIMPLE,
				  y+sr*(1-x1),x+r*y1,0);
				track.addEdge(Track::ET_STRAIGHT,pv,
				  n==0?0:1,v,0);
				x0= x1;
				y0= y1;
				pv= v;
			}
			if (d < 0) {
				x+= r*sin(-d);
				y-= r*(1-cos(d));
			} else {
				x+= r*sin(d);
				y+= r*(1-cos(d));
			}
			cs= cos(-d);
			sn= sin(-d);
			x0= cs*dx + sn*dy;
			y0= cs*dy - sn*dx;
			dx= x0;
			dy= y0;
		}
		Track::Vertex* v= track.addVertex(Track::VT_SIMPLE,y,x,0);
		track.addEdge(Track::ET_STRAIGHT,pv,n==0?0:1,v,0);
		pv= v;
	}
	auto group= vsg::Group::create();
	track.shape= dynTrackRails;
	group->addChild(track.makeGeometry(vsgOptions));
	if (!bridge) {
		track.shape= dynTrackBase;
		group->addChild(track.makeGeometry(vsgOptions));
		if (bermHeight > 0) {
			track.shape= dynTrackBerm;
			group->addChild(track.makeGeometry(vsgOptions));
		}
	} else if (bridgeBase) {
		track.shape= dynTrackBridge;
		group->addChild(track.makeGeometry(vsgOptions));
	}
	if (!bridge && dynTrackTies) {
		track.shape= dynTrackTies;
		group->addChild(track.makeGeometry(vsgOptions));
	}
	if (wireHeight > 0) {
		track.shape= dynTrackWire;
		group->addChild(track.makeGeometry(vsgOptions));
	}
	vsg::ref_ptr<vsg::MatrixTransform> mt= vsg::MatrixTransform::create();
	mt->matrix= vsg::dmat4(1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1);
	mt->addChild(group);
	return mt;
}

//	makes profile information for dynamic track
bool MSTSRoute::makeDynTrackShapes()
{
	string path= rTexturesDir+dirSep+"acleantrack1.ace";
	vsg::ref_ptr<vsg::Data> image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"acleantrack1.ace";
		image= readCacheACEFile(path.c_str());
	}
	if (!image)
		return false;
	dynTrackBase= new TrackShape;
	dynTrackBase->image= image;
//	OR uses -.153916 .862105
//	default MSTS track uses -.139 .862 and .2 per meter
	dynTrackBase->offsets.push_back(TrackShape::Offset(-2.5,-.18));
	dynTrackBase->offsets.push_back(TrackShape::Offset(-1.5,-.2));
	dynTrackBase->offsets.push_back(TrackShape::Offset(1.5,-.2));
	dynTrackBase->offsets.push_back(TrackShape::Offset(2.5,-.18));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(0,1,-.139,.062,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(1,2,.062,.662,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(2,3,.662,.862,5,4));
	dynTrackBase->matchOffsets();
	path= rTexturesDir+dirSep+"acleantrack2.ace";
	image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"acleantrack2.ace";
		image= readCacheACEFile(path.c_str());
	}
	dynTrackBridge= new TrackShape;
	dynTrackBridge->image= image;
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,.4));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,.4));
	if (bermHeight > 0) {
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(-1.1-.2*bermHeight,.4+bermHeight));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.1,.4));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(1.1,.4));
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(1.1+.2*bermHeight,.4+bermHeight));
	}
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(0,1,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(1,2,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(2,3,.662,.862,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(3,0,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(4,5,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(6,7,.662,.862,5,4));
	if (bermHeight > 0) {
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(8,9,-.139,.062,5,4));
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(10,11,.662,.862,5,4));
	}
	dynTrackBridge->matchOffsets();
	dynTrackRails= new TrackShape;
	dynTrackRails->image= image;
	dynTrackRails->offsets.push_back(TrackShape::Offset(.718,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.868,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.718,-.2));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.868,-.2));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.718,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.868,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.718,-.2));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.868,-.2));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(0,1,.12,.22,-4,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(2,0,.01,.11,-4,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(1,3,.11,.01,-4,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(5,4,.12,.22,-4,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(4,6,.01,.11,-4,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(7,5,.11,.01,-4,4));
	dynTrackRails->matchOffsets();
	if (bermHeight > 0) {
		string path= rTexturesDir+dirSep+"acleantrackbase.ace";
		image= readCacheACEFile(path.c_str());
		dynTrackBerm= new TrackShape;
		dynTrackBerm->image= image;
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  -2.75-1.5*bermHeight,bermHeight));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(-2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  2.75+1.5*bermHeight,bermHeight));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(0,1,0,.4,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(1,2,.4,.6,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(2,3,.6,1,50,4));
		dynTrackBerm->matchOffsets();
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(0,0,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(3,1,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(2,.6,.2));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(1,.4,.2));
	}
	if (wireHeight <= 0)
		return true;
	dynTrackWire= new TrackShape;
	float r= .015;
	dynTrackWire->offsets.push_back(TrackShape::Offset(0,-wireHeight));
	dynTrackWire->offsets.push_back(TrackShape::Offset(r,-wireHeight-r));
	dynTrackWire->offsets.push_back(TrackShape::Offset(0,-wireHeight-2*r));
	dynTrackWire->offsets.push_back(TrackShape::Offset(-r,-wireHeight-r));
	dynTrackWire->surfaces.push_back(TrackShape::Surface(0,1,0,0,0,0));
	dynTrackWire->surfaces.push_back(TrackShape::Surface(1,2,0,0,0,0));
	dynTrackWire->surfaces.push_back(TrackShape::Surface(2,3,0,0,0,0));
	dynTrackWire->surfaces.push_back(TrackShape::Surface(3,0,0,0,0,0));
	dynTrackWire->color= vsg::vec4(.2,.3,.2,1);
	return true;
}

//	makes profile information for scalerail dynamic track
bool MSTSRoute::makeSRDynTrackShapes()
{
	string path= rTexturesDir+dirSep+"sr_track_w1a.ace";
	vsg::ref_ptr<vsg::Data> image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"sr_track_w1a.ace";
		image= readCacheACEFile(path.c_str());
	}
	if (!image)
		return false;
	dynTrackBase= new TrackShape;
	dynTrackBase->image= image;
//	OR uses -.153916 .862105
//	default MSTS track uses -.139 .862 and .2 per meter
	dynTrackBase->offsets.push_back(TrackShape::Offset(-2.4384,.2));
	dynTrackBase->offsets.push_back(TrackShape::Offset(-1.4478,-.16));
	dynTrackBase->offsets.push_back(TrackShape::Offset(1.4478,-.16));
	dynTrackBase->offsets.push_back(TrackShape::Offset(2.4384,.2));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(0,1,0.,.203,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(1,2,.203,.797,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(2,3,.797,1.,5,4));
	dynTrackBase->matchOffsets();
	path= rTexturesDir+dirSep+"sr_track_w3a.ace";
	image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"sr_track_w3a.ace";
		image= readCacheACEFile(path.c_str());
	}
	dynTrackBridge= new TrackShape;
	dynTrackBridge->image= image;
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,.4));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,.4));
	if (bermHeight > 0) {
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(-1.1-.2*bermHeight,.4+bermHeight));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.1,.4));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(1.1,.4));
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(1.1+.2*bermHeight,.4+bermHeight));
	}
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(0,1,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(1,2,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(2,3,.662,.862,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(3,0,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(4,5,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(6,7,.662,.862,5,4));
	if (bermHeight > 0) {
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(8,9,-.139,.062,5,4));
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(10,11,.662,.862,5,4));
	}
	dynTrackBridge->matchOffsets();
	dynTrackRails= new TrackShape;
	dynTrackRails->image= image;
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7175,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7826,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7175,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7826,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7175,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7826,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7175,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7826,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7501,-.330));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7501,-.330));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.6850,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7387,-.2000));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7614,-.2000));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.8152,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.6850,-.1822));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7387,-.2000));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7614,-.2000));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.8152,-.1822));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(0,8,.43,.50,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(8,1,.50,.57,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(2,0,.97,.57,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(1,3,.43,.03,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(5,9,.57,.50,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(9,4,.50,.43,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(4,6,.57,.97,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(7,5,.03,.43,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(10,11,1.,.8,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(12,13,.2,.0,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(15,14,1.,.8,2,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(17,16,.2,.0,2,4));
	dynTrackRails->matchOffsets();
	if (bermHeight > 0) {
		string path= rTexturesDir+dirSep+"acleantrackbase.ace";
		image= readCacheACEFile(path.c_str());
		dynTrackBerm= new TrackShape;
		dynTrackBerm->image= image;
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  -2.75-1.5*bermHeight,bermHeight));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(-2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  2.75+1.5*bermHeight,bermHeight));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(0,1,0,.4,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(1,2,.4,.6,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(2,3,.6,1,50,4));
		dynTrackBerm->matchOffsets();
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(0,0,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(3,1,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(2,.6,.2));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(1,.4,.2));
	}
	if (wireHeight > 0) {
		dynTrackWire= new TrackShape;
		float r= .015;
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(0,-wireHeight));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(r,-wireHeight-r));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(0,-wireHeight-2*r));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(-r,-wireHeight-r));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(0,1,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(1,2,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(2,3,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(3,0,0,0,0,0));
		dynTrackWire->color= vsg::vec4(.2,.3,.2,1);
	}
	path= rTexturesDir+dirSep+"sr_track_w2a.ace";
	image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"sr_track_w2a.ace";
		image= readCacheACEFile(path.c_str());
	}
	dynTrackTies= new TrackShape;
	dynTrackTies->image= image;
	dynTrackTies->offsets.push_back(TrackShape::Offset(-1.2954,-.16));
	dynTrackTies->offsets.push_back(TrackShape::Offset(-1.2954,-.1822));
	dynTrackTies->offsets.push_back(TrackShape::Offset(1.2954,-.1822));
	dynTrackTies->offsets.push_back(TrackShape::Offset(2.2954,-.16));
	dynTrackTies->surfaces.push_back(
	  TrackShape::Surface(1,2,.235,.766,5,4));
	dynTrackTies->matchOffsets();
	return true;
}

//	makes profile information for US tracks dynamic track
bool MSTSRoute::makeUSTDynTrackShapes()
{
	string path= rTexturesDir+dirSep+"US_Track3.ace";
	vsg::ref_ptr<vsg::Data> image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"US_Track3.ace";
		image= readCacheACEFile(path.c_str());
	}
	if (!image) {
		ustDynTrack= false;
		return false;
	}
	dynTrackBase= new TrackShape;
	dynTrackBase->image= image;
	dynTrackBase->offsets.push_back(TrackShape::Offset(-2.6,.2));
	dynTrackBase->offsets.push_back(TrackShape::Offset(-1.7,-.136));
	dynTrackBase->offsets.push_back(TrackShape::Offset(1.7,-.136));
	dynTrackBase->offsets.push_back(TrackShape::Offset(2.6,.2));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(0,1,.862,.7158,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(1,2,.7158,.0342,5,4));
	dynTrackBase->surfaces.push_back(
	  TrackShape::Surface(2,3,.0342,-.1389,5,4));
	dynTrackBase->matchOffsets();
	path= rTexturesDir+dirSep+"US_Rails3.ace";
	image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"US_Rails3.ace";
		image= readCacheACEFile(path.c_str());
	}
	dynTrackBridge= new TrackShape;
	dynTrackBridge->image= image;
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,-.2));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1.5,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,.4));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(-1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,0));
	dynTrackBridge->offsets.push_back(TrackShape::Offset(1,.4));
	if (bermHeight > 0) {
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(-1.1-.2*bermHeight,.4+bermHeight));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(-1.1,.4));
		dynTrackBridge->offsets.push_back(TrackShape::Offset(1.1,.4));
		dynTrackBridge->offsets.push_back(
		  TrackShape::Offset(1.1+.2*bermHeight,.4+bermHeight));
	}
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(0,1,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(1,2,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(2,3,.662,.862,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(3,0,.062,.662,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(4,5,-.139,.062,5,4));
	dynTrackBridge->surfaces.push_back(
	  TrackShape::Surface(6,7,.662,.862,5,4));
	if (bermHeight > 0) {
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(8,9,-.139,.062,5,4));
		dynTrackBridge->surfaces.push_back(
		  TrackShape::Surface(10,11,.662,.862,5,4));
	}
	dynTrackBridge->matchOffsets();
	dynTrackRails= new TrackShape;
	dynTrackRails->image= image;
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7175,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7895,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7175,-.185));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7895,-.185));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7175,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7895,-.325));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7175,-.185));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7895,-.185));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7501,-.330));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7501,-.330));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.6785,-.170));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7535,-.187));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.7535,-.187));
	dynTrackRails->offsets.push_back(TrackShape::Offset(.8285,-.170));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.6785,-.170));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7535,-.187));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.7535,-.187));
	dynTrackRails->offsets.push_back(TrackShape::Offset(-.8285,-.170));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(0,1,-.9336,-.9883,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(2,0,-.8359,-.9336,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(1,3,-.9336,-.8359,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(5,4,-.9883,-.9336,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(4,6,-.9336,-.8359,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(7,5,-.8359,-.9336,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(10,11,-.793,-.8414,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(12,13,-.8414,-.793,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(15,14,-.793,-.8414,-.5,4));
	dynTrackRails->surfaces.push_back(
	  TrackShape::Surface(17,16,-.8414,-.793,-.5,4));
	dynTrackRails->matchOffsets();
	if (bermHeight > 0) {
		string path= rTexturesDir+dirSep+"acleantrackbase.ace";
		image= readCacheACEFile(path.c_str());
		dynTrackBerm= new TrackShape;
		dynTrackBerm->image= image;
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  -2.75-1.5*bermHeight,bermHeight));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(-2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(2.75,.1));
		dynTrackBerm->offsets.push_back(TrackShape::Offset(
		  2.75+1.5*bermHeight,bermHeight));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(0,1,0,.4,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(1,2,.4,.6,50,4));
		dynTrackBerm->surfaces.push_back(
		  TrackShape::Surface(2,3,.6,1,50,4));
		dynTrackBerm->matchOffsets();
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(0,0,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(3,1,.1));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(2,.6,.2));
		dynTrackBerm->endVerts.push_back(TrackShape::EndVert(1,.4,.2));
	}
	if (wireHeight > 0) {
		dynTrackWire= new TrackShape;
		float r= .015;
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(0,-wireHeight));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(r,-wireHeight-r));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(0,-wireHeight-2*r));
		dynTrackWire->offsets.push_back(
		  TrackShape::Offset(-r,-wireHeight-r));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(0,1,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(1,2,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(2,3,0,0,0,0));
		dynTrackWire->surfaces.push_back(
		  TrackShape::Surface(3,0,0,0,0,0));
		dynTrackWire->color= vsg::vec4(.2,.3,.2,1);
	}
	path= rTexturesDir+dirSep+"US_Track3s.ace";
	image= readCacheACEFile(path.c_str());
	if (!image) {
		path= gTexturesDir+dirSep+"US_Track3.ace";
		image= readCacheACEFile(path.c_str());
	}
	dynTrackTies= new TrackShape;
	dynTrackTies->image= image;
	dynTrackTies->offsets.push_back(TrackShape::Offset(-1.35,-.152));
	dynTrackTies->offsets.push_back(TrackShape::Offset(0,-.154));
	dynTrackTies->offsets.push_back(TrackShape::Offset(1.35,-.152));
	dynTrackTies->surfaces.push_back(
	  TrackShape::Surface(0,1,.6457,.375,5,4));
	dynTrackTies->surfaces.push_back(
	  TrackShape::Surface(1,2,.375,.1043,5,4));
	dynTrackTies->matchOffsets();
	return true;
}

struct Random {
	int value;
	Random(int seed=12345) { value= seed; };
	double next() {
		value= (value*2661+36979) % 175000;
		return value/(double)(175000-1);
	};
};

int addCrossTree(int vIndex, float w, float h, float scale,
  float x, float y, float z, vsg::ref_ptr<vsg::vec3Array>& verts,
  vsg::ref_ptr<vsg::vec3Array>& normals, vsg::ref_ptr<vsg::vec2Array>& texCoords,
  vsg::ref_ptr<vsg::vec4Array>& colors, vsg::ref_ptr<vsg::ushortArray>& indices)
{
	verts->at(vIndex)= vsg::vec3(-w/2*scale+x,y,z);
	verts->at(vIndex+1)= vsg::vec3(w/2*scale+x,y,z);
	verts->at(vIndex+2)= vsg::vec3(w/2*scale+x,h*scale+y,z);
	verts->at(vIndex+3)= vsg::vec3(-w/2*scale+x,h*scale+y,z);
	normals->at(vIndex)= vsg::vec3(0,0,1);
	normals->at(vIndex+1)= vsg::vec3(0,0,1);
	normals->at(vIndex+2)= vsg::vec3(0,0,1);
	normals->at(vIndex+3)= vsg::vec3(0,0,1);
	texCoords->at(vIndex)= vsg::vec2(0,1);
	texCoords->at(vIndex+1)= vsg::vec2(1,1);
	texCoords->at(vIndex+2)= vsg::vec2(1,0);
	texCoords->at(vIndex+3)= vsg::vec2(0,0);
	colors->at(vIndex)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+1)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+2)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+3)= vsg::vec4(1,1,1,1);
	int ii= 3*vIndex;
	indices->set(ii++,0+vIndex);
	indices->set(ii++,1+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,3+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,1+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,3+vIndex);
	indices->set(ii++,2+vIndex);
	vIndex+= 4;
	verts->at(vIndex)= vsg::vec3(x,y,-w/2*scale+z);
	verts->at(vIndex+1)= vsg::vec3(x,y,w/2*scale+z);
	verts->at(vIndex+2)= vsg::vec3(x,h*scale+y,w/2*scale+z);
	verts->at(vIndex+3)= vsg::vec3(x,h*scale+y,-w/2*scale+z);
	normals->at(vIndex)= vsg::vec3(1,0,0);
	normals->at(vIndex+1)= vsg::vec3(1,0,0);
	normals->at(vIndex+2)= vsg::vec3(1,0,0);
	normals->at(vIndex+3)= vsg::vec3(1,0,0);
	texCoords->at(vIndex)= vsg::vec2(0,1);
	texCoords->at(vIndex+1)= vsg::vec2(1,1);
	texCoords->at(vIndex+2)= vsg::vec2(1,0);
	texCoords->at(vIndex+3)= vsg::vec2(0,0);
	colors->at(vIndex)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+1)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+2)= vsg::vec4(1,1,1,1);
	colors->at(vIndex+3)= vsg::vec4(1,1,1,1);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,1+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,3+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,2+vIndex);
	indices->set(ii++,1+vIndex);
	indices->set(ii++,0+vIndex);
	indices->set(ii++,3+vIndex);
	indices->set(ii++,2+vIndex);
	vIndex+= 4;
	return vIndex;
}

vsg::ref_ptr<vsg::Node> MSTSRoute::makeForest(MSTSFileNode* forest,
  Tile* tile, MSTSFileNode* pos, MSTSFileNode* qdir)
{
	MSTSFileNode* treeTexture= forest->children->find("TreeTexture");
	MSTSFileNode* scaleRange= forest->children->find("ScaleRange");
	MSTSFileNode* area= forest->children->find("Area");
	MSTSFileNode* size= forest->children->find("TreeSize");
	MSTSFileNode* population= forest->children->find("Population");
	if (forest==NULL || pos==NULL || qdir==NULL || treeTexture==NULL ||
	  scaleRange==NULL || area==NULL || size==NULL || population==NULL) {
//		fprintf(stderr,"forest %p %p %p %p %p %p %p %p\n",forest,
//		  pos,qdir,treeTexture,scaleRange,area,size,population);
		return {};
	}
	float x0= 2048*(tile->x-centerTX);
	float z0= 2048*(tile->z-centerTZ);
	vsg::quat q(-atof(qdir->getChild(0)->value->c_str()),
	  -atof(qdir->getChild(1)->value->c_str()),
	  -atof(qdir->getChild(2)->value->c_str()),
	  atof(qdir->getChild(3)->value->c_str()));
	vsg::mat4 rot= vsg::rotate(q);
	vsg::vec3 center= vsg::vec3(atof(pos->getChild(0)->value->c_str()),
	  atof(pos->getChild(1)->value->c_str()),
	  atof(pos->getChild(2)->value->c_str()));
	Tile* t12= findTile(tile->x,tile->z-1);
	Tile* t21= findTile(tile->x+1,tile->z);
	Tile* t22= findTile(tile->x+1,tile->z-1);
	float a0= center[1];//getAltitude(center[0],center[2],tile,t12,t21,t22);
	float scale= atof(scaleRange->getChild(0)->value->c_str());
	float range= atof(scaleRange->getChild(1)->value->c_str());
	float areaW= atof(area->getChild(0)->value->c_str());
	float areaH= atof(area->getChild(1)->value->c_str());
	float w= atof(size->getChild(0)->value->c_str());
	float h= atof(size->getChild(1)->value->c_str());
	int pop= atoi(population->getChild(0)->value->c_str());
//	fprintf(stderr,"forest %s %.2f %.2f %.2f %.2f %.2f %.2f %d\n",
//	  treeTexture->getChild(0)->value->c_str(),
//	  scale,range,areaW,areaH,w,h,pop);
	string path= rTexturesDir+dirSep+treeTexture->getChild(0)->value->c_str();
	vsg::ref_ptr<vsg::Data> image= readMSTSACE(path.c_str());
	if (!image)
		{};
	if (pop > 65536/8)
		pop= 65536/8;
	int numVert= 8*pop;
	int numInd= 3*numVert;
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(numVert));
	vsg::ref_ptr<vsg::vec2Array> texCoords(new vsg::vec2Array(numVert));
	vsg::ref_ptr<vsg::vec3Array> normals(new vsg::vec3Array(numVert));
	vsg::ref_ptr<vsg::vec4Array> colors(new vsg::vec4Array(numVert));
	auto indices= vsg::ushortArray::create(3*numVert);
	int vIndex= 0;
	Random random;
	int nh= (int)ceil(sqrt(pop*areaH/areaW));
	int nv= (int)ceil(pop/(double)nh);
	for (int i=0; i<nh; i++) {
		if (i==nh-1 && nv>pop)
			nv= pop;
//		fprintf(stderr,"forest %d %d %d %d\n",i,nh,nv,pop);
		for (int j=0; j<nv; j++) {
			float s= (i+.5+.9*(random.next()-.5))/(float)nh;
			float t= (j+.5+.9*(random.next()-.5))/(float)nv;
			float size= scale + (range-scale)*random.next();
			float x= (s-.5)*(areaW>size?areaW-size:0);
			float z= (t-.5)*(areaH>size?areaH-size:0);
			vsg::vec3 p= rot*vsg::vec3(x,0,z) + center;
			float a= getAltitude(p.x,p.z,tile,t12,t21,t22);
			vIndex= addCrossTree(vIndex,w,h,size,x,a-a0,z,
			  verts,normals,texCoords,colors,indices);
			if (vIndex >= numVert)
				break;
//			fprintf(stderr,"forest %d %d %f %f %d\n",i,j,s,t,pop);
			if (pop-- == 0)
				break;
		}
		if (vIndex >= numVert)
			break;
	}
	auto attributeArrays= vsg::DataList{verts,normals,texCoords,colors};
	auto vid= vsg::VertexIndexDraw::create();
	vid->assignArrays(attributeArrays);
	vid->assignIndices(indices);
	vid->indexCount= indices->size();
	vid->instanceCount= 1;
	vid->firstIndex= 0;
	vid->vertexOffset= 0;
	vid->firstInstance= 0;
	auto stateGroup= vsg::StateGroup::create();
	stateGroup->addChild(vid);
	auto shaderSet= vsg::createFlatShadedShaderSet(vsgOptions);;
	auto matValue= vsg::PhongMaterialValue::create();
	matValue->value().ambient= vsg::vec4(0,0,0,0);
	matValue->value().diffuse= vsg::vec4(.5,.5,.5,1);
	matValue->value().specular= vsg::vec4(0,0,0,1);
	matValue->value().shininess= 0;
	auto sampler= vsg::Sampler::create();
	sampler->addressModeU= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler->addressModeV= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	vsgOptions->sharedObjects->share(sampler);
	auto gpConfig= vsg::GraphicsPipelineConfigurator::create(shaderSet);
	matValue->value().alphaMask= 1;
	matValue->value().alphaMaskCutoff= .6;
	gpConfig->shaderHints->defines.insert("VSG_ALPHA_TEST");
	gpConfig->assignTexture("diffuseMap",image,sampler);
	gpConfig->assignDescriptor("material",matValue);
	gpConfig->enableArray("vsg_Vertex",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_Normal",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_TexCoord0",VK_VERTEX_INPUT_RATE_VERTEX,8);
	gpConfig->enableArray("vsg_Color",VK_VERTEX_INPUT_RATE_VERTEX,16);
	if (vsgOptions->sharedObjects)
		vsgOptions->sharedObjects->share(gpConfig,
		  [](auto gpc) { gpc->init(); });
	else
		gpConfig->init();
	vsg::StateCommands commands;
	gpConfig->copyTo(commands,vsgOptions->sharedObjects);
	stateGroup->stateCommands.swap(commands);
	stateGroup->prototypeArrayState= gpConfig->getSuitableArrayState();
	return stateGroup;
}

MstsWorldReader::MstsWorldReader()
{
}

bool MstsWorldReader::getFeatures(Features& features) const
{
	features.extensionFeatureMap[".world"]=
	  static_cast<vsg::ReaderWriter::FeatureMask>(READ_FILENAME);
	return true;
}

vsg::ref_ptr<vsg::Object> MstsWorldReader::read(
  const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
	if (!mstsRoute)
		return {};
	const auto ext= vsg::lowerCaseFileExtension(filename);
	if (ext != ".world")
		return {};
	auto i= mstsRoute->terrainTileMap.find(
	  filename.string().substr(0,9).c_str());
	if (i == mstsRoute->terrainTileMap.end())
		return {};
	auto tile= i->second;
	if (!tile->models)
		mstsRoute->loadModels(tile);
	return tile->models;
}
