//	code to read MSTS shape files and convert them to OSG
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
#include "mstsfile.h"
#include "mstsbfile.h"
#include "mstsace.h"
#include "mstsshape.h"

#include <vsg/all.h>

//	reads an uncompressed shape file
void MSTSShape::readFile(const char* filename, const char* texDir1,
  const char* texDir2)
{
//	fprintf(stderr,"readshape %s\n",filename);
	this->filename= filename;
	if (texDir1 == NULL) {
		char* p= strrchr((char*)filename,'/');
		if (p != NULL)
			directory= string(filename,p-filename+1);
	} else {
		directory= string(texDir1);
		if (texDir2 != NULL)
			directory2= string(texDir2);
	}
	if (readBinFile(filename))
		return;
	MSTSFile file;
	file.readFile(filename);
	MSTSFileNode* shape= file.find("shape");
	if (shape == NULL)
		throw "not a MSTS shape file?";
	MSTSFileNode* node= shape->children->find("shader_names");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("named_shader");
		  p!=NULL; p=p->find("named_shader")) {
			const char* s= p->getChild(0)->value->c_str();
			if (strcmp(s,"TexDiff")==0 || strcmp(s,"Tex")==0) {
				shaders.push_back(0);
			} else if (strcmp(s,"BlendATexDiff")==0 ||
			  strcmp(s,"BlendATex")==0) {
				shaders.push_back(1);
			} else {
				shaders.push_back(0);
//				fprintf(stderr,"named_shader %s\n",s);
			}
		}
	}
	node= shape->children->find("points");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("point");
		  p!=NULL; p=p->find("point")) {
			points.push_back(Point(
			  atof(p->getChild(0)->value->c_str()),
			  atof(p->getChild(1)->value->c_str()),
			  atof(p->getChild(2)->value->c_str())));
		}
	}
	node= shape->children->find("uv_points");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("uv_point");
		  p!=NULL; p=p->find("uv_point")) {
			uvPoints.push_back(UVPoint(
			  atof(p->getChild(0)->value->c_str()),
			  atof(p->getChild(1)->value->c_str())));
		}
	}
	node= shape->children->find("normals");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("vector");
		  p!=NULL; p=p->find("vector")) {
			normals.push_back(Normal(
			  atof(p->getChild(0)->value->c_str()),
			  atof(p->getChild(1)->value->c_str()),
			  atof(p->getChild(2)->value->c_str())));
		}
	}
	node= shape->children->find("matrices");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("matrix");
		  p!=NULL; p=p->find("matrix")) {
			int i= matrices.size();
			matrices.push_back(Matrix(*(p->value)));
#if 1
			matrices[i].matrix= vsg::dmat4(
			  atof(p->next->getChild(0)->value->c_str()),
			  atof(p->next->getChild(1)->value->c_str()),
			  atof(p->next->getChild(2)->value->c_str()),
			  0,
			  atof(p->next->getChild(3)->value->c_str()),
			  atof(p->next->getChild(4)->value->c_str()),
			  atof(p->next->getChild(5)->value->c_str()),
			  0,
			  atof(p->next->getChild(6)->value->c_str()),
			  atof(p->next->getChild(7)->value->c_str()),
			  atof(p->next->getChild(8)->value->c_str()),
			  0,
			  atof(p->next->getChild(9)->value->c_str()),
			  atof(p->next->getChild(10)->value->c_str()),
			  atof(p->next->getChild(11)->value->c_str()),
			  1);
#else
			matrices[i].matrix= vsg::dmat4(
			  atof(p->next->getChild(0)->value->c_str()),
			  atof(p->next->getChild(3)->value->c_str()),
			  atof(p->next->getChild(6)->value->c_str()),
			  atof(p->next->getChild(9)->value->c_str()),
			  atof(p->next->getChild(1)->value->c_str()),
			  atof(p->next->getChild(4)->value->c_str()),
			  atof(p->next->getChild(7)->value->c_str()),
			  atof(p->next->getChild(10)->value->c_str()),
			  atof(p->next->getChild(2)->value->c_str()),
			  atof(p->next->getChild(5)->value->c_str()),
			  atof(p->next->getChild(8)->value->c_str()),
			  atof(p->next->getChild(11)->value->c_str()),
			  0,0,0,1);
#endif
		}
	}
	node= shape->children->find("images");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("image");
		  p!=NULL; p=p->find("image")) {
			images.push_back(*(p->getChild(0)->value));
		}
	}
	node= shape->children->find("textures");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("texture");
		  p!=NULL; p=p->find("texture")) {
			textures.push_back(
			  Texture(atoi(p->getChild(0)->value->c_str())));
		}
	}
	node= shape->children->find("vtx_states");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("vtx_state");
		  p!=NULL; p=p->find("vtx_state")) {
			vtxStates.push_back(
			  VTXState(atoi(p->getChild(1)->value->c_str()),
			    atoi(p->getChild(2)->value->c_str())));
		}
	}
	node= shape->children->find("prim_states");
	if (node != NULL) {
		for (MSTSFileNode* p= node->children->find("prim_state");
		  p!=NULL; p=p->find("prim_state")) {
			if (p->children == NULL)
				p= p->next;
			int i= primStates.size();
			primStates.push_back(PrimState(
			  atoi(p->getChild(5)->value->c_str()),
			  atoi(p->getChild(1)->value->c_str())));
			MSTSFileNode* p1= p->children->find("tex_idxs");
			int n= atoi(p1->getChild(0)->value->c_str());
			if (n >= 1)
				primStates[i].texIdxs.push_back(
				  atoi(p1->getChild(1)->value->c_str()));
		}
	}
	node= shape->children->find("lod_controls");
	for (MSTSFileNode* lod= node->children->find("lod_control");
	  lod!=NULL; lod=lod->find("lod_control")) {
	for (MSTSFileNode* dls= lod->children->find("distance_levels");
	  dls!=NULL; dls=dls->find("distance_levels")) {
	for (MSTSFileNode* dl= dls->children->find("distance_level");
	  dl!=NULL; dl=dl->find("distance_level")) {
		int i= distLevels.size();
		distLevels.push_back(DistLevel());
		MSTSFileNode* dlh= dl->children->find("distance_level_header");
		MSTSFileNode* p= dlh->children->find("dlevel_selection");
		distLevels[i].dist= atof(p->getChild(0)->value->c_str());
		p= dlh->children->find("hierarchy");
		int n= atoi(p->getChild(0)->value->c_str());
		for (int j=1; j<=n; j++)
			distLevels[i].hierarchy.push_back(
			  atoi(p->getChild(j)->value->c_str()));
		MSTSFileNode* subs= dl->children->find("sub_objects");
		for (MSTSFileNode* so= subs->children->find("sub_object");
		  so!=NULL; so=so->find("sub_object")) {
			int j= distLevels[i].subObjects.size();
			distLevels[i].subObjects.push_back(SubObject());
			node= so->children->find("vertices");
			if (node == NULL)
				continue;
			for (MSTSFileNode* p= node->children->find("vertex");
			  p!=NULL; p=p->find("vertex")) {
				MSTSFileNode* p1=
				  p->children->find("vertex_uvs");
				int n= p1==NULL?0:
				  atoi(p1->getChild(0)->value->c_str());
				distLevels[i].subObjects[j].vertices.push_back(
				  Vertex(
				    atoi(p->getChild(1)->value->c_str()),
				    atoi(p->getChild(2)->value->c_str()),
				    n==0?0:
				    atoi(p1->getChild(1)->value->c_str())));
			}
			node= so->children->find("vertex_sets");
			for (MSTSFileNode* p=
			  node->children->find("vertex_set");
			  p!=NULL; p=p->find("vertex_set")) {
				distLevels[i].subObjects[j].vertexSets.
				  push_back(VertexSet(
				    atoi(p->getChild(0)->value->c_str()),
				    atoi(p->getChild(1)->value->c_str()),
				    atoi(p->getChild(2)->value->c_str())));
			}
			node= so->children->find("primitives");
			int primStateIndex= -1;
			for (MSTSFileNode* p=node->children; p!=NULL;
			  p=p->next) {
				if (p->value!=NULL &&
				  *(p->value)=="prim_state_idx") {
					primStateIndex= atoi(p->next->
					  getChild(0)->value->c_str());
					continue;
				}
				if (p->value==NULL ||
				  *(p->value)!="indexed_trilist")
					continue;
				int k= distLevels[i].subObjects[j].
				  triLists.size();
				distLevels[i].subObjects[j].triLists.push_back(
				  TriList(primStateIndex));
				MSTSFileNode* p1=
				  p->next->children->find("vertex_idxs");
				int n= atoi(p1->getChild(0)->value->c_str());
				for (int j1=1; j1<=n; j1++)
					distLevels[i].subObjects[j].triLists[k].
					  vertexIndices.push_back(atoi(
					   p1->getChild(j1)->value->c_str()));
				p1= p->next->children->find("normal_idxs");
				n= atoi(p1->getChild(0)->value->c_str());
				for (int j1=1; j1<=n; j1++)
					distLevels[i].subObjects[j].triLists[k].
					  normalIndices.push_back(atoi(
					   p1->getChild(j1)->value->c_str()));
			}
		}
	}
	}
	}	
//	fprintf(stderr,"animations\n");
	node= shape->children->find("animations");
	if (node != NULL) {
	 for (MSTSFileNode* a= node->children->find("animation");
	   a!=NULL; a=a->find("animation")) {
	 	int i= animations.size();
	 	animations.push_back(Animation(
	 	  atoi(a->getChild(0)->value->c_str()),
	 	  atof(a->getChild(1)->value->c_str())));
	 	for (MSTSFileNode* anodes= a->children->find("anim_nodes");
	 	  anodes!=NULL; anodes=anodes->find("anim_nodes")) {
		 	for (MSTSFileNode* an=
			  anodes->children->find("anim_node");
		 	  an!=NULL; an=an->find("anim_node")) {
				int j= animations[i].nodes.size();
				animations[i].nodes.push_back(
				  AnimNode(*(an->value)));
			 	for (MSTSFileNode* c=
				  an->next->children->find("controllers");
			 	  c!=NULL; c=c->find("controllers")) {
			 	 for (MSTSFileNode* r=
				   c->children->find("tcb_rot");
			 	   r!=NULL; r=r->find("tcb_rot")) {
			 	  for (MSTSFileNode* k=
				    r->children->find("tcb_key");
			 	    k!=NULL; k=k->find("tcb_key")) {
					int frame=
					  atoi(k->getChild(0)->value->c_str());
					if (frame>=animations[i].nFrames &&
					  animations[i].nodes[j].quats.size()==
					  animations[i].nFrames)
						continue;
					animations[i].nodes[j].quats[frame]=
					  vsg::dquat(
					  -atof(k->getChild(1)->value->c_str()),
					  -atof(k->getChild(2)->value->c_str()),
					  -atof(k->getChild(3)->value->c_str()),
					  atof(k->getChild(4)->value->c_str()));
				  }
			 	  for (MSTSFileNode* k=
				    r->children->find("slerp_rot");
			 	    k!=NULL; k=k->find("slerp_rot")) {
					int frame=
					  atoi(k->getChild(0)->value->c_str());
					if (frame>=animations[i].nFrames &&
					  animations[i].nodes[j].quats.size()==
					  animations[i].nFrames)
						continue;
					animations[i].nodes[j].quats[frame]=
					  vsg::dquat(
					  -atof(k->getChild(1)->value->c_str()),
					  -atof(k->getChild(2)->value->c_str()),
					  -atof(k->getChild(3)->value->c_str()),
					  atof(k->getChild(4)->value->c_str()));
				  }
				 }
			 	 for (MSTSFileNode* p=
				   c->children->find("linear_pos");
			 	   p!=NULL; p=p->find("linear_pos")) {
			 	  for (MSTSFileNode* k=
				    p->children->find("linear_key");
			 	    k!=NULL; k=k->find("linear_key")) {
					int frame=
					  atoi(k->getChild(0)->value->c_str());
					if (frame>=animations[i].nFrames &&
					  animations[i].nodes[j].positions.
					  size()==animations[i].nFrames)
						continue;
					animations[i].nodes[j].
					  positions[frame]= vsg::dvec3(
					  atof(k->getChild(1)->value->c_str()),
					  atof(k->getChild(2)->value->c_str()),
					  atof(k->getChild(3)->value->c_str()));
				  }
				 }
				}
			}
	 	}
	 }
	}
}

//	reads a binary shape file
int MSTSShape::readBinFile(const char* filename)
{
	MSTSBFile reader;
	if (reader.open(filename))
		return 0;
//	fprintf(stderr,"%s is binary %d\n",filename,reader.compressed);
	Byte buf[16];
	reader.getBytes(buf,16);
	int dli= 0;
	int soi= 0;
	int psi= 0;
	for (;;) {
		int code= reader.getInt();
		int len= reader.getInt();
		if (code == 0)
			break;
//		fprintf(stderr,"%d %d\n",code,len);
		switch (code) {
		 case 70: // shape_header
		 case 68: // volumes
		 case 74: // texture_filter_names
		 case 76: // sort_vectors
		 case 11: // colours
		 case 18: // light_materials
		 case 79: // light_model_cfgs
		 case 33: // distance_levels_header
		 case 40: // sub_object_header
		 case 52: // vertex_sets
		 case 6: // normal_idxs
		 case 64: // flags
			reader.getBytes(NULL,len);
			break;
		 case 72: // shader_names
		 case 7: // points
		 case 9: // uv_points
		 case 5: // normals
		 case 66: // matrices
		 case 14: // images
		 case 16: // textures
		 case 47: // vtx_states
		 case 55: // prim_states
		 case 31: // lod_controls
		 case 36: // distance_levels
		 case 38: // sub_objects
		 case 50: // vertices
		 case 53: // primitives
		 case 27: // anim_nodes
			reader.getString();
			reader.getInt();
			break;
		 case 71: // shape
		 case 32: // lod_control
		 case 34: // distance_level_header
		 case 60: // indexed_trilist
			reader.getString();
			break;
		 case 129: // named_shader
			reader.getString();
			{
				int n= reader.getShort();
				const char* s= reader.getString(n).c_str();
				if (strcmp(s,"TexDiff")==0 ||
				  strcmp(s,"Tex")==0) {
					shaders.push_back(0);
				} else if (strcmp(s,"BlendATexDiff")==0 ||
				  strcmp(s,"BlendATex")==0) {
					shaders.push_back(1);
				} else {
					shaders.push_back(0);
//					fprintf(stderr,"named_shader %s\n",s);
				}
			}
			break;
		 case 2: // point
			reader.getString();
			{
				float x= reader.getFloat();
				float y= reader.getFloat();
				float z= reader.getFloat();
				points.push_back(Point(x,y,z));
			}
			break;
		 case 8: // uv_point
			reader.getString();
			{
				float u= reader.getFloat();
				float v= reader.getFloat();
				uvPoints.push_back(UVPoint(u,v));
			}
			break;
		 case 3: // vector
			reader.getString();
			{
				float x= reader.getFloat();
				float y= reader.getFloat();
				float z= reader.getFloat();
				normals.push_back(Normal(x,y,z));
			}
			break;
		 case 65: // matrix
			{
				string name= reader.getString();
				float m1= reader.getFloat();
				float m2= reader.getFloat();
				float m3= reader.getFloat();
				float m4= reader.getFloat();
				float m5= reader.getFloat();
				float m6= reader.getFloat();
				float m7= reader.getFloat();
				float m8= reader.getFloat();
				float m9= reader.getFloat();
				float m10= reader.getFloat();
				float m11= reader.getFloat();
				float m12= reader.getFloat();
				int i= matrices.size();
				matrices.push_back(Matrix(name));
				matrices[i].matrix= vsg::dmat4(m1,m2,m3,0,
				  m4,m5,m6,0,m7,m8,m9,0,m10,m11,m12,1);
//				matrices[i].matrix= vsg::dmat4(m1,m4,m7,m10,
//				  m2,m5,m8,m11,m3,m6,m9,m12,0,0,0,1);
//				fprintf(stderr,"matrix %s %f %f %f"
//				  " %f %f %f %f %f %f %f %f %f\n",name.c_str(),
//				m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12);
			}
			break;
		 case 13: // image
			{
				reader.getString();
				int n= reader.getShort();
				string s= reader.getString(n);
				images.push_back(s);
			}
			break;
		 case 15: // texture
			reader.getString();
			textures.push_back(Texture(reader.getInt()));
			reader.getInt();
			reader.getFloat();
			if (len > 13)
				reader.getInt();
			break;
		 case 46: // vtx_state
			{
				int read= reader.read;
				reader.getString();
				reader.getInt();
				int matIdx= reader.getInt();
				int lightMatIdx= reader.getInt();
				vtxStates.push_back(
				  VTXState(matIdx,lightMatIdx));
				if (reader.read < read+len)
					reader.getBytes(NULL,
					  read+len-reader.read);
			}
			break;
		 case 54: // prim_state
			{ 
				int read= reader.read;
				int i= primStates.size();
				primStates.push_back(PrimState(0,0));
				reader.getString();
				reader.getInt();
				primStates[i].shaderIndex= reader.getInt();
				reader.getInt(); // tex_idxs
				int len1= reader.getInt();
				reader.getString();
				int n= reader.getInt();
//				fprintf(stderr,"tex_idxs %d %d\n",len1,n);
				for (int j=0; j<n; j++)
					primStates[i].texIdxs.push_back(
					  reader.getInt());
				reader.getInt();
				primStates[i].vStateIndex= reader.getInt();
				if (reader.read < read+len) {
					n= read+len-reader.read;
					if (n >= 4) {
						primStates[i].alphaTestMode=
						  reader.getInt();
						n-= 4;
					}
					if (n >= 4) {
						reader.getInt();
						n-= 4;
					}
					if (n >= 4) {
						primStates[i].zBufMode=
						  reader.getInt();
						n-= 4;
					}
					if (n > 0)
						reader.getBytes(NULL,n);
				}
			}
			break;
		 case 37: // distance_level
			reader.getString();
			dli= distLevels.size();
			distLevels.push_back(DistLevel());
			break;
		 case 35: // dlevel_selection
			reader.getString();
			distLevels[dli].dist= reader.getFloat();
			break;
		 case 67: // heirarchy
			{
				reader.getString();
				int n= reader.getInt();
				for (int j=1; j<=n; j++)
					distLevels[dli].hierarchy.push_back(
					  reader.getInt());
			}
			break;
		 case 39: // sub_object
			reader.getString();
			soi= distLevels[dli].subObjects.size();
			distLevels[dli].subObjects.push_back(SubObject());
			break;
		 case 48: // vertex
			{
				int read= reader.read;
				reader.getString();
				reader.getInt();
				int pi= reader.getInt();
				int ni= reader.getInt();
				reader.getInt();
				reader.getInt();
				reader.getInt(); // vertex_uvs
				int uvlen= reader.getInt();
				reader.getString();
				int nuv= reader.getInt();
				int uvi= 0;
				for (int k=0; k<nuv; k++)
					uvi= reader.getInt();
				distLevels[dli].subObjects[soi].vertices.
				  push_back(Vertex(pi,ni,uvi));
				if (reader.read < read+len)
					reader.getBytes(NULL,
					  read+len-reader.read);
			}
			break;
		 case 56: // prim_state_index
			reader.getString();
			psi= reader.getInt();
			break;
		 case 63: // vertex_idxs
			{
				reader.getString();
				int n= reader.getInt();
				int k= distLevels[dli].subObjects[soi].
				  triLists.size();
				distLevels[dli].subObjects[soi].triLists.
				  push_back(TriList(psi));
				for (int j=0; j<n; j++)
					distLevels[dli].subObjects[soi].
					  triLists[k].vertexIndices.
					  push_back(reader.getInt());
			}
			break;
		 case 29: // animations
			reader.getString();
			reader.getInt();
			break;
		 case 28: // animation
			{
				reader.getString();
				int n= reader.getInt();
				int r= reader.getInt();
				animations.push_back(Animation(n,r));
			}
			break;
		 case 26: // anim_node
			{
				string name= reader.getString();
				int i= animations.size()-1;
				animations[i].nodes.push_back(AnimNode(name));
			}
			break;
		 case 25: // controllers
			reader.getString();
			reader.getInt();
			break;
		 case 21: // linear_pos
			reader.getString();
			reader.getInt();
			//fprintf(stderr,"linear_pos\n");
			break;
		 case 22: // tcb_pos
			//fprintf(stderr,"tcb_pos %d %d\n",code,len);
			reader.getBytes(NULL,len);
			break;
		 case 23: // slerp_rot
			{
				reader.getString();
				int frame= reader.getInt();
				float x= reader.getFloat();
				float y= reader.getFloat();
				float z= reader.getFloat();
				float w= reader.getFloat();
				//fprintf(stderr,
				//  "%d %f %f %f %f\n",
				//  frame,x,y,z,w);
				int i= animations.size()-1;
				int j= animations[i].nodes.size()-1;
				if (frame<animations[i].nFrames ||
				  animations[i].nodes[j].quats.size()<
				  animations[i].nFrames)
					animations[i].nodes[j].quats[frame]=
					  vsg::dquat(-x,-y,-z,w);
			}
			break;
		 case 24: // tcb_rot
			reader.getString();
			reader.getInt();
			//fprintf(stderr,"tcb_rot\n");
			break;
		 case 19: // linear_key
			{
				reader.getString();
				int frame= reader.getInt();
				float x= reader.getFloat();
				float y= reader.getFloat();
				float z= reader.getFloat();
				//fprintf(stderr,"%d %f %f %f\n",frame,x,y,z);
				int i= animations.size()-1;
				int j= animations[i].nodes.size()-1;
				if (frame<animations[i].nFrames ||
				  animations[i].nodes[j].positions.size()<
				  animations[i].nFrames)
				animations[i].nodes[j].positions[frame]=
				  vsg::dvec3(x,y,z);
			}
			break;
		 case 20: // tcb_key
			{
				reader.getString();
				int frame= reader.getInt();
				float x= reader.getFloat();
				float y= reader.getFloat();
				float z= reader.getFloat();
				float w= reader.getFloat();
				float tension= reader.getFloat();
				float continuity= reader.getFloat();
				float bias= reader.getFloat();
				float in= reader.getFloat();
				float out= reader.getFloat();
				//fprintf(stderr,
				//  "%d %f %f %f %f\n",
				//  frame,x,y,z,w);
				int i= animations.size()-1;
				int j= animations[i].nodes.size()-1;
				if (frame<animations[i].nFrames ||
				  animations[i].nodes[j].quats.size()<
				  animations[i].nFrames)
					animations[i].nodes[j].quats[frame]=
					  vsg::dquat(-x,-y,-z,w);
			}
			break;
		 case 1: // comment
			fprintf(stderr,"%d %d\n",code,len);
			for (int i=0; i<len; i++) {
				Byte b;
				reader.getBytes(&b,1);
				fprintf(stderr," %d",0xff&b);
				if (i%16==15)
					fprintf(stderr,"\n");
			}
			fprintf(stderr,"\n");
			break;
		 default:
			fprintf(stderr,"%d %d\n",code,len);
			reader.getBytes(NULL,len);
			break;
		}
	}
	return 1;
}

//	makes OSG geometry for triangle list
void MSTSShape::makeGeometry(SubObject& subObject, TriList& triList,
  int& transparentBin, bool incTransparentBin)
{
	for (int i=0; i<subObject.vertices.size(); i++)
		subObject.vertices[i].index= -1;
	int nv= 0;
	int npt= 0;
	int nuv= 0;
	int nn= 0;
	for (int i=0; i<triList.vertexIndices.size(); i++) {
		int j= triList.vertexIndices[i];
		if (subObject.vertices[j].index < 0)
			subObject.vertices[j].index= nv++;
	}
	vsg::ref_ptr<vsg::vec3Array> verts(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec2Array> texCoords(new vsg::vec2Array(nv));
	vsg::ref_ptr<vsg::vec3Array> norms(new vsg::vec3Array(nv));
	vsg::ref_ptr<vsg::vec4Array> colors(new vsg::vec4Array(nv));
#if 0
	osg::Vec2Array* mtTexCoords= NULL;
	if (mtStateSet) {
		PrimState* ps= &primStates[triList.primStateIndex];
		if (ps->texIdxs.size() > 0) {
			int ti= ps->texIdxs[0];
			int imageIndex= textures[ti].imageIndex;
			if (images[imageIndex]=="fieldmt.ace" ||
			  images[imageIndex]=="field20mt.ace")
				mtTexCoords= new osg::Vec2Array;
		}
	}
#endif
	nv= 0;
	for (int i=0; i<subObject.vertices.size(); i++) {
		if (subObject.vertices[i].index < 0)
			continue;
		subObject.vertices[i].index= nv++;
		int vi= subObject.vertices[i].index;
		int j= subObject.vertices[i].pointIndex;
		if (j >= points.size())
			fprintf(stderr,"pointIndex too big\n");
		verts->at(vi)= vsg::vec3(points[j].x,points[j].y,points[j].z);
#if 0
		if (mtTexCoords) {
			float u= patchU0 + patchDuDx*points[j].x;
			float v= patchV0 + patchDvDz*points[j].z;
			texCoords->push_back(osg::Vec2(u,v));
			mtTexCoords->push_back(
			  osg::Vec2(mtUVMult*u,mtUVMult*v));
		} else {
#endif
			j= subObject.vertices[i].uvIndex;
			if (j<0 || j >= uvPoints.size())
//				fprintf(stderr,"uvIndex too big\n");
				texCoords->at(vi)= vsg::vec2(0,0);
			else
				texCoords->at(vi)=
				  vsg::vec2(uvPoints[j].u,uvPoints[j].v);
//		}
		j= subObject.vertices[i].normalIndex;
		if (j >= normals.size())
			fprintf(stderr,"normalIndex too big\n");
		norms->at(vi)=
		  vsg::vec3(normals[j].x,normals[j].y,normals[j].z);
		colors->at(vi)= vsg::vec4(1,1,1,1);
	}
	auto indices= vsg::ushortArray::create(triList.vertexIndices.size());
	for (int i=0; i<triList.vertexIndices.size(); i++) {
		int i3= i%3;
		int i1= i3==0 ? i+2 : i3==2 ? i-2 : i;
		int j= triList.vertexIndices[i1];
//		drawElements->push_back(subObject.vertices[j].index);
		indices->set(i,subObject.vertices[j].index);
	}
	auto attributeArrays= vsg::DataList{verts,norms,texCoords,colors};
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
	PrimState* ps= &primStates[triList.primStateIndex];
	int vsi= ps->vStateIndex;
	int mi= vtxStates[vsi].matrixIndex;
	if (matrices[mi].group == nullptr)
		matrices[mi].group= new vsg::Group();
//	fprintf(stderr,"vsi %d\n",ps->vStateIndex);
//	fprintf(stderr,"lmi %d\n",vtxStates[ps->vStateIndex].lightMaterialIndex);
	bool useFlatShader= false;
	auto matValue= vsg::PhongMaterialValue::create();
	matValue->value().ambient= vsg::vec4(1,1,1,1);// multiplied by diffuse in shader
	matValue->value().diffuse= vsg::vec4(.5,.5,.5,1);
	matValue->value().specular= vsg::vec4(0,0,0,1);
	matValue->value().shininess= 0;
	switch (vtxStates[ps->vStateIndex].lightMaterialIndex) {
	 case -5://normal
		break;
	 case -1: //maybe supershine used in Wayne Campbell autos
	 case -6: //spec25, strong highlight, roughness .3 in blender
		matValue->value().specular= vsg::vec4(.1,.1,.1,1);
		matValue->value().shininess= 4;
		break;
	 case -7: //spec750, small highlight, roughness .1 in blender
		matValue->value().specular= vsg::vec4(.1,.1,.1,1);
		matValue->value().shininess= 8;
		break;
	 case -8: // full bright
	 case -9: // cruciform
		useFlatShader= true;
		break;
	 case -11: // half bright
		matValue->value().diffuse= vsg::vec4(.375,.375,.375,1);
		useFlatShader= true;
		break;
	 case -12: // dark bright
		matValue->value().diffuse= vsg::vec4(.25,.25,.25,1);
		useFlatShader= true;
		break;
	 case -10: // emissive
		matValue->value().emissive= vsg::vec4(1,1,1,1);
		break;
	 default:
//		fprintf(stderr,"unknown lmi %d %s\n",
//		  vtxStates[ps->vStateIndex].lightMaterialIndex,
//		  filename.c_str());
		break;
	}
	auto shaderSet= useFlatShader ? vsg::createFlatShadedShaderSet(vsgOptions) :
	  vsg::createPhongShaderSet(vsgOptions);
	matValue->value().alphaMask= 0;
	auto gpConfig= vsg::GraphicsPipelineConfigurator::create(shaderSet);
	auto& defines= gpConfig->shaderHints->defines;
	if ((ps->alphaTestMode || shaders[ps->shaderIndex]==1)
	  && transparentBin!=0) {
		int& tbin= transparentBin;
		if (incTransparentBin &&
		  ps->zBufMode==3 && ps->alphaTestMode==0)
			tbin++;
		else if (incTransparentBin && ps->zBufMode==1)
			tbin+= 2;
		if (ps->alphaTestMode) {
			matValue->value().alphaMask= 1;
			matValue->value().alphaMaskCutoff= .6;
			defines.insert("VSG_ALPHA_TEST");
		} else {
			vsg::ColorBlendState::ColorBlendAttachments cbas;
			VkPipelineColorBlendAttachmentState cba= {};
			cba.colorWriteMask= VK_COLOR_COMPONENT_R_BIT |
			  VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			  VK_COLOR_COMPONENT_A_BIT;
			cbas.push_back(cba);
			auto colorBlendState=
			  vsg::ColorBlendState::create(cbas);
			colorBlendState->configureAttachments(true);
			gpConfig->pipelineStates.push_back(colorBlendState);
		}
		vsg::ComputeBounds computeBounds;
		stateGroup->accept(computeBounds);
		vsg::dvec3 center=
		  (computeBounds.bounds.min+computeBounds.bounds.max)*0.5;
		double radius= vsg::length(computeBounds.bounds.max-
		  computeBounds.bounds.min)*0.6;
		auto depthSorted= vsg::DepthSorted::create();
		depthSorted->binNumber= tbin;
		depthSorted->bound.set(center.x,center.y,center.z,radius);
		depthSorted->child= stateGroup;
		matrices[mi].group->addChild(depthSorted);
	} else {
		matrices[mi].group->addChild(stateGroup);
	}
#if 0
	if (mtTexCoords) {
		geometry->setTexCoordArray(1,mtTexCoords);
		osg::StateSet* ss= new osg::StateSet(*mtStateSet,
		  osg::CopyOp::SHALLOW_COPY);
		ss->setAttribute(new osg::FrontFace(
		  osg::FrontFace::CLOCKWISE));
		geometry->setStateSet(ss);
		return;
	}
#endif
	if (ps->texIdxs.size()>0) {
		int ti= ps->texIdxs[0];
		if (textures[ti].image) {
			auto sampler= vsg::Sampler::create();
			sampler->addressModeU=
			  VK_SAMPLER_ADDRESS_MODE_REPEAT;
			sampler->addressModeV=
			  VK_SAMPLER_ADDRESS_MODE_REPEAT;
			gpConfig->assignTexture("diffuseMap",
			  textures[ti].image,sampler);
			if (vsgOptions)
				vsgOptions->sharedObjects->share(sampler);
		}
	}
	gpConfig->assignDescriptor("material",matValue);
	gpConfig->enableArray("vsg_Vertex",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_Normal",VK_VERTEX_INPUT_RATE_VERTEX,12);
	gpConfig->enableArray("vsg_TexCoord0",VK_VERTEX_INPUT_RATE_VERTEX,8);
	gpConfig->enableArray("vsg_Color",VK_VERTEX_INPUT_RATE_VERTEX,16);
//	defines.insert("VSG_TWO_SIDED_LIGHTING");
	if (vsgOptions)
		vsgOptions->sharedObjects->share(gpConfig,
		  [](auto gpc) { gpc->init(); });
	else
		gpConfig->init();
	vsg::StateCommands commands;
	if (vsgOptions)
		gpConfig->copyTo(commands,vsgOptions->sharedObjects);
	else
		gpConfig->copyTo(commands);
	stateGroup->stateCommands.swap(commands);
	stateGroup->prototypeArrayState= gpConfig->getSuitableArrayState();
}

//	reads all the ACE files needs for the shape
void MSTSShape::readACEFiles()
{
	for (int i=0; i<textures.size(); i++) {
		string filename= directory+images[textures[i].imageIndex];
		auto image= readCacheACEFile(filename.c_str(),
		  directory2.size()==0);
		if (!image && directory2.size()>0) {
			filename= directory2+images[textures[i].imageIndex];
			image= readCacheACEFile(filename.c_str());
		}
		textures[i].image= image;
	}
}

//	creates an VSG subgraph for the shape file
vsg::ref_ptr<vsg::Node> MSTSShape::createModel(
  int transform, int transparentBin,
  bool saveNames, bool incTransparentBin)
{
	readACEFiles();
	bool hasWheels= false;
	for (int j=0; j<matrices.size(); j++) {
		if (strncasecmp(matrices[j].name.c_str(),"WHEELS",6) == 0)
			hasWheels= true;
	}
	for (int i=0; i<distLevels.size(); i++) {
		DistLevel& dl= distLevels[i];
	}
	for (int i=0; i<distLevels.size(); i++) {
		DistLevel& dl= distLevels[i];
		if (i > 0)
			continue;
		for (int j=0; j<matrices.size(); j++) {
			matrices[j].group= nullptr;
			matrices[j].transform= nullptr;
		}
		for (int j=0; j<dl.subObjects.size(); j++) {
			SubObject& so= dl.subObjects[j];
			for (int k=0; k<so.triLists.size(); k++) {
				TriList& tl= so.triLists[k];
				makeGeometry(so,tl,transparentBin,
				  incTransparentBin);
			}
		}
#if 0
		for (int j=0; j<matrices.size(); j++) {
			vsg::mat4 m= matrices[j].matrix;
			fprintf(stderr,"matrix %d %s:\n",
			  j,matrices[j].name.c_str());
			fprintf(stderr," group %p\n",matrices[j].group);
		}
#endif
		for (int j=0; j<matrices.size(); j++) {
			vsg::MatrixTransform* mt= new vsg::MatrixTransform();
			mt->matrix= matrices[j].matrix;
			if (matrices[j].group)
				mt->addChild(vsg::ref_ptr(matrices[j].group));
			matrices[j].transform= mt;
		}
	}
	int nSigHead= 0;
	if (signalLightOffset) {
		for (int k=0; k<matrices.size(); k++) {
			Matrix& m= matrices[k];
			if (strncasecmp(m.name.c_str(),"head",4) == 0)
				nSigHead++;
		}
	}
	vsg::ref_ptr<vsg::Animation> anim;
	for (int i=0; i<animations.size(); i++) {
		Animation& a= animations[i];
		if (a.nFrames < 2)
			continue;
		anim= vsg::Animation::create();
#if 0
		fprintf(stderr,"animnodes %d %d\n",
		  a.nodes.size(),matrices.size());
		for (int k=0; k<matrices.size(); k++) {
			Matrix& m= matrices[k];
			fprintf(stderr,"mat %s %s\n",m.name.c_str(),
			  a.nodes[k].name.c_str());
		}
#endif
		for (int j=0; j<a.nodes.size(); j++) {
			AnimNode& n= a.nodes[j];
			if (n.positions.size()<2 && n.quats.size()<2)
				continue;
			if (strncasecmp(n.name.c_str(),"Door",4)==0 ||
			  strncasecmp(n.name.c_str(),"Mirror",6)==0)
				continue;
#if 0
			fprintf(stderr,"anim %s %d %d\n",
			  n.name.c_str(),a.nFrames,a.frameRate);
			for (map<int,vsg::dvec3>::iterator
			  k=n.positions.begin(); k!=n.positions.end(); ++k)
				fprintf(stderr," pos %d %f %f %f\n",k->first,
				  k->second[0],k->second[1],k->second[2]);
			for (map<int,vsg::dquat>::iterator k=n.quats.begin();
			  k!=n.quats.end(); ++k)
				fprintf(stderr," quat %d %f %f %f %f\n",
				  k->first,k->second[0],k->second[1],
				  k->second[2],k->second[3]);
#endif
			vsg::MatrixTransform* mt= NULL;
			if (j < matrices.size()) {
				Matrix& m= matrices[j];
				if (m.transform!=NULL && m.name==n.name) {
					mt= m.transform;
					m.hasAnimation= true;
				}
			}
			if (mt == NULL) {
				for (int k=0; k<matrices.size(); k++) {
					Matrix& m= matrices[k];
					if (m.transform==NULL || m.name!=n.name)
						continue;
					mt= m.transform;
					matrices[k].hasAnimation= true;
					break;
				}
			}
			if (mt==NULL && j < matrices.size()) {
				Matrix& m= matrices[j];
//				fprintf(stderr," anim j %s %s\n",
//				  m.name.c_str(),n.name.c_str());
				if (m.transform!=NULL) {
					mt= m.transform;
					m.hasAnimation= true;
				}
			}
			if (mt == NULL)
				continue;
			auto keyframes= vsg::TransformKeyframes::create();
			if (n.quats.size() < n.positions.size()) {
				for (map<int,vsg::dvec3>::iterator
				  k=n.positions.begin(); k!=n.positions.end();
				  ++k)
					keyframes->positions.push_back(vsg::time_dvec3{(double)k->first,k->second});
			} else if(n.positions.size() < n.quats.size()) {
				for (map<int,vsg::dquat>::iterator
				  k=n.quats.begin(); k!=n.quats.end(); ++k)
					keyframes->rotations.push_back(vsg::time_dquat{(double)k->first,k->second});
			} else {
				for (map<int,vsg::dquat>::iterator
				  k=n.quats.begin(); k!=n.quats.end(); ++k) {
					map<int,vsg::dvec3>::iterator k1=
					  n.positions.find(k->first);
					if (k1 != n.positions.end())
						keyframes->add((double)k->first,k1->second,k->second);
				}
			}
			auto sampler= vsg::TransformSampler::create();
			vsg::decompose(mt->matrix,sampler->position,sampler->rotation,sampler->scale);
			sampler->object= mt;
			sampler->keyframes= keyframes;
			sampler->name= n.name;
			if (hasWheels && (a.nFrames==16 ||
			  strncasecmp(n.name.c_str(),"WHEELS",6) == 0 ||
			  strncasecmp(n.name.c_str(),"ROD",3) == 0)) {
				if (keyframes->positions.size()>0 &&
				  keyframes->positions[keyframes->positions.size()-1].time<a.nFrames)
					keyframes->positions.push_back(
					  vsg::time_dvec3{(double)a.nFrames,n.positions.begin()->second});
				if (keyframes->rotations.size()>0 &&
				  keyframes->rotations[keyframes->rotations.size()-1].time<a.nFrames)
					keyframes->rotations.push_back(
					  vsg::time_dquat{(double)a.nFrames,n.quats.begin()->second});
				for (auto& p: keyframes->positions)
					p.time/= a.nFrames;
				for (auto& r: keyframes->rotations)
					r.time/= a.nFrames;
				if (!rodAnimation)
					rodAnimation= vsg::Animation::create();
				rodAnimation->samplers.push_back(sampler);
			} else {
				anim->samplers.push_back(sampler);
			}
#if 0
			if (strncasecmp(n.name.c_str(),
			  "Pantograph",10) == 0) {
				int len= n.name.length();
				TwoStateAnimPathCB* apc=
				  new TwoStateAnimPathCB(NULL,ap,atoi(
				   n.name.substr(len>16?16:(len>13?13:10)).
				   c_str()));
				mt->setUpdateCallback(apc);
			} else if (strncasecmp(n.name.c_str(),"Door",4)==0 ||
			  strncasecmp(n.name.c_str(),"Mirror",6)==0) {
				;//ignore animation
			} else if (hasWheels && (a.nFrames==16 ||
			  strncasecmp(n.name.c_str(),"WHEELS",6) == 0 ||
			  strncasecmp(n.name.c_str(),"ROD",3) == 0)) {
				RodAnimPathCB* apc=
				  new RodAnimPathCB(NULL,ap,a.nFrames);
				mt->setUpdateCallback(apc);
			} else if (strncasecmp(n.name.c_str(),"coupler",
			  7) == 0) {
				CouplerAnimPathCB* apc=
				  new CouplerAnimPathCB(NULL,ap,a.frameRate>0);
				mt->setUpdateCallback(apc);
			} else if (strncasecmp(n.name.c_str(),"head",4) == 0) {
				int unit= n.name[4]-'1';
//				fprintf(stderr,"signalanim %s %d\n",
//				  n.name.c_str(),n);
				SignalAnimPathCB* apc=
				  new SignalAnimPathCB(NULL,ap,unit);
				mt->setUpdateCallback(apc);
			} else {
//				fprintf(stderr,"twostateanim %s\n",
//				  n.name.c_str());
				TwoStateAnimPathCB* apc=
				  new TwoStateAnimPathCB(NULL,ap,-1);
				mt->setUpdateCallback(apc);
			}
#endif
		}
	}
	vsg::Group* top= nullptr;
	for (int i=0; i<1 && i<distLevels.size(); i++) {
		DistLevel& dl= distLevels[i];
		for (int j=0; j<dl.hierarchy.size(); j++) {
			int parent= dl.hierarchy[j];
			vsg::MatrixTransform* mt= matrices[j].transform;
			if (mt == nullptr)
				continue;
			if (parent < 0)
				top= mt;
			else
				matrices[parent].transform->addChild(
				  vsg::ref_ptr(mt));
#if 0
			osg::Vec3d trans=
			  matrices[j].transform->getMatrix().getTrans();
			fprintf(stderr,"h %d %s %d %lf %lf %lf %d %p\n",
			  j,matrices[j].name.c_str(),dl.hierarchy[j],
			  trans[0],trans[1],trans[2],
			  matrices[j].part,
			  mt->getUpdateCallback());
#endif
		}
	}
	if (top == nullptr) {
		fprintf(stderr,"no top for model %ld %ld\n",matrices.size(),
		  distLevels.size());
		for (int i=0; i<distLevels.size(); i++) {
			DistLevel& dl= distLevels[i];
			if (i > 0)
				continue;
			for (int j=0; j<dl.hierarchy.size(); j++) {
				vsg::dmat4 m= matrices[j].transform->matrix;
				fprintf(stderr,"h %d %s %d %lf %lf %lf\n",
				  j,matrices[j].name.c_str(),dl.hierarchy[j],
				  m[0][3],m[1][3],m[2][3]);
			}
		}
		return {};
	}
#if 0
	if (signalLightOffset) {
		osgSim::LightPointNode* node= new osgSim::LightPointNode;
		node->setUpdateCallback(new SignalLightUCB(NULL));
		for (int i=0; i<nSigHead; i++) {
			osgSim::LightPoint lp;
			node->addLightPoint(lp);
		}
		for (int k=0; k<matrices.size(); k++) {
			Matrix& m= matrices[k];
			if (strncasecmp(m.name.c_str(),"head",4) == 0) {
				int unit= m.name[4]-'1';
				int state= 1;
				if (unit!=0 && nSigHead==3)
					state= 0;
				osgSim::LightPoint& lp=
				  node->getLightPoint(unit);
				lp._on= true;
				osg::Vec3d trans=
				  m.transform->getMatrix().getTrans();
//				fprintf(stderr,"unit %d %f %f %f\n",
//				  unit,trans[0],trans[1],trans[2]);
				lp._position= trans+*signalLightOffset;
				if (state)
					lp._color= osg::Vec4d(0,1,0,1);
				else
					lp._color= osg::Vec4d(1,0,0,1);
				lp._radius= fabs((*signalLightOffset)[2]);
			}
		}
		top->addChild(node);
	}
#endif
#if 0
	if (anim && anim->samplers.size()>0) {
		vsg::AnimationGroup* agroup= new vsg::AnimationGroup;
		agroup->animations.push_back(anim);
		agroup->addChild(vsg::ref_ptr(top));
		top= agroup;
	}
#endif
	if (transform) {
		vsg::MatrixTransform* mt= new vsg::MatrixTransform;
		mt->matrix= vsg::dmat4(0,-1,0,0, 0,0,1,0, 1,0,0,0, 0,0,0,1);
		mt->addChild(vsg::ref_ptr(top));
		top= mt;
	}
	return vsg::ref_ptr(top);
}

void MSTSShape::printSubobjects()
{
	for (int i=0; i<distLevels.size(); i++) {
		DistLevel& dl= distLevels[i];
		printf("dl %d %f\n",i,dl.dist);
		for (int j=0; j<dl.subObjects.size(); j++) {
			SubObject& so= dl.subObjects[j];
			for (int k=0; k<so.triLists.size(); k++) {
				TriList& tl= so.triLists[k];
				PrimState* ps= &primStates[tl.primStateIndex];
				int vsi=
				  primStates[tl.primStateIndex].vStateIndex;
				int mi= vtxStates[vsi].matrixIndex;
				printf(" %d.%d.%d %s %ld",i,j,k,
				  matrices[mi].name.c_str(),
				  tl.vertexIndices.size());
				if (ps->texIdxs.size()>0) {
					int ti= ps->texIdxs[0];
					printf(" %s %d",images[textures[ti].
					  imageIndex].c_str(),
					  shaders[ps->shaderIndex]);
				}
				printf("\n");
			}
		}
	}
}

void MSTSShape::fixTop()
{
	if (matrices.size() == 1) {
		vsg::dmat4& m= matrices[0].matrix;
		if (m[0][3]!=0 || m[1][3]!=0 || m[2][3]!=0) {
//			fprintf(stderr,"nonzero top %f %f %f\n",
//			  m[0][3],m[1][3],m[2][3]);
			m[0][3]= 0;
			m[1][3]= 0;
			m[2][3]= 0;
		}
	}
}

//	devides the shape file parts into railcar parts
void MSTSShape::createRailCar(RailCarDef* car)
{
	DistLevel& dl= distLevels[0];
	car->parts.clear();
	map<int,int> partMap;
	set<int> wheelParents;
	for (int j=0; j<matrices.size(); j++) {
		if (strncasecmp(matrices[j].name.c_str(),"WHEELS",6) == 0) {
			int id= atoi(matrices[j].name.c_str()+6);
			if (partMap.find(id) == partMap.end())
				partMap[id]= j;
			else
				fprintf(stderr,"duplicate wheel %s %s\n",
				  car->name.c_str(),matrices[j].name.c_str());
			int parent= dl.hierarchy[j];
			if (parent >= 0)
				wheelParents.insert(parent);
		}
	}
	for (map<int,int>::iterator i=partMap.begin(); i!=partMap.end(); ++i) {
		matrices[i->second].part= car->parts.size();
		car->parts.push_back(RailCarPart(-1,0,0));
	}
	car->axles= partMap.size();
//	fprintf(stderr,"axles=%d\n",car->axles);
	partMap.clear();
	for (int j=0; j<matrices.size(); j++)
		if (matrices[j].name.size()==6 &&
		  strncasecmp(matrices[j].name.c_str(),"BOGIE",5)==0 &&
		  wheelParents.find(j)!=wheelParents.end())
			partMap[atoi(matrices[j].name.c_str()+5)]= j;
	if (partMap.size()<2 && car->axles<3)
		partMap.clear();
	for (map<int,int>::iterator i=partMap.begin(); i!=partMap.end(); ++i) {
		matrices[i->second].part= car->parts.size();
		car->parts.push_back(RailCarPart(-1,0,0));
	}
	for (int j=0; j<dl.hierarchy.size(); j++) {
		int parent= dl.hierarchy[j];
		if (parent < 0)
			continue;
		if (matrices[j].part>=0 && matrices[parent].part<0) {
			const double* m1= matrices[parent].matrix.data();
			const double* m2= matrices[j].matrix.data();
			int p= matrices[j].part;
			car->parts[p].xoffset= m1[14]+m2[14];
			car->parts[p].zoffset= m1[13]+m2[13];//-.05;
			car->parts[p].parent= car->parts.size();
//			fprintf(stderr,"p1 %d %s %d %d %s %f %f\n",
//			  j,matrices[j].name.c_str(),p,
//			  parent,matrices[parent].name.c_str(),
//			  car->parts[p].xoffset,car->parts[p].zoffset);
			while (parent>=0) {
				parent= dl.hierarchy[parent];
				if (parent < 0)
					break;
				m1= matrices[parent].matrix.data();
				car->parts[p].xoffset+= m1[14];
				car->parts[p].zoffset+= m1[13];
//				fprintf(stderr," pp1 %f %f\n",
//				  car->parts[p].xoffset,car->parts[p].zoffset);
			}
		}
	}
	if (car->axles == 1) {
		car->parts.push_back(RailCarPart(-1,0,0));
		car->parts[1].xoffset= -car->parts[0].xoffset;
		car->parts[1].zoffset= car->parts[0].zoffset;
		car->parts[1].parent= 2;
		car->parts[0].parent= 2;
		car->axles= 2;
	}
	for (int j=0; j<dl.hierarchy.size(); j++) {
		int parent= dl.hierarchy[j];
		if (parent < 0)
			continue;
		if (matrices[j].part>=0 && matrices[parent].part>=0) {
			const double* m= matrices[j].matrix.data();
			int pp= matrices[parent].part;
			int p= matrices[j].part;
			car->parts[p].xoffset= m[14]+car->parts[pp].xoffset;
			car->parts[p].zoffset= m[13]+car->parts[pp].zoffset;
			car->parts[p].parent= pp;
//			fprintf(stderr,"p2 %d %s %d %d %s %d %f %f\n",
//			  j,matrices[j].name.c_str(),p,
//			  parent,matrices[parent].name.c_str(),pp,
//			  car->parts[p].xoffset,car->parts[p].zoffset);
		}
	}
	int i= car->parts.size();
	car->parts.push_back(RailCarPart(-1,0,0));
	car->parts[i].model= createModel(1,11,false,true);
	if (rodAnimation)
		car->rodAnimation= rodAnimation;
#if 0
	if (car->headlights.size() > 0) {
		osgSim::LightPointNode* node= new osgSim::LightPointNode;
		for (list<HeadLight>::iterator j=car->headlights.begin();
		  j!=car->headlights.end(); j++) {
			osgSim::LightPoint lp;
			lp._on= false;
			lp._position= osg::Vec3d(j->x,j->y,
			  j->z+(j->z>0?.005:-.005));
			lp._color= osg::Vec4d((j->color>>16)&0xff,
			  (j->color>>8)&0xff,j->color&0xff,
			  (j->color>>24)&0xff);
			lp._radius= j->radius>.5?.1*j->radius:.2*j->radius;
			if (j->unit == 2)
				lp._radius= .15;
			else
				lp._radius= .06;
			node->addLightPoint(lp);
//			fprintf(stderr,"headlight %f %f %f %f %d %x\n",
//			  j->x,j->y,j->z,j->radius,j->unit,j->color);
		}
		osg::MatrixTransform* mt=
		  (osg::MatrixTransform*)car->parts[i].model;
		if (mt != NULL)
			mt->addChild(node);
	}
#endif
	for (int j=0; j<matrices.size(); j++) {
		int p= matrices[j].part;
		if (p<0 || matrices[j].hasAnimation)
			continue;
		car->parts[p].model= matrices[j].transform;
	}
#if 0
	for (int i=0; i<car->parts.size(); i++) {
		fprintf(stderr,"part %d %d %f %f %p\n",i,car->parts[i].parent,
		  car->parts[i].xoffset,car->parts[i].zoffset,
		  car->parts[i].model.get());
	}
#endif
}

MstsShapeReaderWriter::MstsShapeReaderWriter()
{
}

bool MstsShapeReaderWriter::getFeatures(Features& features) const
{
	features.extensionFeatureMap[".s"]=
	  static_cast<vsg::ReaderWriter::FeatureMask>(READ_FILENAME);
	return true;
}

vsg::ref_ptr<vsg::Object> MstsShapeReaderWriter::read(
  const vsg::Path& filename, vsg::ref_ptr<const vsg::Options> options) const
{
	const auto ext= vsg::lowerCaseFileExtension(filename);
	if (ext != ".s")
		return {};
	vsg::Path filepath= findFile(filename,options);
	if (!filepath)
		return {};
	MSTSShape shape;
	shape.readFile(filepath.string().c_str(),NULL,NULL);	
	return shape.createModel(1,10,false,true);
}
