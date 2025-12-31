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
	fprintf(stderr,"loadModels from %s %f %f\n",path.c_str(),x0,z0);
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
#if 0
				model= loadTrackModel(file->getChild(0)->value,
				  swVertex);
			} else if (*(node->value)=="Dyntrack") {
				model= makeDynTrack(next);
			} else if (*(node->value)=="Transfer") {
				model= makeTransfer(next,
				  file->getChild(0)->value,tile,pos,qdir);
			} else if (*(node->value)=="Forest") {
				model= makeForest(next,tile,pos,qdir);
			} else if (*(node->value)=="Hazard" && file!=NULL) {
				model=
				  loadHazardModel(file->getChild(0)->value);
#if 0
			  	fprintf(stderr,"hazard pos %s %s %s %f %f\n",
				  pos->getChild(0)->value->c_str(),
			  	  pos->getChild(2)->value->c_str(),
			  	  pos->getChild(1)->value->c_str(),x0,z0);
			  	fprintf(stderr,"hazard qdir %s %s %s %s\n",
				  qdir->getChild(0)->value->c_str(),
				  qdir->getChild(1)->value->c_str(),
				  qdir->getChild(2)->value->c_str(),
				  qdir->getChild(3)->value->c_str());
#endif
			} else if (*(node->value)=="Signal") {
#if 0
				fprintf(stderr,"signal %s\n",
				  file->get(0)->c_str());
#endif
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
		vsg::Node* model= i->second;
#if 0
		if (signal) {
			model= (vsg::Node*)
			  model->clone(vsg::CopyOp::DEEP_COPY_NODES);
			SetSignalVisitor visitor(signal);
			model->accept(visitor);
		}
#endif
		return vsg::ref_ptr(model);
	}
	string path= rShapesDir+dirSep+*filename;
	fprintf(stderr,"loading static model %s\n",path.c_str());
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
		vsg::ref_ptr<vsg::Node> model= shape.createModel(0);
		if (model == NULL) {
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
	fprintf(stderr,"read world %s\n",filename.string().c_str());
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
