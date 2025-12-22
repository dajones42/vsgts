//	class for managing/reading MSTS route information
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
#ifndef MSTSROUTE_H
#define MSTSROUTE_H

struct TrackShape;
struct Event;
struct MSTSSignal;
struct Track;
struct TrackDB;
struct MSTSFileNode;
struct LooseConsist;
struct MSTSSignal;

#include "track.h"

struct MSTSRoute {
	std::string mstsDir;
	std::string routeID;
	std::string dirSep;
	std::string routeDir;
	std::string fileName;
	std::string gShapesDir;
	std::string gTexturesDir;
	std::string tilesDir;
	std::string terrtexDir;
	std::string worldDir;
	std::string rShapesDir;
	std::string rTexturesDir;
	std::string activityName;
	std::string consistName;
	int centerTX;
	int centerTZ;
	float centerLat;
	float centerLong;
	double cosCenterLat;
	struct Terrain {
		unsigned short y[256][256];
		unsigned char n[256][256];
		unsigned char f[256][256];
	};
	struct Patch {
		int flags;
		short texIndex;
		float u0;
		float v0;
		float dudx;
		float dudz;
		float dvdx;
		float dvdz;
		float centerX;
		float centerZ;
	};
	typedef std::map<int,Track::SwVertex*> SwVertexMap;
	struct Tile {
		int x;
		int z;
		float floor;
		float scale;
		float swWaterLevel;
		float seWaterLevel;
		float neWaterLevel;
		float nwWaterLevel;
		std::string tFilename;
		vsg::Group* models;
		vsg::Node* terrModel;
		vsg::PagedLOD* plod;
		Terrain* terrain;
		Patch patches[256];
		SwVertexMap swVertexMap;
		std::vector<std::string> textures;
		std::vector<std::string> microTextures;
		float microTexUVMult;
		void freeTerrain();
		Tile(int tx, int tz) {	
			x= tx;
			z= tz;
			terrain= NULL;
			models= NULL;
			terrModel= NULL;
			microTexUVMult= 32;
		};
		float getWaterLevel(int i, int j);
	};
	int tileID(int tx, int tz) {
		return ((0xffff&tx)<<16) + (0xffff&tz);
	};
	struct TrackSection {
		float dist;
		float radius;
		TrackSection(float d, float r) {
			dist= d;
			radius= r;
		}
	};
	typedef std::vector<TrackSection> TrackSections;
	typedef std::map<int,Tile*> TileMap;
	typedef std::map<std::string,Tile*> TerrainTileMap;
	TileMap tileMap;
	TerrainTileMap terrainTileMap;
	typedef std::map<std::string,vsg::Node*> ModelMap;
	ModelMap trackModelMap;
	ModelMap staticModelMap;
	TrackShape* dynTrackBase;
	TrackShape* dynTrackRails;
	TrackShape* dynTrackWire;
	TrackShape* dynTrackBerm;
	TrackShape* dynTrackBridge;
	TrackShape* dynTrackTies;
	float bermHeight;
	float wireHeight;
	bool bridgeBase;
	bool srDynTrack;
	bool ustDynTrack;
	bool wireTerrain;
	std::string wireModelsDir;
	bool ignoreHiddenTerrain;
	vsg::ref_ptr<vsg::Switch> trackLines;
	typedef std::map<int,Event*> EventMap;
	EventMap eventMap;
	MSTSRoute(const char* mstsDir, const char* routeID);
	static MSTSRoute* createRoute(std::string tdbPath);
	~MSTSRoute();
	void findCenter(TrackDB* trackDB);
	double convX(int tx, float x) {
		return 2048*(double)(tx-centerTX) + x;
	}
	double convZ(int tz, float z) {
		return 2048*(double)(tz-centerTZ) + z;
	}
	void ll2xy(double lat, double lng, double* x, double *y);
	void xy2ll(double lat, double lng, double* x, double *y);
	void makeTrack();
	void addSwitchStands(double offset, double zoffset,
	  vsg::Node* model, vsg::Group* rootNode, double poffset);
	void adjustWater(int setTerrain);
	const char* tFile(int x, int z);
	void tFileToXZ(char* filename, int *xp, int * zp);
	void readTiles();
	int readTFile(const char* path, Tile* tile);
	void readTerrain(Tile* tile);
	void writeTerrain(Tile* tile);
	Tile* findTile(int tx, int tz);
	void makeTileMap(vsg::Group* root);
	void loadModels(Tile* tile);
	int readBinWFile(const char* filename, Tile* tile, float x0, float z0);
	void loadTerrainData(Tile* tile);
//	vsg::Node* loadTrackModel(std::string* filename, Track::SwVertex* sw);
	void overrideTrackModel(std::string& shapename, std::string& model);
	vsg::Node* loadStaticModel(std::string* filename, MSTSSignal* signal=NULL);
	vsg::Node* loadHazardModel(std::string* filename);
	vsg::Node* attachSwitchStand(Tile* tile, vsg::Node* model,
	  double x, double y, double z);
	void cleanStaticModelMap();
	vsg::Node* makeDynTrack(MSTSFileNode* dynTrack);
	vsg::Node* makeDynTrack(TrackSections& trackSections,bool bridge);
	vsg::Node* makeTransfer(MSTSFileNode* transfer, std::string* filename,
	  Tile* tile, MSTSFileNode* pos, MSTSFileNode* qdir);
	vsg::Node* makeTransfer(std::string* filename, Tile* tile,
	  vsg::vec3 center, vsg::quat quat, float w, float h);
	vsg::Node* makeForest(MSTSFileNode* transfer,
	  Tile* tile, MSTSFileNode* pos, MSTSFileNode* qdir);
	void makeTerrainModel(Tile* tile);
	void makeWater(Tile* tile, float dl, const char* texture,
	  int renderBin);
	bool makeDynTrackShapes();
	bool makeSRDynTrackShapes();
	bool makeUSTDynTrackShapes();
	void createWater(float waterLevel);
	void saveShoreMarkers(const char* filename);
	int drawWater;
	float waterLevelDelta;
	void makeTerrainPatches(Tile* tile);
	vsg::Geometry* makePatch(Patch* patch, int i0, int j0,
	  Tile* tile, Tile* t12, Tile* t21, Tile* t22);
	vsg::Geometry* loadPatchGeoFile(Patch* patch, int i0, int j0,
	  Tile* tile);
	float getAltitude(int i, int j, Tile* tile,
	  Tile* t12, Tile* t21, Tile* t22);
	float getAltitude(float x, float z, Tile* tile,
	  Tile* t12, Tile* t21, Tile* t22);
	int getNormalIndex(int i, int j, Tile* tile,
	  Tile* t12, Tile* t21, Tile* t22);
	vsg::vec3 getNormal(int i, int j, Tile* tile,
	  Tile* t12, Tile* t21, Tile* t22);
	bool getVertexHidden(int i, int j, Tile* tile,
	  Tile* t12, Tile* t21, Tile* t22);
	std::vector<vsg::vec3> terrainNormals;
	void loadActivity(vsg::Group* root, int activityFlags);
	void loadConsist(LooseConsist* consist, vsg::Group* root);
	void loadExploreConsist(vsg::Group* root);
//	Track::Path* loadPath(std::string filename, bool align);
//	Track::Path* loadService(std::string filename, vsg::Group* root,
//	  bool player, int id);
	bool signalSwitchStands;
	bool createSignals;
	MSTSSignal* findSignalInfo(MSTSFileNode* node);
	float getWaterDepth(double x, double y);
	std::vector<double> ignorePolygon;
	std::multimap<std::string,vsg::dvec3> ignoreShapeMap;
	bool ignoreShape(std::string* filename, double x, double y, double z);
	vsg::ref_ptr<vsg::Switch> createTrackLines();
};

#include <vsg/io/ReaderWriter.h>

class MstsRouteReaderWriter : public vsg::Inherit<vsg::CompositeReaderWriter,
  MstsRouteReaderWriter>
{
public:
	MstsRouteReaderWriter();
	vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename,
	  vsg::ref_ptr<const vsg::Options> options= {}) const override;
	bool getFeatures(Features& features) const override;
};

#endif
