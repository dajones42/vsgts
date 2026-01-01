//	track profile code for making 3D model for track
/*
Copyright © 2021,2026 Doug Jones

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
#include "vsg/all.h"

#include <map>
#include <vector>

using namespace std;

#include "trackshape.h"
#include "track.h"

TrackShapeMap trackShapeMap;

struct VInfo {
	int id;
	vsg::vec3 normal;
	int nEdges;
	VInfo(int i) {
		id= i;
		normal= vsg::vec3(0,0,0);
		nEdges= 0;
	};
	void addNormal(float dx, float dy);
};
typedef map<Track::Vertex*,VInfo> VInfoMap;

int sameDirection(float x1, float y1, float x2, float y2)
{
	float x= x1+x2;
	float y= y1+y2;
	float d1= x*x+y*y;
	x= x1-x2;
	y= y1-y2;
	float d2= x*x+y*y;
	return d1 > d2;
}

void VInfo::addNormal(float dx, float dy)
{
	if (nEdges==0 || sameDirection(normal[1],normal[0],dx,-dy))
		normal+= vsg::vec3(-dy,dx,0);
	else
		normal+= vsg::vec3(dy,-dx,0);
	nEdges++;
}

vsg::ref_ptr<vsg::StateGroup> Track::makeGeometry(vsg::ref_ptr<vsg::Options> vsgOptions)
{
	if (shape == NULL)
		return NULL;
	VInfoMap vInfoMap;
	int nv= 0;
	for (VertexList::iterator i=vertexList.begin(); i!=vertexList.end();
	  ++i) {
		Vertex* v= *i;
		vInfoMap.insert(make_pair(v,VInfo(nv++)));
	}
	for (EdgeList::iterator i=edgeList.begin(); i!=edgeList.end();
	  ++i) {
		Edge* e= *i;
		float dx= (e->v1->location.coord[0]-e->v2->location.coord[0]) /
		  e->length;
		float dy= (e->v1->location.coord[1]-e->v2->location.coord[1]) /
		  e->length;
		VInfoMap::iterator vi= vInfoMap.find(e->v1);
		vi->second.addNormal(dx,dy);
		vi= vInfoMap.find(e->v2);
		vi->second.addNormal(dx,dy);
	}
	int no= shape->offsets.size();
	vector<vsg::vec3> vertv;
	for (VertexList::iterator i=vertexList.begin(); i!=vertexList.end();
	  ++i) {
		Vertex* v= *i;
		VInfoMap::iterator vi= vInfoMap.find(v);
//		if (vi->second.nEdges == 0)
//			continue;
		vi->second.normal= normalize(vi->second.normal);
		if (v->occupied)
		fprintf(stderr,"v %d %d %lf %lf %lf\n",
		  vi->second.id,vi->second.nEdges,vi->second.normal[0],
		  vi->second.normal[1],vi->second.normal[2]);
		float nx= vi->second.normal[0];
		float ny= vi->second.normal[1];
		for (int j=0; j<shape->offsets.size(); j++) {
			int j1= j;
			if (v->occupied && j%2==1)
				j1--;
			vsg::dvec3 p= v->location.coord+
			  vsg::dvec3(nx*shape->offsets[j1].x,
			  ny*shape->offsets[j1].x,-shape->offsets[j1].y);
			if (v->occupied)
			fprintf(stderr,"%d %f %f %f\n",
			  vi->second.id*no+j,p[0],p[1],p[2]);
			vertv.push_back(vsg::vec3(p.x,p.y,p.z));
		}
	}
//	static bool firstTime= true;
	nv= edgeList.size()*shape->surfaces.size()*6 +
	  shape->endVerts.size()*vertexList.size()*3;
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec2Array> texCoords(new vsg::vec2Array(nv));
	vsg::ref_ptr<vsg::vec3Array> normals(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec4Array> colors(new vsg::vec4Array(nv));
	int nvi= 0;
	for (EdgeList::iterator i=edgeList.begin(); i!=edgeList.end();
	  ++i) {
		Edge* e= *i;
		float edx= (e->v1->location.coord[0]-e->v2->location.coord[0])
		  / e->length;
		float edy= (e->v1->location.coord[1]-e->v2->location.coord[1])
		  / e->length;
//		if (firstTime)
//			fprintf(stderr,"%f %f\n",edx,edy);
		VInfoMap::iterator vi1= vInfoMap.find(e->v1);
		VInfoMap::iterator vi2= vInfoMap.find(e->v2);
		int v1o= vi1->second.id*no;
		int flip1= !sameDirection(vi1->second.normal[1],
		  vi1->second.normal[0],edx,-edy);
		int v2o= vi2->second.id*no;
		int flip2= !sameDirection(vi2->second.normal[1],
		  vi2->second.normal[0],edx,-edy);
		for (int j=0; j<shape->surfaces.size(); j++) {
			int flags= shape->surfaces[j].flags;
			if (e->occupied && flags && (e->occupied&flags)==0)
				continue;
			int i1= shape->surfaces[j].vo1;
			int o1= shape->offsets[i1].other;
			int i2= shape->surfaces[j].vo2;
			int o2= shape->offsets[i2].other;
			if (flip1) {
				verts->at(nvi)= vertv[v1o+o1];
				verts->at(nvi+1)= vertv[v1o+o2];
			} else {
				verts->at(nvi)= vertv[v1o+i1];
				verts->at(nvi+1)= vertv[v1o+i2];
			}
			if (flip2) {
				verts->at(nvi+2)= vertv[v2o+o2];
				verts->at(nvi+3)= vertv[v2o+o2];
				verts->at(nvi+4)= vertv[v2o+o1];
			} else {
				verts->at(nvi+2)= vertv[v2o+i2];
				verts->at(nvi+3)= vertv[v2o+i2];
				verts->at(nvi+4)= vertv[v2o+i1];
			}
			if (flip1) {
				verts->at(nvi+5)= vertv[v1o+o1];
			} else {
				verts->at(nvi+5)= vertv[v1o+i1];
			}
			float dx= shape->offsets[i2].x-shape->offsets[i1].x;
			float dy= shape->offsets[i2].y-shape->offsets[i1].y;
			vsg::vec3 normal= vsg::cross(vsg::normalize(vsg::vec3(dx,0,-dy)),
			  vsg::normalize(vsg::vec3(edx,-edy,0)));
			for (int k=0; k<6; k++)
				normals->at(nvi+k)= normal;
//			if (firstTime)
//			fprintf(stderr,
//			  "%p %f %f %f %d %d %d %d %d %d %d %d %d %d\n",e,
//			  normal.x,normal.y,normal.z,
//			  v1o,i1,v2o,i2,
//			  v1o+i1,v1o+i2,v2o+i2,v2o+i2,v2o+i1,v1o+i1);
			if (shape->image) {
				float top= e->length/shape->surfaces[j].meters;
				float u1= shape->surfaces[j].u1;
				float u2= shape->surfaces[j].u2;
//				fprintf(stderr,"%f %f %f\n",u1,u2,top);
				if (top < 0) {
					texCoords->at(nvi)= vsg::vec2(0,u1);
					texCoords->at(nvi+1)= vsg::vec2(0,u2);
					texCoords->at(nvi+2)= vsg::vec2(top,u2);
					texCoords->at(nvi+3)= vsg::vec2(top,u2);
					texCoords->at(nvi+4)= vsg::vec2(top,u1);
					texCoords->at(nvi+5)= vsg::vec2(0,u1);
				} else {
					texCoords->at(nvi)= vsg::vec2(u1,0);
					texCoords->at(nvi+1)= vsg::vec2(u2,0);
					texCoords->at(nvi+2)= vsg::vec2(u2,top);
					texCoords->at(nvi+3)= vsg::vec2(u2,top);
					texCoords->at(nvi+4)= vsg::vec2(u1,top);
					texCoords->at(nvi+5)= vsg::vec2(u1,0);
				}
			}
			for (int k=0; k<6; k++)
				colors->at(nvi+k)= vsg::vec4(1,1,1,1);
			nvi+= 6;
		}
//		firstTime= false;
	}
	if (shape->endVerts.size() > 0) {
		int eo0= shape->endVerts[0].offset;
		float u0= shape->endVerts[0].u;
		float v0= shape->endVerts[0].v;
		for (VertexList::iterator i=vertexList.begin();
		  i!=vertexList.end(); ++i) {
			Vertex* v= *i;
			VInfoMap::iterator vi= vInfoMap.find(v);
			if (vi->second.nEdges != 1)
				continue;
			int vo= vi->second.id*no;
			int flip= vi->second.id>0;
			for (int j=2; j<shape->endVerts.size(); j++) {
				int eo1= shape->endVerts[j-1].offset;
				int eo2= shape->endVerts[j].offset;
				float u1= shape->endVerts[j-1].u;
				float v1= shape->endVerts[j-1].v;
				float u2= shape->endVerts[j].u;
				float v2= shape->endVerts[j].v;
				verts->at(nvi)= vertv[vo+eo0];
				texCoords->at(nvi)= vsg::vec2(u0,v0);
				if (flip) {
					verts->at(nvi+1)= vertv[vo+eo2];
					verts->at(nvi+2)= vertv[vo+eo1];
					texCoords->at(nvi+1)= vsg::vec2(u2,v2);
					texCoords->at(nvi+2)= vsg::vec2(u1,v1);
				} else {
					verts->at(nvi+1)= vertv[vo+eo1];
					verts->at(nvi+2)= vertv[vo+eo2];
					texCoords->at(nvi+1)= vsg::vec2(u1,v1);
					texCoords->at(nvi+2)= vsg::vec2(u2,v2);
				}
				for (int k=0; k<3; k++)
					colors->at(nvi+k)= vsg::vec4(1,1,1,1);
				nvi+= 3;
			}
		}
	}
	auto attributeArrays= vsg::DataList{verts,normals,texCoords,colors};
	auto vd= vsg::VertexDraw::create();
	vd->assignArrays(attributeArrays);
	vd->vertexCount= nv;
	vd->instanceCount= 1;
	vd->firstVertex= 0;
	vd->firstInstance= 0;
	auto stateGroup= vsg::StateGroup::create();
	stateGroup->addChild(vd);
	auto shaderSet= vsg::createPhongShaderSet(vsgOptions);;
	auto matValue= vsg::PhongMaterialValue::create();
	matValue->value().alphaMask= 0;
	matValue->value().ambient= vsg::vec4(.6,.6,.6,1);
	matValue->value().diffuse= vsg::vec4(.4,.4,.4,1);
	matValue->value().specular= vsg::vec4(0,0,0,1);
	matValue->value().shininess= 0;
	auto sampler= vsg::Sampler::create();
	sampler->addressModeU= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler->addressModeV= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vsgOptions->sharedObjects->share(sampler);
	auto gpConfig= vsg::GraphicsPipelineConfigurator::create(shaderSet);
	gpConfig->assignTexture("diffuseMap",shape->image,sampler);
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

void TrackShape::matchOffsets()
{
	for (int i=0; i<offsets.size(); i++) {
		float bestd= 1e10;
		int bestj= i;
		for (int j=0; j<offsets.size(); j++) {
			float dx= offsets[i].x+offsets[j].x;
			float dy= offsets[i].y-offsets[j].y;
			float d= dx*dx+dy*dy;
			if (bestd > d) {
				bestd= d;
				bestj= j;
			}
		}
		offsets[i].other= bestj;
	}
}
