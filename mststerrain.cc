//	code for creating MSTS terrain models
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

//	makes 3D models for each patch in a tile
void MSTSRoute::makeTerrainPatches(Tile* tile)
{
	if (tile->terrModel)
		return;
//	scoped_lock lock {loadMutex};
	readTerrain(tile);
//	fprintf(stderr,"makeTerrain %d %d %f %f\n",
//	  tile->x,tile->z,tile->floor,tile->scale);
	Tile* t12= findTile(tile->x,tile->z-1);
	if (t12 != NULL)
		readTerrain(t12);
	Tile* t21= findTile(tile->x+1,tile->z);
	if (t21 != NULL)
		readTerrain(t21);
	Tile* t22= findTile(tile->x+1,tile->z-1);
	if (t22 != NULL)
		readTerrain(t22);
	std::vector<vsg::ref_ptr<vsg::Data>> textures;
	for (int i=0; i<tile->textures.size()/2; i++) {
		std::string path= terrtexDir+dirSep+tile->textures[i];
		auto img= readCacheACEFile(path.c_str());
		textures.push_back(img);
	}
#if 0
	vector<osg::Texture2D*> microTextures;
	for (int i=0; i<tile->microTextures.size(); i++) {
		osg::Texture2D* t= new osg::Texture2D;
		t->ref();
		t->setDataVariance(osg::Object::DYNAMIC);
		microTextures.push_back(t);
		std::string path= terrtexDir+dirSep+tile->microTextures[i];
		osg::Image* image= readMSTSACE(path.c_str());
		if (image != NULL)
			t->setImage(image);
		t->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::REPEAT);
		t->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::REPEAT);
	}
#endif
#if 0
	osg::TexEnvCombine* tec= new osg::TexEnvCombine();
	tec->setCombine_RGB(osg::TexEnvCombine::MODULATE);
	tec->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
	tec->setSource1_RGB(osg::TexEnvCombine::TEXTURE1);
	tec->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
	tec->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
	tec->setScale_RGB(2.);
	tec->ref();
#endif
	auto shaderSet= vsg::createPhongShaderSet(vsgOptions);;
	auto matValue= vsg::PhongMaterialValue::create();
	matValue->value().alphaMask= 0;
	matValue->value().ambient= vsg::vec4(1,1,1,1);
	matValue->value().diffuse= vsg::vec4(.5,.5,.5,1);
	matValue->value().specular= vsg::vec4(0,0,0,1);
	matValue->value().shininess= 0;
	auto sampler= vsg::Sampler::create();
	sampler->addressModeU= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler->addressModeV= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	vsgOptions->sharedObjects->share(sampler);
	Patch* patch= tile->patches;
	auto group= vsg::Group::create();
	tile->terrModel= group;
	for (int i=0; i<16; i++) {
		for (int j=0; j<16; j++) {
			if ((patch->flags&1)!=0) {
				patch++;
				continue;
			}
			auto stateGroup=
			  makePatch(patch,i*16,j*16,tile,t12,t21,t22);
			auto gpConfig=
			  vsg::GraphicsPipelineConfigurator::create(shaderSet);
			gpConfig->assignTexture("diffuseMap",
			  textures[patch->texIndex],sampler);
			gpConfig->assignDescriptor("material",matValue);
			gpConfig->enableArray("vsg_Vertex",
			  VK_VERTEX_INPUT_RATE_VERTEX,12);
			gpConfig->enableArray("vsg_Normal",
			  VK_VERTEX_INPUT_RATE_VERTEX,12);
			gpConfig->enableArray("vsg_TexCoord0",
			  VK_VERTEX_INPUT_RATE_VERTEX,8);
			gpConfig->enableArray("vsg_Color",
			  VK_VERTEX_INPUT_RATE_INSTANCE,16);
			if (vsgOptions->sharedObjects)
				vsgOptions->sharedObjects->share(gpConfig,
				  [](auto gpc) { gpc->init(); });
			else
				gpConfig->init();
			vsg::StateCommands commands;
			gpConfig->copyTo(commands,vsgOptions->sharedObjects);
			stateGroup->stateCommands.swap(commands);
			stateGroup->prototypeArrayState=
			   gpConfig->getSuitableArrayState();
#if 0
			if (patch->texIndex<microTextures.size() &&
			  microTextures[patch->texIndex]) {
				stateSet->setTextureAttributeAndModes(1,
				  microTextures[patch->texIndex],
				  osg::StateAttribute::ON);
				stateSet->setTextureAttributeAndModes(1,tec,
				  osg::StateAttribute::ON);
			}
#endif
			group->addChild(stateGroup);
			patch++;
		}
	}
	vsg::ComputeBounds computeBounds;
	group->accept(computeBounds);
	vsg::dvec3 center=
	  (computeBounds.bounds.min+computeBounds.bounds.max)*0.5;
	double radius= vsg::length(computeBounds.bounds.max-
	  computeBounds.bounds.min)*0.6;
	auto lod= vsg::PagedLOD::create();
	lod->options= vsgOptions;
	lod->bound.set(center.x,center.y,center.z,radius);
	lod->filename= tile->tFilename+".world";
	lod->children[0]= vsg::PagedLOD::Child{.8,{}};
	lod->children[1]= vsg::PagedLOD::Child{1,{}};
	group->addChild(lod);
//	auto cg= vsg::CullGroup::create();
//	cg->bound.set(center.x,center.y,center.z,radius);
//	cg->addChild(group);
//	tile->terrModel= cg;
//	for (int i=0; i<microTextures.size(); i++)
//		microTextures[i]->unref();
//	tec->unref();
}

//	makes a 3D model for a single patch
vsg::ref_ptr<vsg::StateGroup> MSTSRoute::makePatch(Patch* patch, int i0, int j0,
  Tile* tile, Tile* t12, Tile* t21, Tile* t22)
{
	float x0= 2048*(tile->x-centerTX);
	float z0= 2048*(tile->z-centerTZ);
	float uvmult= tile->microTexUVMult;
	int nv= 17*17;
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec2Array> texCoords(new vsg::vec2Array(nv));
	vsg::ref_ptr<vsg::vec3Array> normals(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec4Array> colors= vsg::vec4Array::create({vsg::vec4(1,1,1,1)});
	auto indices= vsg::ushortArray::create(6*16*16);
	int ii= 0;
	for (int i=0; i<=16; i++) {
		int k= i*17;
		for (int j=0; j<=16; j++) {
			float a= getAltitude(i+i0,j+j0,tile,t12,t21,t22);
			int vi= k+j;
			verts->at(vi)=
			  vsg::vec3(x0+8*(j0+j-128),z0+8*(128-i-i0),a);
			float u= patch->u0+patch->dudx*j+patch->dudz*i;
			float v= patch->v0+patch->dvdx*j+patch->dvdz*i;
			texCoords->at(vi)= vsg::vec2(u,v);
//			microTexCoords->push_back(
//			  osg::Vec2(uvmult*u,uvmult*v));
			normals->at(vi)= getNormal(i+i0,j+j0,tile,t12,t21,t22);
			float h00= getVertexHidden(i+i0,j+j0,tile,t12,t21,t22);
			if (i<16 && j<16) {
				float a11= getAltitude(i+i0+1,j+j0+1,
				  tile,t12,t21,t22);
				float a01= getAltitude(i+i0,j+j0+1,
				  tile,t12,t21,t22);
				float a10= getAltitude(i+i0+1,j+j0,
				  tile,t12,t21,t22);
				float h11= getVertexHidden(
				  i+i0+1,j+j0+1,tile,t12,t21,t22);
				float h01= getVertexHidden(
				  i+i0,j+j0+1,tile,t12,t21,t22);
				float h10= getVertexHidden(
				  i+i0+1,j+j0,tile,t12,t21,t22);
				int kj= k+j;
				if (fabs(a11-a) < fabs(a10-a01)) {
					if (!h00 && !h10 && !h11) {
						indices->set(ii++,kj);
						indices->set(ii++,kj+17);
						indices->set(ii++,kj+18);
					}
					if (!h00 && !h01 && !h11) {
						indices->set(ii++,kj+18);
						indices->set(ii++,kj+1);
						indices->set(ii++,kj);
					}
				} else {
					if (!h00 && !h10 && !h01) {
						indices->set(ii++,kj);
						indices->set(ii++,kj+17);
						indices->set(ii++,kj+1);
					}
					if (!h11 && !h01 && !h10) {
						indices->set(ii++,kj+1);
						indices->set(ii++,kj+17);
						indices->set(ii++,kj+18);
					}
				}
			}
		}
	}
	auto attributeArrays= vsg::DataList{verts,normals,texCoords,colors};
	auto vid= vsg::VertexIndexDraw::create();
	vid->assignArrays(attributeArrays);
	vid->assignIndices(indices);
	vid->indexCount= ii;
	vid->instanceCount= 1;
	vid->firstIndex= 0;
	vid->vertexOffset= 0;
	vid->firstInstance= 0;
	auto stateGroup= vsg::StateGroup::create();
	stateGroup->addChild(vid);
	return stateGroup;
}

MstsTerrainReader::MstsTerrainReader()
{
}

bool MstsTerrainReader::getFeatures(Features& features) const
{
	features.extensionFeatureMap[".raw"]=
	  static_cast<vsg::ReaderWriter::FeatureMask>(READ_FILENAME);
	return true;
}

vsg::ref_ptr<vsg::Object> MstsTerrainReader::read(
  const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
	if (!mstsRoute)
		return {};
	const auto ext= vsg::lowerCaseFileExtension(filename);
	if (ext != ".raw")
		return {};
	auto i= mstsRoute->terrainTileMap.find(
	  filename.string().substr(0,9).c_str());
	if (i == mstsRoute->terrainTileMap.end())
		return {};
	auto tile= i->second;
	if (!tile->terrModel)
		mstsRoute->makeTerrainPatches(tile);
	return tile->terrModel;
}
