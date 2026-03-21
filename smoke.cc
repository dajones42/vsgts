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
#include <iostream>
#include <vsg/all.h>

#include "mstsroute.h"
#include "mstsace.h"
#include "smoke.h"
#include "ttosim.h"

const char* smokeVert= R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants {
	mat4 projection;
	mat4 modelView;
} pc;

layout(location = 0) in vec3 vsg_Vertex;
layout(location = 1) in vec3 vsg_Normal;
layout(location = 2) in vec2 vsg_TexCoord0;
layout(location = 6) in vec4 vsg_Color;
layout(location = 7) in vec4 vsg_Translation_scaleDistance;

layout(location = 0) out vec3 eyePos;
layout(location = 1) out vec3 normalDir;
layout(location = 2) out vec4 vertexColor;
layout(location = 3) out vec3 viewDir;
layout(location = 4) out vec2 texCoord[1];

out gl_PerVertex{
	vec4 gl_Position;
};

mat4 computeBillboadMatrix(vec4 center_eye, float scale)
{
	mat4 S= mat4(scale, 0.0, 0.0, 0.0,
	              0.0, scale, 0.0, 0.0,
	              0.0, 0.0, scale, 0.0,
	              0.0, 0.0, 0.0, 1.0);
	mat4 T= mat4(1.0, 0.0, 0.0, 0.0,
	              0.0, 1.0, 0.0, 0.0,
	              0.0, 0.0, 1.0, 0.0,
	              center_eye.x, center_eye.y, center_eye.z, 1.0);
	return T*S;
}

void main()
{
	vec4 vertex= vec4(vsg_Vertex, 1.0);
	vec4 normal= vec4(vsg_Normal, 0.0);
	float alpha= vsg_Translation_scaleDistance.w;
	float scale= 1 + vsg_Translation_scaleDistance.z + 4*(1-alpha);
	mat4 mv= computeBillboadMatrix(pc.modelView * vec4(vsg_Translation_scaleDistance.xyz, 1.0), scale);
	gl_Position= (pc.projection * mv) * vertex;
	eyePos= (mv * vertex).xyz;
	viewDir= -(mv * vertex).xyz;
	normalDir= (mv * normal).xyz;
	vertexColor= vec4(vsg_Color.rgb, alpha*alpha);
	texCoord[0]= vsg_TexCoord0;
}
)";

#define VIEW_DESCRIPTOR_SET 0
#define MATERIAL_DESCRIPTOR_SET 1

vsg::ref_ptr<vsg::ShaderSet> smokeShaderSet(vsg::ref_ptr<vsg::Options> options)
{
	if (options) {
		auto i= options->shaderSets.find("smoke");
		if (i != options->shaderSets.end())
			return i->second;
	}
	auto vertexShader= vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", smokeVert);
	auto fragmentShader= vsg::read_cast<vsg::ShaderStage>("shaders/standard_phong.frag", options);
	if (!vertexShader || !fragmentShader) {
		std::cerr<<"cannot create smoke shaders "<<vertexShader.get()<<" "<<fragmentShader.get()<<
		  " "<<options->paths<<"\n";
		return {};
	}
	auto shaderSet= vsg::ShaderSet::create(vsg::ShaderStages{vertexShader, fragmentShader});

	shaderSet->addAttributeBinding("vsg_Vertex", "", 0,
	  VK_FORMAT_R32G32B32_SFLOAT, vsg::vec3Array::create(1));
	shaderSet->addAttributeBinding("vsg_Normal", "", 1,
	  VK_FORMAT_R32G32B32_SFLOAT, vsg::vec3Array::create(1));
	shaderSet->addAttributeBinding("vsg_TexCoord0", "VSG_TEXTURECOORD_0", 2,
	  VK_FORMAT_R32G32_SFLOAT, vsg::vec2Array::create(1));
	shaderSet->addAttributeBinding("vsg_TexCoord1", "VSG_TEXTURECOORD_1", 3,
	  VK_FORMAT_R32G32_SFLOAT, vsg::vec2Array::create(1));
	shaderSet->addAttributeBinding("vsg_TexCoord2", "VSG_TEXTURECOORD_2", 4,
	  VK_FORMAT_R32G32_SFLOAT, vsg::vec2Array::create(1));
	shaderSet->addAttributeBinding("vsg_TexCoord3", "VSG_TEXTURECOORD_3", 5,
	  VK_FORMAT_R32G32_SFLOAT, vsg::vec2Array::create(1));
	shaderSet->addAttributeBinding("vsg_Color", "", 6,
	  VK_FORMAT_R32G32B32A32_SFLOAT, vsg::vec4Array::create(1), vsg::CoordinateSpace::LINEAR);
	shaderSet->addAttributeBinding("vsg_Translation_scaleDistance", "VSG_BILLBOARD", 7,
	  VK_FORMAT_R32G32B32A32_SFLOAT, vsg::vec4Array::create(1));
	shaderSet->addAttributeBinding("vsg_Translation", "VSG_INSTANCE_TRANSLATION", 7,
	  VK_FORMAT_R32G32B32_SFLOAT, vsg::vec3Array::create(1));
	shaderSet->addAttributeBinding("vsg_Rotation", "VSG_INSTANCE_ROTATION", 8,
	  VK_FORMAT_R32G32B32A32_SFLOAT, vsg::quatArray::create(1));
	shaderSet->addAttributeBinding("vsg_Scale", "VSG_INSTANCE_SCALE", 9,
	  VK_FORMAT_R32G32B32_SFLOAT, vsg::vec3Array::create(1));
	shaderSet->addAttributeBinding("vsg_JointIndices", "VSG_SKINNING", 10,
	  VK_FORMAT_R32G32B32A32_SINT, vsg::ivec4Array::create(1));
	shaderSet->addAttributeBinding("vsg_JointWeights", "VSG_SKINNING", 11,
	  VK_FORMAT_R32G32B32A32_SFLOAT, vsg::vec4Array::create(1));

	shaderSet->addDescriptorBinding("diffuseMap", "VSG_DIFFUSE_MAP", MATERIAL_DESCRIPTOR_SET, 0,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::ubvec4Array2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM}));
	shaderSet->addDescriptorBinding("detailMap", "VSG_DETAIL_MAP", MATERIAL_DESCRIPTOR_SET, 1,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::ubvec4Array2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM}));
	shaderSet->addDescriptorBinding("normalMap", "VSG_NORMAL_MAP", MATERIAL_DESCRIPTOR_SET, 2,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::vec3Array2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R32G32B32_SFLOAT}),
	  vsg::CoordinateSpace::LINEAR);
	shaderSet->addDescriptorBinding("aoMap", "VSG_LIGHTMAP_MAP", MATERIAL_DESCRIPTOR_SET, 3,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::vec4Array2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R32_SFLOAT}));
	shaderSet->addDescriptorBinding("emissiveMap", "VSG_EMISSIVE_MAP", MATERIAL_DESCRIPTOR_SET, 4,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::vec4Array2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R8G8B8A8_UNORM}));
	shaderSet->addDescriptorBinding("displacementMap", "VSG_DISPLACEMENT_MAP", MATERIAL_DESCRIPTOR_SET, 7,
	  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT,
	  vsg::floatArray2D::create(1, 1, vsg::Data::Properties{VK_FORMAT_R32_SFLOAT}), vsg::CoordinateSpace::LINEAR);
	shaderSet->addDescriptorBinding("displacementMapScale", "VSG_DISPLACEMENT_MAP", MATERIAL_DESCRIPTOR_SET, 8,
	  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
	  vsg::vec3Value::create(1.0f, 1.0f, 1.0f));
	shaderSet->addDescriptorBinding("material", "", MATERIAL_DESCRIPTOR_SET, 10,
	  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::PhongMaterialValue::create(), vsg::CoordinateSpace::LINEAR);
	shaderSet->addDescriptorBinding("texCoordIndices", "", MATERIAL_DESCRIPTOR_SET, 11,
	  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::TexCoordIndicesValue::create(), vsg::CoordinateSpace::LINEAR);
	shaderSet->addDescriptorBinding("jointMatrices", "VSG_SKINNING", MATERIAL_DESCRIPTOR_SET, 12,
	  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, vsg::mat4Value::create());
	shaderSet->addDescriptorBinding("lightData", "", VIEW_DESCRIPTOR_SET, 0,
	  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::vec4Array::create(64));
	shaderSet->addDescriptorBinding("viewportData", "", VIEW_DESCRIPTOR_SET, 1,
	  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::vec4Value::create(0, 0, 1280, 1024));
	shaderSet->addDescriptorBinding("shadowMaps", "", VIEW_DESCRIPTOR_SET, 2,
	  VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
	  vsg::floatArray3D::create(1, 1, 1, vsg::Data::Properties{VK_FORMAT_R32_SFLOAT}));
	shaderSet->addDescriptorBinding("shadowMapDirectSampler", "VSG_SHADOWS_PCSS", VIEW_DESCRIPTOR_SET, 3,
	  VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
	shaderSet->addDescriptorBinding("shadowMapShadowSampler", "", VIEW_DESCRIPTOR_SET, 4,
	  VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

	shaderSet->addPushConstantRange("pc", "", VK_SHADER_STAGE_ALL, 0, 128);

	shaderSet->optionalDefines = {"VSG_GREYSCALE_DIFFUSE_MAP", "VSG_TWO_SIDED_LIGHTING", "VSG_POINT_SPRITE",
	  "VSG_SHADOWS_PCSS", "VSG_SHADOWS_SOFT", "VSG_SHADOWS_HARD", "SHADOWMAP_DEBUG", "VSG_ALPHA_TEST"};

	shaderSet->definesArrayStates.push_back(vsg::DefinesArrayState{{"VSG_INSTANCE_TRANSLATION"},
	  vsg::TranslationArrayState::create()});
	shaderSet->definesArrayStates.push_back(vsg::DefinesArrayState{{"VSG_INSTANCE_TRANSLATION",
	  "VSG_INSTANCE_ROTATION", "VSG_INSTANCE_SCALE"}, vsg::TranslationRotationScaleArrayState::create()});
	shaderSet->definesArrayStates.push_back(vsg::DefinesArrayState{{"VSG_INSTANCE_TRANSLATION",
	  "VSG_DISPLACEMENT_MAP"}, vsg::TranslationAndDisplacementMapArrayState::create()});
	shaderSet->definesArrayStates.push_back(vsg::DefinesArrayState{{"VSG_DISPLACEMENT_MAP"},
	  vsg::DisplacementMapArrayState::create()});
	shaderSet->definesArrayStates.push_back(vsg::DefinesArrayState{{"VSG_BILLBOARD"},
	  vsg::BillboardArrayState::create()});

	shaderSet->customDescriptorSetBindings.push_back(vsg::ViewDependentStateBinding::create(VIEW_DESCRIPTOR_SET));

	if (options)
		options->shaderSets["smoke"]= vsg::ref_ptr(shaderSet);
	return shaderSet;
}

SmokeModel::SmokeModel(int nParticles, float size, float exDist)
{
	particleSize= size;
	exhaustDistance= exDist;
	std::string path= mstsRoute->gTexturesDir+mstsRoute->dirSep+"smoke.ace";
	auto img= readCacheACEFile(path.c_str());
	float maxuv= 1;
	if (!img) {
		path= mstsRoute->gTexturesDir+mstsRoute->dirSep+"smokemain.ace";
		img= readCacheACEFile(path.c_str());
		maxuv= .25;
	}
	auto shaderSet= smokeShaderSet(mstsRoute->vsgOptions);;
	if (!shaderSet)
		shaderSet= vsg::createPhongShaderSet(mstsRoute->vsgOptions);
	auto matValue= vsg::PhongMaterialValue::create();
	matValue->value().alphaMask= 0;
	matValue->value().ambient= vsg::vec4(1,1,1,1);
	matValue->value().diffuse= vsg::vec4(1,1,1,1);
	matValue->value().specular= vsg::vec4(0,0,0,1);
	matValue->value().shininess= 0;
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(4));
	vsg::ref_ptr<vsg::vec2Array> texCoords(new vsg::vec2Array(4));
	vsg::ref_ptr<vsg::vec3Array> normals(new vsg::vec3Array(4));
	vsg::ref_ptr<vsg::vec4Array> colors(new vsg::vec4Array(4));
	auto indices= vsg::ushortArray::create({0,1,2,0,2,3});
	float sz= .5*particleSize;
	verts->at(0)= vsg::vec3(-sz,-sz,0);
	verts->at(1)= vsg::vec3(sz,-sz,0);
	verts->at(2)= vsg::vec3(sz,sz,0);
	verts->at(3)= vsg::vec3(-sz,sz,0);
	texCoords->at(0)= vsg::vec2(0,maxuv);
	texCoords->at(1)= vsg::vec2(maxuv,maxuv);
	texCoords->at(2)= vsg::vec2(maxuv,0);
	texCoords->at(3)= vsg::vec2(0,0);
	for (int i=0; i<4; i++) {
		normals->at(i)= vsg::vec3(0,1,0);
		colors->at(i)= vsg::vec4(1,1,1,1);
//		colors->at(i)= vsg::vec4(0,0,0,1);
	}
	positions= new vsg::vec4Array(nParticles);
	speeds= new vsg::vec4Array(nParticles);
	for (int i=0; i<nParticles; i++) {
		positions->at(i)= vsg::vec4(0,0,-1,0);
		speeds->at(i)= vsg::vec4(0,0,1,.33);
	}
	auto attributeArrays= vsg::DataList{verts,normals,texCoords,colors,positions};
	auto vid= vsg::VertexIndexDraw::create();
	vid->assignArrays(attributeArrays);
	vid->assignIndices(indices);
	vid->indexCount= 6;
	vid->instanceCount= nParticles;
	vid->firstIndex= 0;
	vid->vertexOffset= 0;
	vid->firstInstance= 0;
	auto gpConfig= vsg::GraphicsPipelineConfigurator::create(shaderSet);
	gpConfig->shaderHints->defines.insert("VSG_BILLBOARD");
	if (img) {
		auto sampler= vsg::Sampler::create();
		sampler->addressModeU= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler->addressModeV= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		mstsRoute->vsgOptions->sharedObjects->share(sampler);
		gpConfig->assignTexture("diffuseMap",img,sampler);
	}
	matValue->value().alphaMask= 1;
	matValue->value().alphaMaskCutoff= .2;
	gpConfig->shaderHints->defines.insert("VSG_ALPHA_TEST");
	gpConfig->assignDescriptor("material",matValue);
	gpConfig->enableArray("vsg_Vertex",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_Normal",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_TexCoord0",VK_VERTEX_INPUT_RATE_VERTEX,8);
	gpConfig->enableArray("vsg_Color",VK_VERTEX_INPUT_RATE_VERTEX,16);
	gpConfig->enableArray("vsg_Translation_scaleDistance",VK_VERTEX_INPUT_RATE_INSTANCE,16);
	auto colorBlendState= vsg::ColorBlendState::create();
	colorBlendState->configureAttachments(true);
//	for (auto& a: colorBlendState->attachments) {
//		a.dstColorBlendFactor= VK_BLEND_FACTOR_ONE;
//		a.colorBlendOp= VK_BLEND_OP_MAX;
//		a.alphaBlendOp= VK_BLEND_OP_MAX;
//		a.colorBlendOp= VK_BLEND_OP_MIN;
//		a.alphaBlendOp= VK_BLEND_OP_MIN;
//	}
	gpConfig->pipelineStates.push_back(colorBlendState);
	if (mstsRoute->vsgOptions->sharedObjects)
		mstsRoute->vsgOptions->sharedObjects->share(gpConfig,
		  [](auto gpc) { gpc->init(); });
	else
		gpConfig->init();
	vsg::StateCommands commands;
	gpConfig->copyTo(commands,mstsRoute->vsgOptions->sharedObjects);
	auto stateGroup= vsg::StateGroup::create();
	stateGroup->addChild(vid);
	stateGroup->stateCommands.swap(commands);
	stateGroup->prototypeArrayState= gpConfig->getSuitableArrayState();
	binNumber= 20;
	bound.set(0,0,0,1);
	child= stateGroup;
	positions->properties.dataVariance= vsg::DYNAMIC_DATA;
}

void SmokeModel::update(float dt, float dx, float dy, float smokeSpeed)
{
	if (dt==0)
		return;
	if (dx) {
		for (int i=0; i<positions->size(); i++)
			if (positions->at(i).w > 0)
				positions->at(i).x-= dx;
	}
	float dw= dt/3;
	auto sz= .1*particleSize;
	for (int i=0; i<positions->size(); i++) {
		float x= positions->at(i).w;
		float dz= dt*x*x*x*speeds->at(i).w;
		positions->at(i).z+= dz;
		positions->at(i).w-= dw;
		positions->at(i).x+= dt*speeds->at(i).x;
		positions->at(i).y+= dt*speeds->at(i).y;
		if (positions->at(i).w < 0) {
			positions->at(i)= vsg::vec4(0,0,-1,0);
		}
	}
	if (smokeSpeed > avgSmokeSpeed)
		avgSmokeSpeed= smokeSpeed;
	else
		avgSmokeSpeed= (1-dt)*avgSmokeSpeed + dt*smokeSpeed;
	if (smokeSpeed < .5)
		distance= -1;
	else if (distance < 0)
		distance= exhaustDistance;
	else
		distance+= fabs(dx);
	if (exhaustDistance && distance>=exhaustDistance) {
		distance= 0;
		auto n= positions->size();
		for (int i=0; i<n-1; i++) {
			positions->at(i)= positions->at(i+1);
			speeds->at(i)= speeds->at(i+1);
		}
		positions->at(n-1)= vsg::vec4(0,0,0,1);
		speeds->at(n-1).x= .2*(.5-drand48());
		speeds->at(n-1).y= .2*(.5-drand48());
		speeds->at(n-1).w= 1+8*smokeSpeed;
	}
	positions->dirty();
}
