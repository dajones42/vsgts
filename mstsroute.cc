//	code for reading various MSTS route data
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
#include <plib/ul.h>
#include <vsg/all.h>

#include "mstsroute.h"
#include "mstsfile.h"
#include "mstsbfile.h"
#include "trackdb.h"
#include "track.h"
//#include "activity.h"
//#include "service.h"
//#include "consist.h"
//#include "trackpath.h"
//#include "ghproj.h"
//#include "trigrid.h"
//#include "mstswag.h"
//#include "train.h"

using namespace std;

MSTSRoute* mstsRoute= NULL;

string fixFilenameCase(string path)
{
	return fixFilenameCase(path.c_str());
}

MSTSRoute* MSTSRoute::createRoute(string tdbPath)
{
	auto i= tdbPath.rfind('/');
	if (i == string::npos)
		return nullptr;
	string path1= tdbPath.substr(0,i);
	i= path1.rfind('/');
	if (i == string::npos)
		return nullptr;
	string routeID= path1.substr(i+1);
	path1= path1.substr(0,i);
	i= path1.rfind('/');
	if (i == string::npos)
		return nullptr;
	string mstsDir= path1.substr(0,i);
	mstsRoute= new MSTSRoute(mstsDir.c_str(),routeID.c_str());
	return mstsRoute;
}

MSTSRoute::MSTSRoute(const char* mDir, const char* rID)
{
	mstsDir= string(mDir);
	dirSep= "/";
	routeID= string(rID);
	fileName= routeID;
	string routesDir= fixFilenameCase(mstsDir+dirSep+"ROUTES");
	string globalDir= fixFilenameCase(mstsDir+dirSep+"GLOBAL");
	routeDir= fixFilenameCase(routesDir+dirSep+routeID);
	gShapesDir= fixFilenameCase(globalDir+dirSep+"SHAPES");
	gTexturesDir= fixFilenameCase(globalDir+dirSep+"TEXTURES")+dirSep;
	tilesDir= fixFilenameCase(routeDir+dirSep+"TILES");
	terrtexDir= fixFilenameCase(routeDir+dirSep+"TERRTEX");
	worldDir= fixFilenameCase(routeDir+dirSep+"WORLD");
	rShapesDir= fixFilenameCase(routeDir+dirSep+"SHAPES");
	rTexturesDir= fixFilenameCase(routeDir+dirSep+"TEXTURES")+dirSep;
	dynTrackBase= NULL;
	dynTrackRails= NULL;
	dynTrackWire= NULL;
	dynTrackTies= NULL;
	centerLat= centerLong= 0;
	drawWater= 1;
	waterLevelDelta= 0;
	bermHeight= 0;
	wireHeight= 0;
	bridgeBase= false;
	srDynTrack= false;
	ustDynTrack= true;
	ignoreHiddenTerrain= false;
	signalSwitchStands= false;
	createSignals= false;
	wireTerrain= false;
}

MSTSRoute::~MSTSRoute()
{
}

//	find the center of the route
//	used for coordinate conversion
void MSTSRoute::findCenter(TrackDB* trackDB)
{
	int minTX= 0x7fffffff;
	int maxTX= -0x7fffffff;
	int minTZ= 0x7fffffff;
	int maxTZ= -0x7fffffff;
	for (int i=0; i<trackDB->nNodes; i++) {
		TrackNode* node= &trackDB->nodes[i];
		if (node->nSections > 0)
			continue;
		if (minTX > node->tx)
			minTX= node->tx;
		if (maxTX < node->tx)
			maxTX= node->tx;
		if (minTZ > node->tz)
			minTZ= node->tz;
		if (maxTZ < node->tz)
			maxTZ= node->tz;
	}
//	fprintf(stderr,"%d %d %d %d\n",minTX,maxTX,minTZ,maxTZ);
	if (centerLat!=0 || centerLong!=0) {
		cosCenterLat= cos(centerLat/(180*3.14159));
		fprintf(stderr,"%f %f %lf\n",
		  centerLat,centerLong,cosCenterLat);
	} else {
		centerTX= (minTX+maxTX)/2;
		centerTZ= (minTZ+maxTZ)/2;
	}
//	fprintf(stderr,"center %d %d\n",centerTX,centerTZ);
//	double lat,lng,xx,yy;
//	xy2ll(0,0,&lat,&lng);
//	ll2xy(lat,lng,&xx,&yy);
//	fprintf(stderr,"centerll %f %f %f %f\n",lat,lng,xx,yy);
}

static int findPin(TrackNode* n1, TrackNode* n2, int end)
{
	for (int i=0; i<n1->nPins; i++)
		if (n1->pin[i]==n2 && n1->pinEnd[i]==end)
			return i;
	fprintf(stderr,"cannot find pin %d %d %d\n",n1->id,n2->id,end);
	return -1;
}

#if 0
void MSTSRoute::ll2xy(double lat, double lng, double* xp, double* yp)
{
	if (centerLat!=0 || centerLong!=0) {
		*xp= (lng-centerLong)*cosCenterLat*60*1852.216;
		*yp= (lat-centerLat)*60*1852.216;
	} else {
		double x,y;
		ghProj.ll2xy(lat,lng,&x,&y);
		*xp= x - centerTX*2048 + 20015000 - 16385*2048 + 1024;
		*yp= y - centerTZ*2048 - 8673000 + 16385*2048 - 3072;
	}
}

void MSTSRoute::xy2ll(double x, double y, double* lat, double* lng)
{
	if (centerLat!=0 || centerLong!=0) {
		*lng= x/(cosCenterLat*60*1852.216) + centerLong;
		*lat= y/(60*1852.216) + centerLat;
	} else {
		ghProj.xy2ll(x + centerTX*2048 - 20015000 + 16385*2048 - 1024,
		  y + centerTZ*2048 + 8673000 - 16385*2048 + 3072, lat,lng);
	}
}
#endif

//	Makes Track class data from tsection and tdb data
void MSTSRoute::makeTrack()
{
	TSection tSection;
	string globalDir= fixFilenameCase(mstsDir+dirSep+"GLOBAL");
	string path= fixFilenameCase(globalDir+dirSep+"tsection.dat");
	tSection.readGlobalFile(path.c_str());
	path= fixFilenameCase(routeDir+dirSep+"tsection.dat");
	tSection.readRouteFile(path.c_str());
	path= fixFilenameCase(routeDir+dirSep+fileName+".tdb");
	if (path.size() == 0) {
		ulDir* dir= ulOpenDir(routeDir.c_str());
		if (dir == NULL) {
			fprintf(stderr,"cannot read route directory %s\n",
			  routeDir.c_str());
		}
		for (ulDirEnt* ent=ulReadDir(dir); ent!=NULL;
		  ent=ulReadDir(dir)) {
			int n= strlen(ent->d_name);
			if (strcasecmp(ent->d_name+n-4,".trk")==0)
				path= routeDir+dirSep+ent->d_name;
		}
		if (path.size() == 0)
			fprintf(stderr,"cannot find tdb file\n");
		fprintf(stderr,"tdbfile %s\n",path.c_str());
	}
	TrackDB trackDB;
	trackDB.readFile(path.c_str(),&tSection);
//	fprintf(stderr,"nNodes %d nTrItems %d\n",
//	  trackDB.nNodes,trackDB.nTrItems);
	findCenter(&trackDB);
	Track* track= new Track;
	trackMap[routeID]= track;
	track->updateSignals= createSignals;
	typedef map<int,Track::Vertex*> VMap;
	VMap vMap;
	for (int i=0; i<trackDB.nNodes; i++) {
		TrackNode* node= &trackDB.nodes[i];
		if (node->nSections > 0)
			continue;
		Track::Vertex* v= track->addVertex(
		  node->shape==0?Track::VT_SIMPLE:Track::VT_SWITCH,
		  convX(node->tx,node->x),
		  convZ(node->tz,node->z),node->y+.275);
		if (v->type == Track::VT_SWITCH)
			((Track::SwVertex*)v)->id= i;
		Tile* tile= findTile(node->tx,node->tz);
		if (node->shape != 0) {
			((Track::SwVertex*)v)->mainEdge=
			  tSection.findMainRoute(node->shape);
			if (tile != NULL)
				tile->swVertexMap[node->id]=
				  (Track::SwVertex*)v;
			//fprintf(stderr,"swid %p %d %p\n",tile,node->id,
			//  (Track::SwVertex*)v);
		}
		vMap[i]= v;
		node->tx= i;
	}
	for (int i=0; i<trackDB.nNodes; i++) {
		TrackNode* node= &trackDB.nodes[i];
		if (node->nSections == 0)
			continue;
		VMap::iterator i1= vMap.find(node->pin[0]->tx);
		if (i1 == vMap.end()) {
			fprintf(stderr,"? cannot find start node %d\n",
			  node->pin[0]->tx);
			continue;
		}
		Track::Vertex* v1= i1->second;
		int n1= findPin(node->pin[0],node,1);
		Track::SSEdge* sse= new Track::SSEdge;
		sse->track= track;
		sse->block= i;
		sse->length= 0;
		sse->v1= v1;
		if (v1->type == Track::VT_SWITCH)
			((Track::SwVertex*)v1)->ssEdges[n1]= sse;
		track->ssEdgeMap[sse->block]= sse;
		double len= node->sections[0].length;
		TrSection* s1= &node->sections[0];
		Tile* tile= findTile(s1->tx,s1->tz);
		for (int j=1; j<node->nSections; j++) {
			TrSection* s2= &node->sections[j];
			if (tile==NULL || tile->x!=s2->x || tile->z!=s2->z)
				tile= findTile(s2->tx,s2->tz);
			Track::Vertex* v2= NULL;
			int n2= 0;
			if (j == node->nSections-1) {
				VMap::iterator i2= vMap.find(node->pin[1]->tx);
				if (i2 == vMap.end())
					fprintf(stderr,
					  "? cannot find end node %d\n",
					  node->pin[1]->tx);
				else
					v2= i2->second;
				n2= findPin(node->pin[1],node,0);
				sse->v2= v2;
				if (v2->type == Track::VT_SWITCH)
					((Track::SwVertex*)v2)->ssEdges[n2]=
					  sse;
			}
			if (v2 == NULL)
				v2= track->addVertex(Track::VT_SIMPLE,
				  convX(s2->tx,s2->x),convZ(s2->tz,s2->z),
				  s2->y+.275);
			if (s1->radius > 0) {
				Track::SplineEdge* e= (Track::SplineEdge*)
				  track->addEdge(Track::ET_SPLINE,v1,n1,v2,n2);
				e->length= s1->length;
				e->setCircle(s1->radius,s1->angle*3.14159/180);
				e->ssEdge= sse;
				e->ssOffset= sse->length;
				sse->length+= e->length;
			} else {
				Track::Edge* e=
				 track->addEdge(Track::ET_STRAIGHT,v1,n1,v2,n2);
				e->ssEdge= sse;
				e->ssOffset= sse->length;
				sse->length+= e->length;
			}
			v1= v2;
			n1= n2+1;
			len= s1->length;
			s1= s2;
		}
	}
	for (int i=0; i<trackDB.nTrItems; i++) {
		TrItem* item= &trackDB.trItems[i];
		if (item->name==NULL)
			continue;
//		if (item->otherID!=0 && item->otherID<item->id)
//			continue;
		double x= convX(item->tx,item->x);
		double z= convZ(item->tz,item->z);
		double y= item->y;
		if (item->type==0 || item->type==1) {
			track->saveLocation(x,z,y,*item->name,item->type);
		} else if (createSignals && item->type==2) {
#if 0
			Track::Location loc;
			track->findLocation(x,z,y,&loc);
			if (loc.edge == NULL)
				continue;
			if (item->dir) {
				loc.rev= 1;
				if (loc.edge->v1->type != Track::VT_SWITCH) {
					loc.move(loc.offset+.01,0,0);
				}
			} else if (loc.edge->v2->type != Track::VT_SWITCH) {
				loc.move(loc.edge->length-loc.offset+.01,0,0);
			}
			Signal* sig= new Signal(Signal::CLEAR);
			//sig->setState(Signal::CLEAR);
			sig->addTrack(&loc);
			loc.edge->signals.push_back(sig);
			char buf[10];
			sprintf(buf,"%d",item->id);
			signalMap[buf]= sig;
#endif
		}
//		else
//			fprintf(stderr,"%s %d %d %d %f %f %f %d %d %f\n",
//			  item->name->c_str(),
//			  item->id,item->otherID,item->type,x,z,y,item->flags,
//			  item->dir,item->offset);
	}
	track->makeSSEdges();
	track->calcGrades();
}

static char hexdigit [] = {
	'0','1','4','5',
	'3','2','7','6',
	'c','d','8','9',
	'f','e','b','a'
};

//	returns file name for tile x,z
const char* MSTSRoute::tFile(int x, int z)
{
	static char filename[10];
	x+= 16384;
	z= 16384-z-1;
	x<<= 1;
	z<<= 1;
	filename[0]= '-';
	filename[9]= '\0';
	for (int i=1; i<9; i++) {
		int s= 16-2*i;
		int m= 3<<s;
		int j= ((x&m)>>s) + 4*((z&m)>>s);
//		printf("%d %d %d %d %d %d\n",i,s,m,j,
//		  (x&m)>>s,(z&m)>>s);
		filename[i]= hexdigit[j];
	}
	return filename;
}

//	converts tile filename to x and z
void MSTSRoute::tFileToXZ(char* filename, int *xp, int * zp)
{
	int x= 0;
	int z= 0;
	for (int i=1; i<9; i++) {
		int j= 0;
		for (; j<16; j++)
			if (filename[i] == hexdigit[j])
				break;
		int s= 16-2*i;
		int m= 3<<s;
		x|= (j&3)<<s;
		z|= ((j&017)>>2)<<s;
	}
	x>>= 1;
	z>>= 1;
	*xp= x-16384;
	*zp= 16384-z-1;
}

//	Reads all files in tiles directory to get tiles in route
void MSTSRoute::readTiles()
{
	
	ulDir* dir= ulOpenDir(tilesDir.c_str());
	if (dir == NULL) {
		fprintf(stderr,"cannot read tiles directory %s\n",
		  tilesDir.c_str());
		return;
	}
	for (ulDirEnt* ent=ulReadDir(dir); ent!=NULL; ent=ulReadDir(dir)) {
		int n= strlen(ent->d_name);
		if (ent->d_name[n-1] != 't')
			continue;
		int x,z;
		tFileToXZ(ent->d_name,&x,&z);
//		fprintf(stderr,"%s %d %d\n",ent->d_name,x,z);
		Tile* t= new Tile(x,z);
		t->tFilename= tFile(x,z);
		string path= tilesDir+dirSep+ent->d_name;
		tileMap[tileID(x,z)]= t;
		terrainTileMap[t->tFilename]= t;
		readTFile(path.c_str(),t);
//		fprintf(stderr,"%s tf=%f sc=%f\n",
//		  ent->d_name,t->floor,t->scale);
//		fprintf(stderr," %d %d %s\n",x,z,tFile(x,z));
//		if (t->swWaterLevel != 0)
//			fprintf(stderr,"%d %d sw=%f se=%f ne=%f nw=%f\n",
//			  x,z,t->swWaterLevel,t->seWaterLevel,
//			  t->neWaterLevel,t->nwWaterLevel);
	}
	ulCloseDir(dir);
}

//	reads a single tile file and saves some of the data
int MSTSRoute::readTFile(const char* filename, Tile* tile)
{
	MSTSBFile reader;
	if (reader.open(filename))
		return 0;
//	fprintf(stderr,"%s is binary %d\n",filename,reader.compressed);
	Byte buf[16];
	reader.getBytes(buf,16);
	Patch* patch= tile->patches;
	for (;;) {
		unsigned short code= (unsigned short) reader.getShort();
		unsigned short flags= (unsigned short) reader.getShort();
		int len= reader.getInt();
		if (code == 0)
			break;
//		fprintf(stderr,"%d %d\n",code,len);
		switch (code) {
		 case 145: // terrain sample fbuffer
		 case 146: // terrain sample ybuffer
		 case 147: // terrain sample ebuffer
		 case 148: // terrain sample nbuffer
		 case 149: // terrain sample cbuffer
		 case 150: // terrain sample dbuffer
		 case 281: // terrain sample asbuffer
		 case 282: // terrain sample usbuffer
		 case 138: // terrain always select maxdist
		 case 160: // terrain patchset dist
		 case 161: // terrain patchset npatches
			reader.getBytes(NULL,len);
			break;
		 case 140: // terrain nsamples
		 case 158: // terrain patchsets
			reader.getString();
			reader.getInt();
			break;
		 case 137: // terrain errthreshold_scale
		 case 141: // terrain sample rotation
		 case 144: // terrain sample size
			reader.getString();
			reader.getFloat();
			break;
		 case 136: // terrain
		 case 139: // terrain_samples
		 case 157: // terrain patches
		 case 159: // terrain patchset
		 case 163: // terrain patchset patches
			reader.getString();
			break;
		 case 142: // terrain sample floor
			reader.getString();
//			fprintf(stderr,"floor offset %d\n",ftell(reader.in));
			tile->floor= reader.getFloat();
			break;
		 case 143: // terrain sample scale
			reader.getString();
//			fprintf(stderr,"scale offset %d\n",ftell(reader.in));
			tile->scale= reader.getFloat();
			break;
		 case 164: // terrain patchset patch
			reader.getString();
			{
			patch->flags= reader.getInt();
			float f1= reader.getFloat();//coord
			float f2= reader.getFloat();
			float f3= reader.getFloat();//coord
			float f4= reader.getFloat();
			float f5= reader.getFloat();
			float f6= reader.getFloat();
			patch->centerX= f1;
			patch->centerZ= f3;
//			fprintf(stderr,"patch %f %f %f %f %f %f\n",
//			  f1,f2,f3,f4,f5,f6);
			patch->texIndex= (short) reader.getInt();
			patch->u0= reader.getFloat();
			patch->v0= reader.getFloat();
			patch->dudx= reader.getFloat();
			patch->dudz= reader.getFloat();
			patch->dvdx= reader.getFloat();
			patch->dvdz= reader.getFloat();
//			fprintf(stderr,"patch %d %f %f %f %f %f %f\n",
//			  patch->texIndex,patch->u0,patch->v0,
//			  patch->dudx,patch->dudz,patch->dvdx,patch->dvdz);
			reader.getFloat();//error threshold
			patch++;
			}
			break;
		 case 251: // water level?
			reader.getString();
			if (len == 17) {
				tile->swWaterLevel= reader.getFloat();
				tile->seWaterLevel= reader.getFloat();
				tile->neWaterLevel= reader.getFloat();
				tile->nwWaterLevel= reader.getFloat();
			} else if (len == 5) {
				tile->swWaterLevel= 
				tile->seWaterLevel=
				tile->neWaterLevel=
				tile->nwWaterLevel= reader.getFloat();
			} else {
				reader.getBytes(NULL,len);
			}
			break;
		 case 151: // terrain shaders
			reader.getString();
			{
				int n= reader.getInt();
				//fprintf(stderr,"%d terrain shaders\n",n);
			}
			break;
		 case 152: // terrain shader
			reader.getString();
			{
				int n= reader.getShort();
				string s= reader.getString(n);
//				fprintf(stderr," terrain shader %s\n",s.c_str());
			}
			break;
		 case 153: // terrain texslots
		 case 155: // terrain uvcalcs
			reader.getString();
			reader.getInt();
			break;
		 case 154: // terrain texslot
			reader.getString();
			{
				int n= reader.getShort();
				string s= reader.getString(n);
				int n1= reader.getInt();
				int n2= reader.getInt();
//				fprintf(stderr,"  texslot %s %d %d\n",
//				  s.c_str(),n1,n2);
				if (n2 == 0)
					tile->textures.push_back(s);
				else if (n2 == 1)
					tile->microTextures.push_back(s);
			}
			break;
		 case 156: // terrain uvcalc
			reader.getString();
			{
				int n1= reader.getInt();
				int n2= reader.getInt();
				int n3= reader.getInt();
				float f1= reader.getFloat();
				if (n3 == 1)
					tile->microTexUVMult= f1;
//				fprintf(stderr,"  uvcalc %d %d %d %f\n",
//				  n1,n2,n3,f1);
			}
			break;
		 case 1: // comment
//			fprintf(stderr,"%d %d\n",code,len);
			for (int i=0; i<len; i++) {
				Byte b;
				reader.getBytes(&b,1);
//				fprintf(stderr," %d",0xff&b);
//				if (i%16==15)
//					fprintf(stderr,"\n");
			}
//			fprintf(stderr,"\n");
			break;
		 default:
			fprintf(stderr,
			  "unsupported tfile content %u %d %s %d\n",
			  code,len,filename,reader.read);
			reader.getBytes(NULL,len);
			break;
		}
	}
	return 1;
}

//	reads and saves a tile's terrain data
void MSTSRoute::readTerrain(Tile* tile)
{
	if (tile->terrain != NULL)
		return;
	tile->terrain= new Terrain;
	string path= tilesDir+dirSep+tile->tFilename+"_y.raw";
	FILE* in= fopen(path.c_str(),"r");
	if (in == NULL) {
		fprintf(stderr,"cannot read %s\n",path.c_str());
		memset(tile->terrain->y,0,sizeof(tile->terrain->y));
	} else {
		if (fread(tile->terrain->y,sizeof(tile->terrain->y),1,in) != 1)
			fprintf(stderr,"cannot read %s\n",path.c_str());
		fclose(in);
	}
#if 0
	path= tilesDir+dirSep+tile->tFilename+"_n.raw";
	in= fopen(path.c_str(),"r");
	if (in == NULL) {
//		fprintf(stderr,"cannot read %s\n",path.c_str());
		memset(tile->terrain->n,0,sizeof(tile->terrain->n));
	} else {
		if (fread(tile->terrain->n,sizeof(tile->terrain->n),1,in) != 1)
			fprintf(stderr,"cannot read %s\n",path.c_str());
		fclose(in);
	}
#endif
	path= tilesDir+dirSep+tile->tFilename+"_f.raw";
	in= fopen(path.c_str(),"r");
	if (in == NULL) {
//		fprintf(stderr,"cannot read %s\n",path.c_str());
		memset(tile->terrain->f,0,sizeof(tile->terrain->f));
	} else {
		if (fread(tile->terrain->f,sizeof(tile->terrain->f),1,in) != 1)
			fprintf(stderr,"cannot read %s\n",path.c_str());
		fclose(in);
	}
}

//	writes terrain data
//	this was used once to build under water terrain based on an
//	electronic chart
void MSTSRoute::writeTerrain(Tile* tile)
{
	if (tile->terrain == NULL)
		return;
	string path= tilesDir+dirSep+tile->tFilename+"_y.raw";
	FILE* out= fopen(path.c_str(),"w");
	if (out == NULL) {
		fprintf(stderr,"cannot read %s\n",path.c_str());
		return;
	}
	fwrite(tile->terrain->y,1,sizeof(tile->terrain->y),out);
	fclose(out);
}

#if 0
void MSTSRoute::loadTerrainData(Tile* tile)
{
	int nt= 0;
	int ntm= 0;
	int nm= 0;
	for (TileMap::iterator i=tileMap.begin(); i!=tileMap.end(); ++i) {
		Tile* t= i->second;
		if (t->terrain)
			nt++;
		if (t->terrModel)
			ntm++;
		if (t->models)
			nm++;
		if (t->terrain==NULL &&
		  t->x>=tile->x-1 && t->x<=tile->x+1 &&
		  t->z>=tile->z-1 && t->z<=tile->z+1)
			readTerrain(t);
		else if (t->terrain!=NULL &&
		  (t->x<tile->x-1 || t->x>tile->x+1 ||
		   t->z<tile->z-1 || t->z>tile->z+1))
			t->freeTerrain();
		if (t->plod && t->plod->getNumChildren() > 1) {
			fprintf(stderr,"loaded %d %d %d %d %d",
			  t->x,t->z,t->plod->getNumChildren(),
			  t->plod->getNumChildrenThatCannotBeExpired(),
			  t->plod->getDisableExternalChildrenPaging());
			for (int j=1; j<t->plod->getNumChildren(); j++)
				fprintf(stderr," %d.%d",
				  t->plod->getFrameNumber(j)==
				  t->plod->getFrameNumberOfLastTraversal(),
				  t->plod->getTimeStamp(j));
			fprintf(stderr,"\n");
		} else {
			if (t->terrModel!=NULL &&
			  (t->x<tile->x-1 || t->x>tile->x+1 ||
			   t->z<tile->z-1 || t->z>tile->z+1)) {
//				fprintf(stderr,"unref tmodel %d %d %d\n",
//				  t->x,t->z,t->terrModel->referenceCount());
				t->terrModel->unref();
				t->terrModel= NULL;
			}
			if (t->models!=NULL &&
			  (t->x<tile->x-1 || t->x>tile->x+1 ||
			   t->z<tile->z-1 || t->z>tile->z+1)) {
//				fprintf(stderr,"unref models %d %d %d\n",
//				  t->x,t->z,t->models->referenceCount());
				t->models->unref();
				t->models= NULL;
			}
		}
	}
//	fprintf(stderr,"loadtd %d %d %d\n",nt,ntm,nm);
}
#endif

void MSTSRoute::Tile::freeTerrain()
{
	if (terrain != NULL)
		delete terrain;
	terrain= NULL;
}

MSTSRoute::Tile* MSTSRoute::findTile(int tx, int tz)
{
	TileMap::iterator i= tileMap.find(tileID(tx,tz));
	return i==tileMap.end() ? NULL : i->second;
}

//int checkTree(osg::Node* node);

//	makes a 3D model to show a map of unloaded tiles when viewing
//	the route from a long distance
//	the model is a green and white checker board with one square per tile
void MSTSRoute::makeTileMap(vsg::Group* root)
{
	vsgOptions->shaderSets["phong"]= vsg::createPhongShaderSet();
	vsgOptions->shaderSets["flat"]= vsg::createFlatShadedShaderSet();
	vsgOptions->add(MstsTerrainReader::create());
	vsgOptions->add(MstsWorldReader::create());
	if (!vsgOptions->sharedObjects)
		vsgOptions->sharedObjects= vsg::SharedObjects::create();
	vsg::StateInfo stateInfo;
	stateInfo.lighting= false;
	auto builder= vsg::Builder::create();
	builder->options= vsgOptions;
//	fprintf(stderr,"%d nodes before makeTileMap\n",checkTree(root));
	for (TileMap::iterator i=tileMap.begin(); i!=tileMap.end(); ++i) {
		Tile* tile= i->second;
		float x0= 2048*(float)(tile->x-centerTX);
		float z0= 2048*(float)(tile->z-centerTZ);
		vsg::GeometryInfo geomInfo;
		geomInfo.position= vsg::dvec3(x0,z0,tile->floor);
		geomInfo.dx= vsg::vec3(2048,0,0);
		geomInfo.dy= vsg::vec3(0,2048,0);
		geomInfo.dz= vsg::vec3(0,0,0);
		if ((tile->x&01) == (tile->z&01))
			geomInfo.color= vsg::vec4(1,1,1,1);
		else
			geomInfo.color= vsg::vec4(.7,1,.7,1);
		auto quad= builder->createQuad(geomInfo,stateInfo);
		vsg::ComputeBounds computeBounds;
		quad->accept(computeBounds);
		vsg::dvec3 center=
		  (computeBounds.bounds.min+computeBounds.bounds.max)*0.5;
		double radius= vsg::length(computeBounds.bounds.max-
		  computeBounds.bounds.min)*0.6;
		auto lod= vsg::PagedLOD::create();
		lod->options= vsgOptions;
		lod->bound.set(center.x,center.y,center.z,radius);
		lod->filename= tile->tFilename+"_y.raw";
		lod->children[0]= vsg::PagedLOD::Child{.6,{}};
		lod->children[1]= vsg::PagedLOD::Child{0,quad};
		root->addChild(lod);
	}
	vsg::CollectResourceRequirements crr;
	root->accept(crr);
	root->setObject("ResourceHints",
	  crr.createResourceHints(2*tileMap.size()));
//	fprintf(stderr,"%d nodes before makeTileMap\n",checkTree(root));
	for (TileMap::iterator i=tileMap.begin(); i!=tileMap.end(); ++i) {
		Tile* tile= i->second;
		int n= 0;
		for (int i=0; i<256; i++)
			if ((tile->patches[i].flags&0xc0) != 0)
				n++;
		if (n == 0)
			tile->swWaterLevel= -1e10;
	}
}

#if 0
//	overrides an MSTS track shape model with the specified OSG shape
void MSTSRoute::overrideTrackModel(string& shapeName, string& modelName)
{
	osg::Node* model= osgDB::readNodeFile(modelName);
	trackModelMap[shapeName]= model;
	model->ref();
}
#endif

#if 0
void MSTSRoute::cleanStaticModelMap()
{
	for (ModelMap::iterator i=staticModelMap.begin();
	  i!=staticModelMap.end(); i++) {
		if (i->second == NULL)
			continue;
		if (i->second->referenceCount() <= 1) {
//			fprintf(stderr,"unused %s %d\n",
//			  i->first.c_str(),i->second->referenceCount());
			i->second->unref();
			i->second= NULL;
//		} else {
//			fprintf(stderr,"ref count %s %d\n",
//			  i->first.c_str(),i->second->referenceCount());
		}
	}
}
#endif

#if 0
int checkTree(osg::Node* node)
{
	if (node == NULL) {
		fprintf(stderr,"null node\n");
		return 0;
	}
	osg::Geode* geode= dynamic_cast<osg::Geode*>(node);
	if (geode) {
		return 1;
	} else {
		osg::MatrixTransform* mt=
		  dynamic_cast<osg::MatrixTransform*>(node);
		osg::Group* group=
		  dynamic_cast<osg::Group*>(node);
		if (mt) {
			int s= 0;
			for (int i=0; i<mt->getNumChildren(); i++)
				s+= checkTree(mt->getChild(i));
			return s;
		} else if (group) {
			int s= 0;
			for (int i=0; i<group->getNumChildren(); i++)
				s+= checkTree(group->getChild(i));
			return s;
		} else {
			//fprintf(stderr,"unknown %s\n",node->className());
			return 1;
		}
	}
}
#endif

float MSTSRoute::Tile::getWaterLevel(int i, int j)
{
	float x= 1-.0625*j;
	float z= .0625*i;
//	fprintf(stderr,"wl %d %d %f %f %f\n",i,j,x,z,
//	  x*(1-z)*nwWaterLevel + x*z*swWaterLevel +
//	  (1-x)*z*seWaterLevel + (1-x)*(1-z)*neWaterLevel);
	return x*(1-z)*nwWaterLevel + x*z*swWaterLevel +
	  (1-x)*z*seWaterLevel + (1-x)*(1-z)*neWaterLevel;
}

#if 0
//	makes a 3D model for water in a tile
void MSTSRoute::makeWater(Tile* tile, float dl, const char* texture,
  int renderBin)
{
//	fprintf(stderr,"makeWater %d %f\n",drawWater,tile->swWaterLevel);
//	for (int i=0; i<256; i++)
//		fprintf(stderr," %x\n",tile->patches[i].flags);
	if (!drawWater || tile->swWaterLevel==-1e10)
		return;
//	fprintf(stderr,"makeWater %p %f sw=%f se=%f ne=%f nw=%f\n",
//	  tile,dl,tile->swWaterLevel,tile->seWaterLevel,
//	  tile->neWaterLevel,tile->nwWaterLevel);
	osg::Vec3Array* verts= new osg::Vec3Array;
	osg::Vec2Array* texCoords= new osg::Vec2Array;
	osg::DrawElementsUShort* drawElements=
	  new osg::DrawElementsUShort(GL_QUADS);;
	float x0= 2048*(float)(tile->x-centerTX);
	float z0= 2048*(float)(tile->z-centerTZ);
	int n= 0;
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			Patch* patch= &tile->patches[i*16+j];
			if ((patch->flags&0xc0) == 0)
				continue;
			float x= x0 + 128*(j-8);
			float z= z0 + 128*(8-i);
//			fprintf(stderr,"patch flags %d %d %x %f %f %f %f %f\n",
//			  i,j,patch->flags,x-x0,z-z0,
//			  patch->centerX,patch->centerZ,
//			  tile->getWaterLevel(i,j));
			verts->push_back(osg::Vec3(x,z,
			  tile->getWaterLevel(i,j)+dl));
			verts->push_back(osg::Vec3(x,z-128,
			  tile->getWaterLevel(i+1,j)+dl));
			verts->push_back(osg::Vec3(x+128,z-128,
			  tile->getWaterLevel(i+1,j+1)+dl));
			verts->push_back(osg::Vec3(x+128,z,
			  tile->getWaterLevel(i,j+1)+dl));
			texCoords->push_back(osg::Vec2(.05,.05));
			texCoords->push_back(osg::Vec2(.05,.95));
			texCoords->push_back(osg::Vec2(.95,.95));
			texCoords->push_back(osg::Vec2(.95,.05));
			drawElements->push_back(n);
			drawElements->push_back(n+1);
			drawElements->push_back(n+2);
			drawElements->push_back(n+3);
			n+= 4;
		}
	}
	osg::Geometry* geometry= new osg::Geometry;
	geometry->setVertexArray(verts);
	osg::Vec4Array* colors= new osg::Vec4Array;
	colors->push_back(osg::Vec4(1,1,1,1));
	geometry->setColorArray(colors);
	geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
	osg::Vec3Array* norms= new osg::Vec3Array;
	norms->push_back(osg::Vec3(0,0,1));
	geometry->setNormalArray(norms);
	geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
	geometry->setTexCoordArray(0,texCoords);
	geometry->addPrimitiveSet(drawElements);
	osg::Texture2D* t= new osg::Texture2D;
	string envDir= fixFilenameCase(routeDir+dirSep+"ENVFILES");
	string envTexDir= fixFilenameCase(envDir+dirSep+"TEXTURES");
	string path= fixFilenameCase(envTexDir+dirSep+texture);
	osg::Image* image= readMSTSACE(path.c_str());
	if (image != NULL)
		t->setImage(image);
	t->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::REPEAT);
	t->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::REPEAT);
	osg::StateSet* stateSet= geometry->getOrCreateStateSet();
	stateSet->setTextureAttributeAndModes(0,t,osg::StateAttribute::ON);
	osg::Material* m= new osg::Material;
	m->setAmbient(osg::Material::FRONT_AND_BACK,osg::Vec4(.6,.6,.6,1));
	m->setDiffuse(osg::Material::FRONT_AND_BACK,osg::Vec4(.4,.4,.4,1));
	m->setSpecular(osg::Material::FRONT_AND_BACK,osg::Vec4(1,1,1,1));
//	m->setShininess(osg::Material::FRONT_AND_BACK,4.);
	m->setShininess(osg::Material::FRONT_AND_BACK,128.);
	stateSet->setAttribute(m,osg::StateAttribute::ON);
	addShaders(stateSet,.3);
//	stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	if (renderBin > 0)
		stateSet->setRenderBinDetails(renderBin,"DepthSortedBin");
	osg::BlendFunc* bf= new osg::BlendFunc();
	bf->setFunction(osg::BlendFunc::SRC_ALPHA,
	  osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
	stateSet->setAttributeAndModes(bf,osg::StateAttribute::ON);
	osg::Geode* geode= new osg::Geode;
	geode->addDrawable(geometry);
	tile->models->addChild(geode);
}
#endif

#if 0
void addClipTriangle(int j1, int j2, int j3, osg::Vec3Array* verts,
  osg::DrawElementsUInt* drawElements, int axis, float w, float h,
  osg::Vec3Array* normals)
{
	float* v= (float*) verts->getDataPointer();
	float* v1= v+3*j1;
	float* v2= v+3*j2;
	float* v3= v+3*j3;
	float half= axis==0 ? w/2 : h/2;
	if (v1[axis]<=-half && v2[axis]<=-half && v3[axis]<=-half)
		return;
	if (v1[axis]>=half && v2[axis]>=half && v3[axis]>=half)
		return;
	if (v2[axis]<v1[axis] && v2[axis]<v3[axis]) {
		addClipTriangle(j2,j3,j1,verts,drawElements,axis,w,h,normals);
		return;
	}
	if (v3[axis]<v1[axis] && v3[axis]<v2[axis]) {
		addClipTriangle(j3,j1,j2,verts,drawElements,axis,w,h,normals);
		return;
	}
	if (v1[axis]>=-half && v2[axis]<=half && v3[axis]<=half) {
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
//			fprintf(stderr,"transfer add %7.3f %7.3f %7.3f  "
//			  "%7.3f %7.3f %7.3f  %7.3f %7.3f %7.3f\n",
//			  v1[0],v1[1],v1[2],v2[0],v2[1],v2[2],
//			  v3[0],v3[1],v3[2]);
		}
		return;
	}
	int axis2= 2-axis;
	if (v1[axis]<-half && v2[axis]<-half) {
		float x23= axis==0 ? -half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],-half);
		float y23= segAxisInt(v2[axis],v2[1],v3[axis],v3[1],-half);
		float z23= axis==2 ? -half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],-half);
		float x13= axis==0 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],-half);
		float y13= segAxisInt(v1[axis],v1[1],v3[axis],v3[1],-half);
		float z13= axis==2 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],-half);
//		fprintf(stderr,"transfer cut12-%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j1= verts->getNumElements();
		verts->push_back(osg::Vec3(x13,y13,z13));
		normals->push_back(osg::Vec3(0,0,1));
		j2= verts->getNumElements();
		verts->push_back(osg::Vec3(x23,y23,z23));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
		}
	} else if (v1[axis]<-half && v3[axis]<-half) {
		float x12= axis==0 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],-half);
		float y12= segAxisInt(v1[axis],v1[1],v2[axis],v2[1],-half);
		float z12= axis==2 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],-half);
		float x23= axis==0 ? -half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],-half);
		float y23= segAxisInt(v2[axis],v2[1],v3[axis],v3[1],-half);
		float z23= axis==2 ? -half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],-half);
//		fprintf(stderr,"transfer cut13-%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j1= verts->getNumElements();
		verts->push_back(osg::Vec3(x12,y12,z12));
		normals->push_back(osg::Vec3(0,0,1));
		j3= verts->getNumElements();
		verts->push_back(osg::Vec3(x23,y23,z23));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
		}
	} else if (v1[axis]<-half) {
		float x12= axis==0 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],-half);
		float z12= axis==2 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],-half);
		float y12= segAxisInt(v1[axis],v1[1],v2[axis],v2[1],-half);
		float x13= axis==0 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],-half);
		float z13= axis==2 ? -half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],-half);
		float y13= segAxisInt(v1[axis],v1[1],v3[axis],v3[1],-half);
//		fprintf(stderr,"transfer cut1-%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j1= verts->getNumElements();
		verts->push_back(osg::Vec3(x12,y12,z12));
		normals->push_back(osg::Vec3(0,0,1));
		int j4= verts->getNumElements();
		verts->push_back(osg::Vec3(x13,y13,z13));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
			addClipTriangle(j1,j3,j4,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
			drawElements->push_back(j1);
			drawElements->push_back(j3);
			drawElements->push_back(j4);
		}
	} else if (v2[axis]>half && v3[axis]>half) {
		float x12= axis==0 ? half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],half);
		float z12= axis==2 ? half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],half);
		float y12= segAxisInt(v1[axis],v1[1],v2[axis],v2[1],half);
		float x13= axis==0 ? half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],half);
		float z13= axis==2 ? half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],half);
		float y13= segAxisInt(v1[axis],v1[1],v3[axis],v3[1],half);
//		fprintf(stderr,"transfer cut23+%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j2= verts->getNumElements();
		verts->push_back(osg::Vec3(x12,y12,z12));
		normals->push_back(osg::Vec3(0,0,1));
		j3= verts->getNumElements();
		verts->push_back(osg::Vec3(x13,y13,z13));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
		}
	} else if (v2[axis]>half) {
		float x12= axis==0 ? half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],half);
		float z12= axis==2 ? half :
		  segAxisInt(v1[axis],v1[axis2],v2[axis],v2[axis2],half);
		float y12= segAxisInt(v1[axis],v1[1],v2[axis],v2[1],half);
		float x23= axis==0 ? half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],half);
		float z23= axis==2 ? half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],half);
		float y23= segAxisInt(v2[axis],v2[1],v3[axis],v3[1],half);
//		fprintf(stderr,"transfer cut2+%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j2= verts->getNumElements();
		verts->push_back(osg::Vec3(x12,y12,z12));
		normals->push_back(osg::Vec3(0,0,1));
		int j4= verts->getNumElements();
		verts->push_back(osg::Vec3(x23,y23,z23));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j4,verts,drawElements,2-axis,w,h,
			  normals);
			addClipTriangle(j1,j4,j3,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j4);
			drawElements->push_back(j1);
			drawElements->push_back(j4);
			drawElements->push_back(j3);
		}
	} else if (v3[axis]>half) {
		float x23= axis==0 ? half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],half);
		float z23= axis==2 ? half :
		  segAxisInt(v2[axis],v2[axis2],v3[axis],v3[axis2],half);
		float y23= segAxisInt(v2[axis],v2[1],v3[axis],v3[1],half);
		float x13= axis==0 ? half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],half);
		float z13= axis==2 ? half :
		  segAxisInt(v1[axis],v1[axis2],v3[axis],v3[axis2],half);
		float y13= segAxisInt(v1[axis],v1[1],v3[axis],v3[1],half);
//		fprintf(stderr,"transfer cut3+%d %f %f %f %f %f %f\n",axis,
//		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
		j3= verts->getNumElements();
		verts->push_back(osg::Vec3(x23,y23,z23));
		normals->push_back(osg::Vec3(0,0,1));
		int j4= verts->getNumElements();
		verts->push_back(osg::Vec3(x13,y13,z13));
		normals->push_back(osg::Vec3(0,0,1));
		if (axis == 0) {
			addClipTriangle(j1,j2,j3,verts,drawElements,2-axis,w,h,
			  normals);
			addClipTriangle(j1,j3,j4,verts,drawElements,2-axis,w,h,
			  normals);
		} else {
			drawElements->push_back(j1);
			drawElements->push_back(j2);
			drawElements->push_back(j3);
			drawElements->push_back(j1);
			drawElements->push_back(j3);
			drawElements->push_back(j4);
		}
	} else {
		fprintf(stderr,"transfer dont add %f %f %f %f %f %f\n",
		  v1[0],v1[2],v2[0],v2[2],v3[0],v3[2]);
	}
}
#endif

#if 0
osg::Node* MSTSRoute::makeTransfer(MSTSFileNode* transfer, string* filename,
  Tile* tile, MSTSFileNode* pos, MSTSFileNode* qdir)
{
	osg::Quat quat(
	  atof(qdir->getChild(0)->value->c_str()),
	  atof(qdir->getChild(1)->value->c_str()),
	  atof(qdir->getChild(2)->value->c_str()),
	  atof(qdir->getChild(3)->value->c_str()));
	osg::Vec3 center= osg::Vec3(atof(pos->getChild(0)->value->c_str()),
	  atof(pos->getChild(1)->value->c_str()),
	  atof(pos->getChild(2)->value->c_str()));
	MSTSFileNode* wid= transfer->children->find("Width");
	MSTSFileNode* hgt= transfer->children->find("Height");
	float w= atof(wid->getChild(0)->value->c_str());
	float h= atof(hgt->getChild(0)->value->c_str());
	return makeTransfer(filename,tile,center,quat,w,h);
}
#endif

#if 0
osg::Node* MSTSRoute::makeTransfer(string* filename, Tile* tile,
  osg::Vec3 center, osg::Quat quat, float w, float h)
{
	float x0= 2048*(tile->x-centerTX);
	float z0= 2048*(tile->z-centerTZ);
	double a,x,y,z;
	quat.getRotate(a,x,y,z);
	osg::Quat q;
	q.makeRotate(a,x,y,z);
	osg::Matrix rot(q);
	q.makeRotate(-a,x,y,z);
	osg::Matrix rot1(q);
//	if (w<8 || h<8)
//		fprintf(stderr,"small transfer %s %f %f\n",
//		  filename->c_str(),w,h);
//	fprintf(stderr,"transfer %s %f %f\n",filename->c_str(),w,h);
//	fprintf(stderr,"transfer rot %lf %lf %lf %lf\n",180*a/3.14159,x,y,z);
//	fprintf(stderr,"transfer pos %lf %lf %lf\n",
//	  center[0],center[1],center[2]);
	osg::Texture2D* t= new osg::Texture2D;
	t->setDataVariance(osg::Object::DYNAMIC);
	string path= rTexturesDir+dirSep+(*filename);
	osg::Image* image= readMSTSACE(path.c_str());
	if (image != NULL)
		t->setImage(image);
	t->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_EDGE);
	t->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_EDGE);
	t->setBorderColor(osg::Vec4(.3,.3,.3,0));
	osg::Material* mat= new osg::Material;
//	mat->setAmbient(osg::Material::FRONT_AND_BACK,osg::Vec4(.6,.6,.6,1));
//	mat->setDiffuse(osg::Material::FRONT_AND_BACK,osg::Vec4(.4,.4,.4,1));
	mat->setAmbient(osg::Material::FRONT_AND_BACK,osg::Vec4(1,1,1,1));
	mat->setDiffuse(osg::Material::FRONT_AND_BACK,osg::Vec4(0,0,0,1));
	osg::Geode* geode= new osg::Geode;
	for (int i=0; i<tile->terrModel->getNumDrawables(); i++) {
		osg::Geometry* patchGeom=
		  dynamic_cast<osg::Geometry*>(tile->terrModel->getDrawable(i));
		if (!patchGeom)
			continue;
		osg::Vec3Array* pVerts=
		  (osg::Vec3Array*) patchGeom->getVertexArray();
		float* v= (float*) pVerts->getDataPointer();
		float minx= 1e10;
		float maxx= -1e10;
		float minz= 1e10;
		float maxz= -1e10;
		for (int j=0; j<pVerts->getNumElements(); j++) {
			int j3= j*3;
			osg::Vec3 p=
			  rot1*(osg::Vec3(v[j3]-x0,v[j3+2],v[j3+1]-z0)-center);
			if (minx > p[0])
				minx= p[0];
			if (maxx < p[0])
				maxx= p[0];
			if (minz > p[2])
				minz= p[2];
			if (maxz < p[2])
				maxz= p[2];
		}
		if (maxx<-w/2 || minx>w/2 || maxz<-h/2 || minz>h/2)
			continue;
//		fprintf(stderr,"transfer patch %d %p\n",i,tile);
		osg::Vec3Array* pNormals=
		  (osg::Vec3Array*) patchGeom->getVertexArray();
		float* n= (float*) pNormals->getDataPointer();
		osg::Vec3Array* verts= new osg::Vec3Array;
		osg::Vec3Array* normals= new osg::Vec3Array;
		for (int j=0; j<pVerts->getNumElements(); j++) {
			int j3= j*3;
			osg::Vec3 p= rot1*
			  (osg::Vec3(v[j3]-x0,v[j3+2],v[j3+1]-z0)-center);
//			if ((-w/2<=p[0] && p[0]<=w/2) &&
//			  (-h/2<=p[2] && p[2]<=h/2)) {
//				p[1]+= .05;
//			}
			verts->push_back(p);
			p= rot1*(osg::Vec3(n[j3],n[j3+2],n[j3+1]));
			normals->push_back(p);
		}
		osg::Vec3 sum(0,0,0);
		osg::Geometry* geometry= new osg::Geometry;
		for (int j=0; j<patchGeom->getNumPrimitiveSets(); j++) {
			osg::DrawElementsUInt* patchDE= (osg::DrawElementsUInt*)
			  patchGeom->getPrimitiveSet(j)->getDrawElements();
			osg::DrawElementsUInt* drawElements=
			  new osg::DrawElementsUInt(GL_TRIANGLES);;
			for (int k=0; k<patchDE->getNumIndices(); k+=3) {
				int j1= patchDE->getElement(k);
				int j2= patchDE->getElement(k+1);
				int j3= patchDE->getElement(k+2);
				addClipTriangle(j1,j2,j3,verts,drawElements,
				  0,w,h,normals);
			}
//			fprintf(stderr,"transfer triangles %d\n",
//			  drawElements->getNumIndices());
			if (drawElements->getNumIndices() == 0) {
				drawElements->unref();
				continue;
			}
			geometry->addPrimitiveSet(drawElements);
			v= (float*) verts->getDataPointer();
			for (int k=0; k<drawElements->getNumIndices(); k+=3) {
				int j1= drawElements->getElement(k);
				int j2= drawElements->getElement(k+1);
				int j3= drawElements->getElement(k+2);
				osg::Vec3 p1(v[j1*3],v[j1*3+1],v[j1*3+2]);
				osg::Vec3 p2(v[j2*3],v[j2*3+1],v[j2*3+2]);
				osg::Vec3 p3(v[j3*3],v[j3*3+1],v[j3*3+2]);
				osg::Vec3 cross= (p2-p1)^(p3-p1);
				cross.normalize();
				sum+= cross;
			}
		}
		geometry->setVertexArray(verts);
		geometry->setColorArray(patchGeom->getColorArray());
		geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
		geometry->setNormalArray(normals);
		geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
		sum.normalize();
//		fprintf(stderr,"transfer normal %f %f %f\n",
//		  sum[0],sum[1],sum[2]);
		osg::Vec2Array* texCoords= new osg::Vec2Array;
		float* xyz= (float*) verts->getDataPointer();
		for (int j=0; j<verts->getNumElements(); j++) {
			int j3= j*3;
			float u= xyz[j3]/w + .5;
			float v= -xyz[j3+2]/h + .5;
			texCoords->push_back(osg::Vec2(u,v));
//			for (int k=0; k<3; k++)
//				xyz[j3+k]-= .03*sum[k];
		}
		geometry->setTexCoordArray(0,texCoords);
		osg::StateSet* stateSet= geometry->getOrCreateStateSet();
		stateSet->setTextureAttributeAndModes(0,t,
		  osg::StateAttribute::ON);
	//	stateSet->setAttribute(mat,osg::StateAttribute::ON);
		stateSet->setAttributeAndModes(mat,osg::StateAttribute::ON);
//		stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
//		stateSet->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
		stateSet->setRenderBinDetails(3,"DepthSortedBin");
		osg::Depth* depth= new osg::Depth(osg::Depth::LEQUAL,0,1,false);
		stateSet->setAttribute(depth,osg::StateAttribute::ON);
		osg::PolygonOffset* offset= new osg::PolygonOffset(-10,-10);
		stateSet->setAttributeAndModes(offset,osg::StateAttribute::ON);
		osg::BlendFunc* bf= new osg::BlendFunc();
		bf->setFunction(osg::BlendFunc::SRC_ALPHA,
		  osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
		stateSet->setAttributeAndModes(bf,osg::StateAttribute::ON);
		stateSet->setMode(GL_LIGHTING,osg::StateAttribute::ON);
		geode->addDrawable(geometry);
	}
	return geode;
}
#endif

#if 0
//	Changes the navigable water to match the route
//	can also adjust the route's terrain to match the water depth
void MSTSRoute::adjustWater(int setTerrain)
{
	if (waterList.size() == 0)
		return;
	for (WaterList::iterator i=waterList.begin(); i!=waterList.end(); ++i) {
		Water* w= *i;
		w->waterLevel= waterLevelDelta;
		for (Water::VertexMap::iterator i=w->vertexMap.begin();
		  i!=w->vertexMap.end(); ++i) {
			Water::Vertex* v= i->second;
			double x,y;
			ll2xy(v->xy[1],v->xy[0],&x,&y);
			v->xy[0]= x;
			v->xy[1]= y;
		}
		for (Water::TriangleList::iterator j=w->triangleList.begin();
		  j!=w->triangleList.end(); ++j) {
			Water::Triangle* t= *j;
			t->area= Water::area(t->v[0],t->v[1],t->v[2]);
		}
	}
	if (!setTerrain)
		return;
	for (TileMap::iterator i=tileMap.begin(); i!=tileMap.end(); ++i) {
		Tile* t= i->second;
//		fprintf(stderr,"%d %d %f %f\n",t->x,t->z,t->floor,t->scale);
		readTerrain(t);
		float x0= 2048*(t->x-centerTX);
		float z0= 2048*(t->z-centerTZ);
//		fprintf(stderr,"%f %f\n",x0,z0);
#if 0
		Water::Location loc;
		for (int j=0; j<256; j++) {
			fprintf(stderr,"%d",j);
			for (int k=0; k<256; k++) {
				double x= x0+8*(k-128);
				double z= z0+8*(128-j);
				if (!loc.update(x,z))
					continue;
				float d= waterLevelDelta - loc.depth;
				t->terrain->y[j][k]= d<t->floor ? 0 :
				  (int)((d-t->floor)/t->scale);
				fprintf(stderr,"%d %d %d %f\n",
				  j,k,t->terrain->y[j][k],d);
			}
		}
#else
		for (WaterList::iterator j=waterList.begin();
		  j!=waterList.end(); ++j) {
			Water* w= *j;
			int n= 0;
			for (Water::TriangleList::iterator
			  k=w->triangleList.begin();
			  k!=w->triangleList.end(); ++k) {
				Water::Triangle* tri= *k;
				double minX= 1e10;
				double maxX= -1e10;
				double minY= 1e10;
				double maxY= -1e10;
				for (int i1=0; i1<3; i1++) {
					Water::Vertex* v= tri->v[i1];
					if (minX > v->xy[0])
						minX= v->xy[0];
					if (minY > v->xy[1])
						minY= v->xy[1];
					if (maxX < v->xy[0])
						maxX= v->xy[0];
					if (maxY < v->xy[1])
						maxY= v->xy[1];
				}
//				fprintf(stderr,"%lf %lf %lf %lf\n",
//				  minX,maxX,minY,maxY);
				if (maxX<x0-1024 || minX>x0+1024 ||
				  maxY<z0-1024 || minY>z0+1024)
					continue;
				int minK= (int)((minX-x0)/8 + 128);
				int maxK= (int)((maxX-x0)/8 + 128);
				int minJ= (int)(128 - (maxY-z0)/8);
				int maxJ= (int)(128 - (minY-z0)/8);
//				fprintf(stderr,"%d %d %d %d\n",
//				  minJ,maxJ,minK,maxK);
				if (minJ < 0)
					minJ= 0;
				if (maxJ > 255)
					maxJ= 255;
				if (minK < 0)
					minK= 0;
				if (maxK > 255)
					maxK= 255;
				n++;
				Water::Location loc;
				for (int j1=minJ; j1<=maxJ; j1++) {
					for (int k1=minK; k1<=maxK; k1++) {
						double x= x0+8*(k1-128);
						double z= z0+8*(128-j1);
						loc.triangle= tri;
						if (!loc.update(x,z))
							continue;
						float d= waterLevelDelta -
						  loc.depth;
						t->terrain->y[j1][k1]= 
						  d<t->floor ? 0 :
						  (int)((d-t->floor)/
						  t->scale);
//						fprintf(stderr,"%d %d %d %f\n",
//						  j1,k1,t->terrain->y[j1][k1],
//						  d);
					}
				}
			}
			fprintf(stderr,"n=%d\n",n);
		}
#endif
		writeTerrain(t);
		t->freeTerrain();
	}
}
#endif

#if 0
//	makes an MSTS marker file to match the navigable water's shore line
void MSTSRoute::saveShoreMarkers(const char* filename)
{
	for (WaterList::iterator i=waterList.begin(); i!=waterList.end(); ++i) {
		Water* w= *i;
		for (Water::VertexMap::iterator j=w->vertexMap.begin();
		  j!=w->vertexMap.end(); ++j) {
			Water::Vertex* v= j->second;
			v->label= 0;
		}
		for (Water::TriangleList::iterator j=w->triangleList.begin();
		  j!=w->triangleList.end(); ++j) {
			Water::Triangle* t= *j;
			for (int k=0; k<3; k++) {
				if (t->adj[k] != NULL)
					continue;
				t->v[k]->label= 1;
				int k1= (k+1)%3;
				t->v[k1]->label= 1;
			}
		}
	}
	FILE* out= fopen(filename,"w");
	if (out == NULL) {
		fprintf(stderr,"cannot create %s\n",filename);
		return;
	}
	fprintf(out,"SIMISA@@@@@@@@@@JINXOIOt______\n");
	int n= 0;
	for (WaterList::iterator i=waterList.begin(); i!=waterList.end(); ++i) {
		Water* w= *i;
		for (Water::VertexMap::iterator j=w->vertexMap.begin();
		  j!=w->vertexMap.end(); ++j) {
			Water::Vertex* v= j->second;
			if (v->label == 0)
				continue;
			double x= v->xy[0] + centerTX*2048 - 20015000 +
			  16385*2048 - 1024;
			double y= v->xy[1] + centerTZ*2048 + 8673000 -
			  16385*2048 + 3072;
			double lat,lng;
			ghProj.xy2ll(x,y,&lat,&lng);
			fprintf(out,"Marker ( %f %f %d 2 )\n",lng,lat,n++);
		}
	}
	fclose(out);
}
#endif

//	returns the altitude of terrain point i,j
float MSTSRoute::getAltitude(int i, int j,
  Tile* tile, Tile* t12, Tile* t21, Tile* t22)
{
	if (tile==NULL || tile->terrain==NULL)
		return 0;
	if (i < 0) {
		Tile* t= findTile(tile->x,tile->z+1);
		if (t && t->terrain)
			return getAltitude(i+256,j,t,NULL,NULL,NULL);
		i= 0;
	}
	if (j < 0) {
		Tile* t= findTile(tile->x-1,tile->z);
		if (t && t->terrain)
			return getAltitude(i,j+256,t,NULL,NULL,NULL);
		j= 0;
	}
	if (i<256 && j<256)
		return tile->floor + tile->scale*tile->terrain->y[i][j];
	else if (i<256 && j>=256)
	  if (t21!=NULL && t21->terrain!=NULL)
		return t21->floor+ t21->scale*t21->terrain->y[i][j-256];
	  else
		return tile->floor+ tile->scale*tile->terrain->y[i][255];
	else if (i>=256 && j<256)
	  if (t12!=NULL && t12->terrain!=NULL)
		return t12->floor+ t12->scale*t12->terrain->y[i-256][j];
	  else
		return tile->floor+ tile->scale*tile->terrain->y[255][j];
	else if (i>=256 && j>=256)
	  if (t22!=NULL && t22->terrain!=NULL)
		return t22->floor+ t22->scale*t22->terrain->y[i-256][j-256];
	  else
		return tile->floor+ tile->scale*tile->terrain->y[255][255];
	return 0;
}

float MSTSRoute::getAltitude(float x, float z,
  Tile* tile, Tile* t12, Tile* t21, Tile* t22)
{
	int j= (int)floor(x/8) + 128;
	int i= 128 - (int)floor(z/8);
	if (j < 0) {
		j+= 256;
		x+= 2048;
		t21= tile;
		t22= t12;
		tile= findTile(t21->x-1,t21->z);
		t12= findTile(t21->x-1,t21->z-1);
	}
	if (i < 0) {
		i+= 256;
		z-= 2048;
		t12= tile;
		t22= t21;
		tile= findTile(t12->x,t12->z+1);
		t21= findTile(t12->x+1,t12->z+1);
	}
	float x0= 8*(j-128);
	float z0= 8*(128-i);
	float wx= (x-x0)/8;
	float wz= (z-z0)/8;
	float a00= getAltitude(i,j,tile,t12,t21,t22);
	float a01= getAltitude(i-1,j,tile,t12,t21,t22);
	float a11= getAltitude(i-1,j+1,tile,t12,t21,t22);
	float a10= getAltitude(i,j+1,tile,t12,t21,t22);
	float a= (1-wx)*(1-wz)*a00 + wx*(1-wz)*a10 + wx*wz*a11 + (1-wx)*wz*a01;
//	fprintf(stderr,"getalt %f %f %f %f %d %d %f %f %f %f\n",
//	  x,z,wx,wz,i,j,a00,a10,a11,a01);
#if 0
	if (tile->terrModel) {
		float tx0= 2048*(tile->x-centerTX);
		float tz0= 2048*(tile->z-centerTZ);
		osgSim::LineOfSight::Intersections hits=
		  osgSim::LineOfSight::computeIntersections(tile->terrModel,
		   osg::Vec3(x+tx0,z+tz0,1e7),
		   osg::Vec3(x+tx0,z+tz0,0),0x7fffffff);
		if (hits.size() > 0)
			a= hits[0][2];
//		for (int i=0; i<hits.size(); i++)
//			fprintf(stderr,
//			  "getalt %f %f %f %f %d %d %f %f %f %f %f %f %f\n",
//			  x,z,wx,wz,i,j,a00,a10,a11,a01,
//			  hits[i][0],hits[i][1],hits[i][2]);
	}
#endif
	return a;
}

//	gets a terrain normal
vsg::vec3 MSTSRoute::getNormal(int i, int j,
  Tile* tile, Tile* t12, Tile* t21, Tile* t22)
{
	float a0m= getAltitude(i-1,j,tile,t12,t21,t22);
	float am0= getAltitude(i,j-1,tile,t12,t21,t22);
	float a0p= getAltitude(i+1,j,tile,t12,t21,t22);
	float ap0= getAltitude(i,j+1,tile,t12,t21,t22);
	auto n= normalize(vsg::vec3(am0-ap0,a0p-a0m,16));
//	n.normalize();
//	fprintf(stderr,"normal %f %f %f %f %f %f\n",
//	  a00,a10,a01,n.x(),n.y(),n.z());
	return n;
}

//	returns true if terrain vertex is hidden
bool MSTSRoute::getVertexHidden(int i, int j,
  Tile* tile, Tile* t12, Tile* t21, Tile* t22)
{
	if (ignoreHiddenTerrain)
		return false;
	if (i<256 && j<256)
		return (tile->terrain->f[i][j]&0x04)!=0;
	else if (i<256 && j>=256)
	  if (t21 != NULL)
		return (t21->terrain->f[i][j-256]&0x04)!=0;
	  else
		return (tile->terrain->f[i][255]&0x04)!=0;
	else if (i>=256 && j<256)
	  if (t12 != NULL)
		return (t12->terrain->f[i-256][j]&0x04)!=0;
	  else
		return (tile->terrain->f[255][j]&0x04)!=0;
	else if (i>=256 && j>=256)
	  if (t22 != NULL)
		return (t22->terrain->f[i-256][j-256]&0x04)!=0;
	  else
		return (tile->terrain->f[255][255]&0x04)!=0;
	return false;
}

#if 0
void findPassingPoints(Track::Path* aiPath, Track::Path* playerPath)
{
	typedef set<Track::SSEdge*> SSEdgeSet;
	SSEdgeSet ssEdgeSet;
	for (Track::Path::Node* pn=playerPath->firstNode; pn!=NULL;
	  pn=pn->next)
		if (pn->nextSSEdge)
			ssEdgeSet.insert(pn->nextSSEdge);
	bool overlap= true;
	for (Track::Path::Node* pn=aiPath->firstNode; pn!=NULL; pn=pn->next) {
		if (pn->nextSSEdge == NULL)
			continue;
		SSEdgeSet::iterator i= ssEdgeSet.find(pn->nextSSEdge);
		if (i!=ssEdgeSet.end() && !overlap) {
			pn->type= Track::Path::MEET;
			fprintf(stderr,"  aimeet %p %d\n",pn,pn->type);
		}
		overlap= i!=ssEdgeSet.end();
	}
}
#endif

#if 0
extern double simTime;

void MSTSRoute::loadActivity(osg::Group* root, int activityFlags)
{
	if (activityName.size()==0) {
		simTime= 12*3600;
		loadExploreConsist(root);
		return;
	}
	Activity activity;
	string path= routeDir+dirSep+"ACTIVITIES"+dirSep+activityName;
//	fprintf(stderr,"path=%s\n",path.c_str());
	activity.readFile(path.c_str());
	simTime= activity.startTime;
	if (timeTable) {
		tt::Station* start= timeTable->findStation("start");
		if (start == NULL)
			start= timeTable->addStation("start");
		for (Traffic* t=activity.traffic; t!=NULL; t=t->next) {
			fprintf(stderr,"traffic %s %d\n",
			  t->service.c_str(),t->startTime);
			Track::Path* path= loadService(t->service,root,false,
			  t->id);
			tt::Train* train= timeTable->addTrain(t->service);
			train->setSchedTime(start,
			  t->startTime-60,t->startTime,0);
			train->path= path;
		}
	}
	if ((activityFlags&01) != 0) {
		Track::Path* playerPath=
		  loadService(activity.playerService,root,true,0);
		if (playerPath && timeTable) {
			for (int i=0; i<timeTable->getNumTrains(); i++) {
				tt::Train* train= timeTable->getTrain(i);
				if (train->getPrevTrain()==NULL)
					findPassingPoints(train->path,
					  playerPath);
			}
		}
	}
	for (LooseConsist* c=activity.consists; c!=NULL; c=c->next) {
//		fprintf(stderr,"consist %d %d %d %d %f %f\n",
//		  c->id,c->direction,c->tx,c->tz,c->x,c->z);
//		for (Wagon* w=c->wagons; w!=NULL; w=w->next)
//			fprintf(stderr," wagon %s %s\n",
//			  w->dir.c_str(),w->name.c_str());
		loadConsist(c,root);
	}
	for (Event* e=activity.events; e!=NULL; e=e->next) {
		eventMap[e->id]= e;
	}
}

void MSTSRoute::loadExploreConsist(osg::Group* root)
{
	string trainsDir= fixFilenameCase(mstsDir+dirSep+"TRAINS");
	string trainsetDir= fixFilenameCase(trainsDir+dirSep+"TRAINSET");
	string consistsDir= fixFilenameCase(trainsDir+dirSep+"CONSISTS");
	string path= fixFilenameCase(consistsDir+dirSep+consistName);
//	fprintf(stderr,"path=%s\n",path.c_str());
	MSTSFile conFile;
	conFile.readFile(path.c_str());
//	fprintf(stderr,"first=%p\n",conFile.getFirstNode());
	MSTSFileNode* cfg= conFile.getFirstNode()->get("TrainCfg");
//	fprintf(stderr,"cfg=%p\n",cfg);
	Train* train= new Train;
	for (MSTSFileNode* node=cfg->get(0); node!=NULL; node=node->next) {
		string dir= trainsetDir+dirSep;;
		string file= "";
		int id;
		int rev= 0;
		if (node->value && *(node->value)=="Engine") {
			MSTSFileNode* data= node->get("EngineData");
			dir+= data->get(1)->c_str();
			file+= data->get(0)->c_str();
			file+= ".eng";
			MSTSFileNode* uid= node->get("UiD");
			if (uid)
				id= atoi(uid->get(0)->c_str());
			MSTSFileNode* flip= node->get("Flip");
			if (flip)
				rev= atoi(flip->get(0)->c_str());
		} else if (node->value && *(node->value)=="Wagon") {
			MSTSFileNode* data= node->get("WagonData");
			dir+= data->get(1)->c_str();
			file+= data->get(0)->c_str();
			file+= ".wag";
			MSTSFileNode* uid= node->get("UiD");
			if (uid)
				id= atoi(uid->get(0)->c_str());
			MSTSFileNode* flip= node->get("Flip");
			if (flip)
				rev= atoi(flip->get(0)->c_str());
		}
		if (file == "")
			continue;
		dir= fixFilenameCase(dir);
		RailCarDef* def= findRailCarDef(file,false);
		if (def == NULL) {
			def= readMSTSWag(dir.c_str(),file.c_str());
			if (def)
				railCarDefMap[file]= def;
			else
				def= findRailCarDef(file,true);
			fprintf(stderr,"loaded %s %s\n",
			  dir.c_str(),file.c_str());
		}
		if (def == NULL) {
			fprintf(stderr,"cannot load %s %s\n",
			  dir.c_str(),file.c_str());
			continue;
		}
		RailCarInst* car= new RailCarInst(def,root,70,"K");
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
		fprintf(stderr,"empty train\n");
		delete train;
	}
	train->name= "explore";;
	trainMap[train->name]= train;
	train->setModelsOff();
	float initAux= 50;
	float initCyl= 50;
	float initEqRes= 50;
	train->connectAirHoses();
	if (train->engAirBrake != NULL)
		train->engAirBrake->setEqResPressure(initEqRes);
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next) {
		if (car->airBrake != NULL) {
			car->airBrake->setCylPressure(initCyl);
			car->airBrake->setAuxResPressure(initAux);
			car->airBrake->setEmergResPressure(70);
			car->airBrake->setPipePressure(initEqRes);
		}
	}
	train->calcPerf();
}

void MSTSRoute::loadConsist(LooseConsist* consist, osg::Group* root)
{
	string trainsDir= fixFilenameCase(mstsDir+dirSep+"TRAINS");
	string trainsetDir= fixFilenameCase(trainsDir+dirSep+"TRAINSET");
	Train* train= new Train;
	float initPipe= 0;
	float initAux= 50;
	float initCyl= 50;
	float initEqRes= 50;
	for (Wagon* w=consist->wagons; w!=NULL; w=w->next) {
		string dir= fixFilenameCase(trainsetDir+"/"+w->dir);
		string file= w->name+(w->isEngine?".eng":".wag");
		RailCarDef* def= findRailCarDef(file,false);
		if (def == NULL) {
			def= readMSTSWag(dir.c_str(),file.c_str());
			if (def)
				railCarDefMap[file]= def;
			else
				def= findRailCarDef(file,true);
		}
		if (def == NULL) {
			fprintf(stderr,"cannot load %s %s\n",
			  dir.c_str(),file.c_str());
			continue;
		}
		RailCarInst* car= new RailCarInst(def,root,70);
		car->setLoad(0);
		car->prev= train->lastCar;
		if (train->lastCar == NULL)
			train->firstCar= car;
		else
			train->lastCar->next= car;
		train->lastCar= car;
		char buf[20];
		sprintf(buf,"%d-%d",consist->id,w->id);
		string carid= buf;
		car->addWaybill(carid,1,0,0,1);
	}
	if (train->firstCar == NULL) {
		fprintf(stderr,"empty train\n");
		delete train;
		return;
	}
	trainList.push_back(train);
	Track* track= trackMap[routeID];
	track->findLocation(convX(consist->tx,consist->x),
	  convZ(consist->tz,consist->z),&train->location);
	train->location.rev= consist->direction!=0;
	float len= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next)
		len+= car->def->length;
//	train->location.move(len/2,1,0);
	train->location.edge->occupied++;
	train->endLocation= train->location;
	train->endLocation.move(-len,1,-1);
	train->connectAirHoses();
	if (train->engAirBrake != NULL) {
		train->engAirBrake->setEqResPressure(initEqRes);
		initPipe= initEqRes;
	}
	if (initCyl > initAux)
		initCyl= initAux;
	float x= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next) {
		car->setLocation(x-car->def->length/2,&train->location);
		x-= car->def->length;
		if (car->airBrake != NULL) {
			car->airBrake->setCylPressure(initCyl);
			car->airBrake->setAuxResPressure(initAux);
			car->airBrake->setPipePressure(initPipe);
			car->airBrake->setEmergResPressure(initPipe);
		}
		if (initCyl == 0)
			car->handBControl= 1;
	}
	train->calcPerf();
	WLocation loc;
	train->location.getWLocation(&loc);
	fprintf(stderr,"consist %d at %lf %lf %f\n",consist->id,
	  loc.coord[0],loc.coord[1],loc.coord[2]);
}

Track::Path* MSTSRoute::loadService(string filename, osg::Group* root,
  bool player, int serviceId)
{
	string servicesDir= fixFilenameCase(routeDir+dirSep+"SERVICES");
	string path= fixFilenameCase(servicesDir+dirSep+filename+".srv");
//	fprintf(stderr,"path=%s\n",path.c_str());
	Service serv;
	serv.readFile(path.c_str());
	fprintf(stderr," service consist %s %s\n",serv.consistName.c_str(),
	  serv.pathName.c_str());
	Track::Path* trackPath= loadPath(serv.pathName+".pat",true);
	if (trackPath == NULL) {
		fprintf(stderr,"no path\n");
		return NULL;
	}
	string trainsDir= fixFilenameCase(mstsDir+dirSep+"TRAINS");
	string trainsetDir= fixFilenameCase(trainsDir+dirSep+"TRAINSET");
	string consistsDir= fixFilenameCase(trainsDir+dirSep+"CONSISTS");
	path= fixFilenameCase(consistsDir+dirSep+serv.consistName+".con");
//	fprintf(stderr,"path=%s\n",path.c_str());
	MSTSFile conFile;
	conFile.readFile(path.c_str());
//	fprintf(stderr,"first=%p\n",conFile.getFirstNode());
	MSTSFileNode* cfg= conFile.getFirstNode()->get("TrainCfg");
//	fprintf(stderr,"cfg=%p\n",cfg);
	Train* train= new Train;
	float initAux= 50;
	float initCyl= 50;
	float initEqRes= 50;
	for (MSTSFileNode* node=cfg->get(0); node!=NULL; node=node->next) {
		string dir= trainsetDir+dirSep;;
		string file= "";
		int id;
		int rev= 0;
		if (node->value && *(node->value)=="Engine") {
			MSTSFileNode* data= node->get("EngineData");
			dir+= data->get(1)->c_str();
			file+= data->get(0)->c_str();
			file+= ".eng";
			MSTSFileNode* uid= node->get("UiD");
			if (uid)
				id= atoi(uid->get(0)->c_str());
			MSTSFileNode* flip= node->get("Flip");
			if (flip)
				rev= atoi(flip->get(0)->c_str());
		} else if (node->value && *(node->value)=="Wagon") {
			MSTSFileNode* data= node->get("WagonData");
			dir+= data->get(1)->c_str();
			file+= data->get(0)->c_str();
			file+= ".wag";
			MSTSFileNode* uid= node->get("UiD");
			if (uid)
				id= atoi(uid->get(0)->c_str());
			MSTSFileNode* flip= node->get("Flip");
			if (flip)
				rev= atoi(flip->get(0)->c_str());
		}
		if (file == "")
			continue;
		dir= fixFilenameCase(dir);
		RailCarDef* def= findRailCarDef(file,false);
		if (def == NULL) {
			def= readMSTSWag(dir.c_str(),file.c_str());
			if (def)
				railCarDefMap[file]= def;
			else
				def= findRailCarDef(file,true);
			fprintf(stderr,"loaded %s %s\n",
			  dir.c_str(),file.c_str());
		}
		if (def == NULL) {
			fprintf(stderr,"cannot load %s %s\n",
			  dir.c_str(),file.c_str());
			continue;
		}
		RailCarInst* car= new RailCarInst(def,root,70);
		car->setLoad(0);
		car->prev= train->lastCar;
		car->rev= rev;
		if (train->lastCar == NULL)
			train->firstCar= car;
		else
			train->lastCar->next= car;
		train->lastCar= car;
		char buf[20];
		sprintf(buf,"%d-%d",serviceId,id);
		string carid= buf;
		car->addWaybill(carid,1,0,0,1);
	}
	if (train->firstCar == NULL) {
		fprintf(stderr,"empty train\n");
		delete train;
		return NULL;
	}
	train->name= filename;
	trainMap[filename]= train;
	trainList.push_back(train);
	if (!player) {
		train->targetSpeed= 8.9;
		train->bControl= 1;
		train->modelCouplerSlack= 0;
	}
	Track* track= trackMap[routeID];
	train->endLocation= trackPath->firstNode->loc;
	float len= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next)
		len+= car->def->length;
	train->endLocation.edge->occupied++;
	train->location= train->endLocation;
	train->location.move(len,1,1);
	train->connectAirHoses();
	if (train->engAirBrake != NULL)
		train->engAirBrake->setEqResPressure(initEqRes);
	if (initCyl > initAux)
		initCyl= initAux;
	float x= 0;
	for (RailCarInst* car=train->firstCar; car!=NULL; car=car->next) {
		car->setLocation(x-car->def->length/2,&train->location);
		x-= car->def->length;
//		fprintf(stderr,"%f\n",x);
		if (car->airBrake != NULL) {
			car->airBrake->setCylPressure(initCyl);
			car->airBrake->setAuxResPressure(initAux);
		}
		if (initCyl == 0)
			car->handBControl= 1;
	}
	train->calcPerf();
	WLocation loc;
	train->location.getWLocation(&loc);
	fprintf(stderr,"service %s at %lf %lf %f\n",filename.c_str(),
	  loc.coord[0],loc.coord[1],loc.coord[2]);
	currentPerson.setLocation(loc.coord);
	return trackPath;
}

//	loads an MSTS path file and converts into a Path structure
Track::Path* MSTSRoute::loadPath(string filename, bool align)
{
	TrackPath trackPath;
	string pathsDir= fixFilenameCase(routeDir+dirSep+"PATHS");
	string filepath= fixFilenameCase(pathsDir+dirSep+filename);
//	fprintf(stderr,"filepath=%s\n",filepath.c_str());
	trackPath.readFile(filepath.c_str());
	vector<Track::Path::Node*> pathNodes;
	pathNodes.reserve(trackPath.nNodes);
	for (int i=0; i<trackPath.nNodes; i++)
		pathNodes[i]= new Track::Path::Node;
	bool print= false;
//	for (int i=0; i<trackPath.nPDPs; i++)
//		if ((trackPath.pdps[i].type2&0x8) != 0)
//			print= true;
	Track* track= trackMap[routeID];
	Track::Path* path= new Track::Path;
	path->firstNode= pathNodes[0];
	for (int i=0; i<trackPath.nNodes; i++) {
		Track::Path::Node* p= pathNodes[i];
		p->sw= NULL;
		p->next= NULL;
		p->nextSiding= NULL;
		TrPathNode* tpn= &trackPath.nodes[i];
		if (tpn->next != NULL)
			p->next= pathNodes[tpn->next-trackPath.nodes];
		if (tpn->nextSiding != NULL)
			p->nextSiding=
			  pathNodes[tpn->nextSiding-trackPath.nodes];
		if ((tpn->flags & 02) != 0) {
			p->type= Track::Path::STOP;
			p->value= ((tpn->flags>>16)&0xffff);
			if (p->value>=20000 && p->value<30000) {
				p->type= Track::Path::UNCOUPLE;
				p->value-= 20000;
			} else if (p->value>=30000 && p->value<40000) {
				p->type= Track::Path::COUPLE;
				p->value-= 30000;
			}
		} else if ((tpn->flags & 01) != 0) {
			p->type= Track::Path::REVERSE;
			p->value= 0;
		} else {
			p->type= Track::Path::OTHER;
			p->value= 0;
		}
		double x= convX(tpn->pdp->tx,tpn->pdp->x);
		double z= convZ(tpn->pdp->tz,tpn->pdp->z);
		double y= tpn->pdp->y;
		track->findLocation(x,z,y,&p->loc);
		if (tpn->pdp->type1 == 2)
			p->sw= track->findSwitch(x,z,y);
		if (print) {
			fprintf(stderr," %d %f %f %f %d %d %p\n",
			  i,x,z,y,p->type,p->value,p->sw);
			if (p->sw)
				fprintf(stderr,"  %f %f %f\n",
				  p->sw->location.coord[0],
				  p->sw->location.coord[1],
				  p->sw->location.coord[2]);
		}
	}
	track->orient(path);
	if (align)
		track->alignSwitches(path->firstNode,NULL,false);
	return path;
}
#endif

#if 0
void MSTSRoute::addSwitchStands(double offset, double zoffset,
  osg::Node* model, osg::Group* rootNode, double pOffset)
{
	Track* track= trackMap[routeID];
	for (Track::VertexList::iterator i=track->vertexList.begin();
	  i!=track->vertexList.end(); ++i) {
		Track::Vertex* v= *i;
		if (v->type != Track::VT_SWITCH)
			continue;
		Track::SwVertex* sw= (Track::SwVertex*) v;
		Track::Vertex* v1= sw->swEdges[0]->otherV(v);
		Track::Vertex* v2= sw->swEdges[1]->otherV(v);
#if 0
		fprintf(stderr,"switchstand %f %f %f %f %f\n",triArea(
		  v->location.coord[0],v->location.coord[1],
		  v1->location.coord[0],v1->location.coord[1],
		  v2->location.coord[0],v2->location.coord[1]),
		  sw->swEdges[0]->length,sw->swEdges[0]->curvature,
		  sw->swEdges[1]->length,sw->swEdges[1]->curvature);
#endif
		double fOffset= pOffset;
		if (sw->swEdges[0]->curvature==0 &&
		  sw->swEdges[1]->curvature==0)
			fOffset+= fmin(sw->swEdges[0]->length,
			  sw->swEdges[1]->length);
		track->addSwitchStand(sw->id,sw->mainEdge==1?offset:-offset,
		  zoffset,model,rootNode,fOffset);
	}
}

osg::Node* MSTSRoute::attachSwitchStand(Tile* tile, osg::Node* model,
  double x, double y, double z)
{
	Track::SwVertex* swVertex= NULL;
	double bestd= 1000;
	for (SwVertexMap::iterator i= tile->swVertexMap.begin();
	  i!=tile->swVertexMap.end(); i++) {
		Track::SwVertex* v= i->second;
		WLocation loc= v->location;
		double dx= loc.coord[0] - x;
		double dy= loc.coord[1] - y;
		double dz= loc.coord[2] - z;
		double d= dx*dx + dy*dy + dz*dz;
		if (bestd > d) {
			bestd= d;
			swVertex= v;
		}
	}
	if (swVertex) {
		fprintf(stderr,"attach %p %f %f %f %f\n",swVertex,bestd,x,y,z);
		osg::Node* copy= (osg::Node*)
		  model->clone(osg::CopyOp::DEEP_COPY_NODES);
		SetSwVertexVisitor visitor(swVertex);
		copy->accept(visitor);
		return copy;
	}
	return model;
}
#endif

#if 0
MSTSSignal* MSTSRoute::findSignalInfo(MSTSFileNode* node)
{
	if (!createSignals)
		return NULL;
	MSTSFileNode* subobj= node->get("SignalSubObj");
	MSTSFileNode* units= node->get("SignalUnits");
//	for (int i=0; subobj->get(i); i++) {
//		fprintf(stderr," subobj %d %s\n",
//		  i,subobj->get(i)->c_str());
//	}
	MSTSSignal* signal= new MSTSSignal;
	for (int i=0; units->get(i); i++) {
		MSTSFileNode* n= units->get(i)->get("TrItemId");
		if (n == NULL)
			continue;
		i++;
		const char* id= n->get(1)->c_str();
		SignalMap::iterator j= signalMap.find(id);
		if (j!=signalMap.end()) {
			Signal* sig= j->second;
//		TrItem* item= &trackDB.trItems[atoi(id)];
//		fprintf(stderr," id %d %s %p\n",i,id,sig);
			signal->units.push_back(sig);
		}
	}
	return signal;
}
#endif

#if 0
osg::Geometry* MSTSRoute::loadPatchGeoFile(Patch* patch, int ti, int tj,
  Tile* tile)
{
	return NULL;
	float x0= 2048*(tile->x-centerTX);
	float z0= 2048*(tile->z-centerTZ);
	char buf[50];
	sprintf(buf,"_%d_%d.obj",ti,tj);
	string path= tilesDir+dirSep+tile->tFilename+buf;
//	fprintf(stderr,"trying patchgeom %s\n",path.c_str());
	osg::Node* model= osgDB::readNodeFile(path);
	if (model == NULL)
		return NULL;
//	fprintf(stderr,"got patchgeom %s\n",path.c_str());
	osg::Group* group= dynamic_cast<osg::Group*>(model);
	if (group == NULL) {
		fprintf(stderr,"patchgeom %s not group\n",path.c_str());
		model->unref();
		return NULL;
	}
	osg::Geode* geode= dynamic_cast<osg::Geode*>(group->getChild(0));
	if (geode == NULL) {
		fprintf(stderr,"patchgeom %s no geode\n",path.c_str());
		model->unref();
		return NULL;
	}
	osg::Geometry* geom=
	  dynamic_cast<osg::Geometry*>(geode->getDrawable(0));
	if (geom == NULL) {
		fprintf(stderr,"patchgeom %s no geom\n",path.c_str());
		model->unref();
		return NULL;
	}
	geom->ref();
	model->unref();
//	fprintf(stderr,"patchgeom %s\n",path.c_str());
	osg::Vec3Array* pVerts= (osg::Vec3Array*) geom->getVertexArray();
	osg::Vec2Array* texCoords= new osg::Vec2Array;
	osg::Vec2Array* microTexCoords= new osg::Vec2Array;
	float* v= (float*) pVerts->getDataPointer();
	for (int j=0; j<pVerts->getNumElements(); j++) {
		int j3= j*3;
		float x= v[j3];
		float y= v[j3+2];
		float z= -v[j3+1];
		float vj= x/8 - tj*16 + 128;
		float vi= 128 - y/8 - ti*16;
//		if (j == 0)
//			fprintf(stderr," %f %f %f %f %f\n",
//			  v[j3],v[j3+1],v[j3+2],vj,vi);
		v[j3]= x0+x;
		v[j3+1]= z0+y;
		v[j3+2]= z;
		float u= patch->u0 + patch->dudx*vj + patch->dudz*vi;
		float v= patch->v0 + patch->dvdx*vj + patch->dvdz*vi;
		texCoords->push_back(osg::Vec2(u,v));
		microTexCoords->push_back(osg::Vec2(32*u,32*v));
	}
	geom->setTexCoordArray(0,texCoords);
	geom->setTexCoordArray(1,microTexCoords);
	return geom;
}
#endif

#if 0
void MSTSRoute::createWater(float waterLevel)
{
	Water* water= new Water;
	water->waterLevel= waterLevel;
	water->mstsRoute= this;
	waterList.push_back(water);
}
#endif

#if 0
float MSTSRoute::getWaterDepth(double x, double y)
{
	int tx= centerTX + (int)rint(x/2048);
	int tz= centerTZ + (int)rint(y/2048);
	float dx= x - 2048*(tx-centerTX);
	float dz= y - 2048*(tz-centerTZ);
	int tx1= dx>128 ? tx+1 : dx<-128 ? tx-1 : tx;
	int tz1= dz>128 ? tz+1 : dz<-128 ? tz-1 : tz;
	Tile* tile= findTile(tx,tz);
	if (tile == NULL)
		return 0;
	Tile* t12= findTile(tile->x,tile->z-1);
	Tile* t21= findTile(tile->x+1,tile->z);
	Tile* t22= findTile(tile->x+1,tile->z-1);
	float a0= getAltitude(dx,dz,tile,t12,t21,t22);
	int j= (int)floor(dx/8) + 128;
	int i= 128 - (int)floor(dz/8);
//	fprintf(stderr,"getWaterDepth %f %f %d %d %f %f\n",
//	  x,y,i,j,a0,tile->getWaterLevel(i,j));
	return tile->getWaterLevel(i,j)-a0;
}
#endif

#if 0
bool MSTSRoute::ignoreShape(string* filename, double x, double y, double z)
{
	int n= ignorePolygon.size();
	if (n > 0) {
		bool inside= true;
		for (int i=0; i<n-3 && inside; i+=2) {
			if (triArea(ignorePolygon[i],
			  ignorePolygon[i+1],ignorePolygon[i+2],
			  ignorePolygon[i+3],x,y)<0)
				inside= false;
		}
		if (inside) {
			fprintf(stderr,"ignore %s %.3f %.3f %.3f\n",
			  filename->c_str(),x,y,z);
			return true;
		}
	}
	for (multimap<string,osg::Vec3d>::iterator
	  i= ignoreShapeMap.lower_bound(*filename);
	  i!=ignoreShapeMap.end() && i->first==*filename; i++) {
		if (x-.001<i->second[0] && i->second[0]<x+.001 &&
		  y-.001<i->second[1] && i->second[1]<y+.001 &&
		  z-.001<i->second[2] && i->second[2]<z+.001)
			return true;
	}
	return false;
}
#endif

vsg::ref_ptr<vsg::Switch> MSTSRoute::createTrackLines()
{
	auto i= trackMap.find(routeID);
	if (i == trackMap.end())
		return {};
	Track* track= i->second;
	auto nv= track->vertexList.size();
	auto ne= track->edgeList.size();
//	fprintf(stderr,"nv %ld ne %ld\n",nv,ne);
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec4Array> colors(new vsg::vec4Array(nv));
	int j= 0;
	for (auto i=track->vertexList.begin(); i!=track->vertexList.end();
	  i++) {
		auto v= *i;
		verts->at(j)= v->location.coord;
		colors->at(j)= vsg::vec4(0,0,0,1);
		v->occupied= j++;
	}
	auto indices= vsg::ushortArray::create(2*ne);
	j= 0;
	for (auto i=track->edgeList.begin(); i!=track->edgeList.end(); i++) {
		auto e= *i;
		indices->set(j++,e->v1->occupied);
		indices->set(j++,e->v2->occupied);
	}
	auto attributeArrays= vsg::DataList{verts,colors};
	auto vid= vsg::VertexIndexDraw::create();
	vid->assignArrays(attributeArrays);
	vid->assignIndices(indices);
	vid->indexCount= indices->valueCount();
	vid->instanceCount= 1;
	vid->firstIndex= 0;
	vid->vertexOffset= 0;
	vid->firstInstance= 0;
	auto stateGroup= vsg::StateGroup::create();
	stateGroup->addChild(vid);
	auto shaderSet= vsg::createFlatShadedShaderSet(vsgOptions);
	auto gpConfig= vsg::GraphicsPipelineConfigurator::create(shaderSet);
	struct SetLineList : public vsg::Visitor {
		SetLineList() { }
		void apply(vsg::InputAssemblyState& ias) override {
			ias.topology= VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		}
	} setLineList;
//	gpConfig->accept(setLineList); this doesn't work for some reason
	for (auto& ps : gpConfig->pipelineStates) ps->accept(setLineList);
	gpConfig->enableArray("vsg_Vertex",VK_VERTEX_INPUT_RATE_VERTEX,12);
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
	auto sw= vsg::Switch::create();
	sw->addChild(true,stateGroup);
	trackLines= sw;
	return sw;
}

MstsRouteReader::MstsRouteReader()
{
}

bool MstsRouteReader::getFeatures(Features& features) const
{
	features.extensionFeatureMap[".tdb"]=
	  static_cast<vsg::ReaderWriter::FeatureMask>(READ_FILENAME);
	return true;
}

vsg::ref_ptr<vsg::Object> MstsRouteReader::read(
  const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
	const auto ext= vsg::lowerCaseFileExtension(filename);
	if (ext != ".tdb")
		return {};
	vsg::Path filepath= findFile(filename,options);
	if (!filepath)
		return {};
	auto route= MSTSRoute::createRoute(filepath.string());
	if (!route)
		return {};
	route->vsgOptions= vsg::Options::create();
	route->readTiles();
	route->makeTrack();
	auto group= vsg::Group::create();
	route->makeTileMap(group);
	group->addChild(route->createTrackLines());
	return group;
}
