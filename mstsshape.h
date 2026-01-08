//	class for reading MSTS shape files
//
/*
Copyright © 2025 Doug Jones

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
#ifndef MSTSSHAPE_H
#define MSTSSHAPE_H

#include <vsg/io/ReaderWriter.h>

#include "track.h"
#include "railcar.h"

struct MSTSSignal;

struct MSTSShape {
	std::string filename;
	std::string directory;
	std::string directory2;
	std::vector<int> shaders;
	struct Point {
		float x;
		float y;
		float z;
		int index;
		Point(float x, float y, float z) {
			this->x= x;
			this->y= y;
			this->z= z;
		};
	};
	std::vector<Point> points;
	struct UVPoint {
		float u;
		float v;
		int index;
		UVPoint(float u, float v) {
			this->u= u;
			this->v= v;
		};
	};
	std::vector<UVPoint> uvPoints;
	struct Normal {
		float x;
		float y;
		float z;
		int index;
		Normal(float x, float y, float z) {
			this->x= x;
			this->y= y;
			this->z= z;
		};
	};
	std::vector<Normal> normals;
	struct Matrix {
		std::string name;
		int part;
		vsg::dmat4 matrix;
		vsg::Group* group;
		vsg::MatrixTransform* transform;
		Matrix(std::string& s) {
			name= s;
			group= nullptr;
			transform= nullptr;
			part= -1;
		};
	};
	std::vector<Matrix> matrices;
	std::vector<std::string> images;
	struct Texture {
		int imageIndex;
		vsg::ref_ptr<vsg::Data> image;
		Texture(int i) {
			imageIndex= i;
		};
	};
	std::vector<Texture> textures;
	struct VTXState {
		int matrixIndex;
		int lightMaterialIndex;
		VTXState(int i, int j) {
			matrixIndex= i;
			lightMaterialIndex= j;
		};
	};
	std::vector<VTXState> vtxStates;
	struct PrimState {
		std::vector<int> texIdxs;
		int vStateIndex;
		int shaderIndex;
		int alphaTestMode;
		int zBufMode;
		PrimState(int vsi, int si) {
			vStateIndex= vsi;
			shaderIndex= si;
			alphaTestMode= 0;
			zBufMode= 1;
		};
	};
	std::vector<PrimState> primStates;
	struct Vertex {
		int pointIndex;
		int normalIndex;
		int uvIndex;
		int index;
		Vertex(int p, int n, int uv) {
			pointIndex= p;
			normalIndex= n;
			uvIndex= uv;
		};
	};
	struct VertexSet {
		int stateIndex;
		int startIndex;
		int nVertex;
		VertexSet(int state, int start, int n) {
			stateIndex= state;
			startIndex= start;
			nVertex= n;
		};
	};
	struct TriList {
		int primStateIndex;
		std::vector<int> vertexIndices;
		std::vector<int> normalIndices;
		TriList(int psi) {
			primStateIndex= psi;
		};
	};
	struct SubObject {
		std::vector<Vertex> vertices;
		std::vector<VertexSet> vertexSets;
		std::vector<TriList> triLists;
	};
	struct DistLevel {
		float dist;
		std::vector<int> hierarchy;
		std::vector<SubObject> subObjects;
		
	};
	std::vector<DistLevel> distLevels;
	struct AnimNode {
		std::string name;
		std::map<int,vsg::dvec3> positions;
		std::map<int,vsg::dquat> quats;
		AnimNode(std::string s) {
			name=s;
		};
	};
	struct Animation {
		int nFrames;
		int frameRate;
		std::vector<AnimNode> nodes;
		Animation(int n, int r) {
			nFrames= n;
			frameRate= r;
		};
	};
	std::vector<Animation> animations;
	int readBinFile(const char* filename);
	void readFile(const char* filename, const char* texDir1=NULL,
	 const char* texDir2=NULL);
	void createRailCar(RailCarDef* def);
	void makeGeometry(SubObject& subObject, TriList& triList,
	  int& transparentBin, bool incTransparentBin= false);
	vsg::ref_ptr<vsg::Node> createModel(int transform=1,
	  int transparentBin=10, bool saveNames=false,
	  bool incTransparentBin= false);
	void readACEFiles();
	void makeLOD();
	void printSubobjects();
	void fixTop();
	vsg::dvec3* signalLightOffset;
	vsg::ref_ptr<vsg::StateGroup> mtStateSet;
	float mtUVMult;
	float patchU0;
	float patchV0;
	float patchDuDx;
	float patchDvDz;
	MSTSShape() {
		signalLightOffset= NULL;
		mtStateSet= NULL;
		mtUVMult= 0;
	}
	vsg::ref_ptr<vsg::Options> vsgOptions;
};

class MstsShapeReaderWriter : public vsg::Inherit<vsg::CompositeReaderWriter,
  MstsShapeReaderWriter>
{
public:
	MstsShapeReaderWriter();
	vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename,
	  vsg::ref_ptr<const vsg::Options> options= {}) const override;
	bool getFeatures(Features& features) const override;
};

#endif
