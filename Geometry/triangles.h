/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.10
* Description : obj-mesh loader
		a. each mesh has a default-material. call .setDefaultMaterial(your-material) to change default-material.
        b. can only process : v、vt、vn、f、mtllib、usemtl、g
        c. several limits:
			c.1. vt is 3-dimention
			c.2. f can only has 3 format : withtex(a/b/c) withnormal(a//c) single(a)   
		bug: f 8427//8428 8664//8665 8441//8442 8428//8429 
*/
#pragma once
#include "model.h"

namespace redips{
	class Mesh{
		friend class Triangles;
	public:
		class FGroup{   //face group
		public:
			FGroup(std::string name, int fsid, int mtlId) : name(name), fsid(fsid), mtlId(mtlId){
				faceCnt = tsid = nsid = 0;
				faceType = GROUP_FACE_TYPE::_unknown_face_type_;
				hasNormals = hasTexture = false;
			}
			~FGroup(){};
			void setType(GROUP_FACE_TYPE type){
				this->faceType = type;
				if (type == GROUP_FACE_TYPE::_withnormal_) { hasNormals = true; }
				else if (type == GROUP_FACE_TYPE::_withtex_) { hasNormals = hasTexture = true; }
			}
		public:
			int mtlId;
			std::string name;
			GROUP_FACE_TYPE  faceType;
			bool hasTexture, hasNormals;

			int fsid, tsid, nsid;   //[  faces_v  ),faces_vt,faces_vn
			int faceCnt;
		};
		//matlib
		MtlLib mtllib;
		//groups
		std::vector<FGroup> groups;
	public:
		Mesh(const char* file){
			groups.push_back(FGroup("default", 0, 0));
			int curGid = 0;
			std::string basepath = ""; {
				int tid; if ((tid = std::string(file).find_last_of('/')) != -1){
					basepath = std::string(file).substr(0, tid + 1);
				}
				else if ((tid = std::string(file).find_last_of('\\')) != -1){
					basepath = std::string(file).substr(0, tid + 1);
				}
			}
			std::ifstream fin(file);
			if (!fin.is_open()){
				std::cerr << "load obj file-" << file << " failed" << std::endl;
				return;
			}
			puts("[mesh loader] : Loading...");  clock_t start, finish;  start = clock();

			std::string curMtllib = "";
			while (fin >> buff){
				if (buff == "mtllib"){
					fin >> curMtllib;
					if (curMtllib[1] != ':'){ curMtllib = basepath + curMtllib; }
					mtllib.load((curMtllib).c_str());
				}
				else if (buff == "v"){
					fin >> floatx >> floaty >> floatz;
					vertices.push_back(float3(floatx, floaty, floatz));
				}
				else if (buff == "vn"){
					fin >> floatx >> floaty >> floatz;
					normals.push_back(float3(floatx, floaty, floatz));
				}
				else if (buff == "vt"){
					fin >> floatx >> floaty >> floatz;
					texcoords.push_back(float3(floatx, floaty, floatz));
				}
				else if (buff == "usemtl"){
					fin >> buff;
					//put an end to last group
					groups[curGid].faceCnt = faces_v.size() - groups[curGid].fsid;
					//find material-id,new group
					groups.push_back(FGroup(buff, faces_v.size(), mtllib.getMtlId(curMtllib, buff)));
					curGid++;
				}
				else if (buff == "f"){
					fin >> str1 >> str2 >> str3;
					faceGroupId.push_back(curGid);

					if (groups[curGid].faceType == GROUP_FACE_TYPE::_unknown_face_type_){
						if (str1.find("//") != std::string::npos) groups[curGid].setType(GROUP_FACE_TYPE::_withnormal_);
						else if (str1.find("/") != std::string::npos) groups[curGid].setType(GROUP_FACE_TYPE::_withtex_);
						else groups[curGid].setType(GROUP_FACE_TYPE::_single_);
						if (groups[curGid].hasNormals) groups[curGid].nsid = faces_vn.size();
						if (groups[curGid].hasTexture) groups[curGid].tsid = faces_vt.size();
					}
					switch (groups[curGid].faceType){
					case GROUP_FACE_TYPE::_single_: {
									   sscanf(str1.c_str(), "%d", v + 0);
									   sscanf(str2.c_str(), "%d", v + 1);
									   sscanf(str3.c_str(), "%d", v + 2);
									   faces_v.push_back(int3(v[0] - 1, v[1] - 1, v[2] - 1));
									   break;
					}
					case GROUP_FACE_TYPE::_withnormal_:{
										  sscanf(str1.c_str(), "%d//%d", v + 0, vn + 0);
										  sscanf(str2.c_str(), "%d//%d", v + 1, vn + 1);
										  sscanf(str3.c_str(), "%d//%d", v + 2, vn + 2);
										  faces_v.push_back(int3(v[0] - 1, v[1] - 1, v[2] - 1));
										  faces_vn.push_back(int3(vn[0] - 1, vn[1] - 1, vn[2] - 1));
										  break;
					}
					case GROUP_FACE_TYPE::_withtex_:{
									   sscanf(str1.c_str(), "%d/%d/%d", v + 0, vt + 0, vn + 0);
									   sscanf(str2.c_str(), "%d/%d/%d", v + 1, vt + 1, vn + 1);
									   sscanf(str3.c_str(), "%d/%d/%d", v + 2, vt + 2, vn + 2);
									   faces_v.push_back(int3(v[0] - 1, v[1] - 1, v[2] - 1));
									   faces_vt.push_back(int3(vt[0] - 1, vt[1] - 1, vt[2] - 1));
									   faces_vn.push_back(int3(vn[0] - 1, vn[1] - 1, vn[2] - 1));
									   break;
					}
					}
				}
			}
			groups[groups.size() - 1].faceCnt = faces_v.size() - groups[groups.size() - 1].fsid;
			finish = clock();
			printf("[mesh loader] : load finish,cost %lf ms\n", (double)(finish - start) / CLOCKS_PER_SEC * 1000);
			//std::cout << (*this);
		}
		Mesh(){
			groups.push_back(FGroup("default", 0, 0));
			groups[0].setType(GROUP_FACE_TYPE::_withnormal_);
		}
		~Mesh(){
			vertices.clear(); normals.clear(); texcoords.clear();
			faces_v.clear(); faces_vn.clear(); faces_vt.clear(); faceGroupId.clear();
			groups.clear();
		};
		const Material& getMaterial(int faceId) const{
			return *mtllib[(groups[faceGroupId[faceId]].mtlId)];
		}
		friend std::ostream& operator<<(std::ostream &os, const Mesh& mesh){
			os << mesh.faces_v.size() << " faces, " << mesh.vertices.size() << " vertices, " << mesh.normals.size() << " normals, " << mesh.texcoords.size() << " texcoords, {" << std::endl;
			for (int i = 0; i < mesh.groups.size(); i++){
				os << "\tmesh " << i << " : " << mesh.groups[i].name << "{" << std::endl;
				os << "\t\t " << mesh.groups[i].faceCnt << " faces, "
					<< (mesh.groups[i].hasTexture ? " has texture, " : "")
					<< (mesh.groups[i].hasNormals ? " has normals, " : "")
					<< "material name : " << mesh.mtllib[mesh.groups[i].mtlId]->name
					<< std::endl;
				os << "\t}" << std::endl;
			}
			return (os << "}" << std::endl);
		}
	public:
		//mesh data
		std::vector<float3> vertices;
		std::vector<float3> normals;
		std::vector<float3> texcoords;
		//index
		std::vector<int3> faces_v;
		std::vector<int3> faces_vn;
		std::vector<int3> faces_vt;
		// face -> (faceGroupId) -> material-id in mtls&(vertices/normals/texcoords)
		std::vector<int> faceGroupId;
	private:
		//buffer
		float floatx, floaty, floatz;
		int v[3], vn[3], vt[3];
		std::string buff, str1, str2, str3;
	};

	class Triangles : public Model {
	public:
		Triangles(const char* file){
			useTree = false;
			type = MODEL_TYPE::_triangle_;
			mesh = new Mesh(file);
			vertCnt = mesh->vertices.size();
			faceCnt = mesh->faces_v.size();

			getRawAABB();
		}
		Triangles(){ setup(); }
		Triangles(const float3 &boxdim,float3 center = float3(0.0f,0.0f,0.0f)){
			setup();
			float3 base = boxdim * -0.5f + center;
			unsigned int xs[] {3, 3, 1, 1, 4, 4, 5, 4, 3, 1, 0, 0};
			unsigned int ys[] {7, 6, 7, 5, 2, 0, 4, 6, 2, 3, 5, 4};
			unsigned int zs[] {6, 2, 3, 7, 6, 2, 7, 7, 0, 0, 1, 5};
			for (int i = 0; i < 12; i++){
				addTriangle(base + boxdim*float3::bits(xs[i]), base + boxdim*float3::bits(ys[i]), base + boxdim*float3::bits(zs[i]));
			}
		}
		Triangles(const float3 &boxdim, const Mat33f& axises, float3 center = float3(0.0f, 0.0f, 0.0f)){
			setup();
			float3 base = center + axises.mix(boxdim*-0.5f);
			unsigned int xs[] {3, 3, 1, 1, 4, 4, 5, 4, 3, 1, 0, 0};
			unsigned int ys[] {7, 6, 7, 5, 2, 0, 4, 6, 2, 3, 5, 4};
			unsigned int zs[] {6, 2, 3, 7, 6, 2, 7, 7, 0, 0, 1, 5};
			for (int i = 0; i < 12; i++){
				addTriangle(base + axises.mix(boxdim*float3::bits(xs[i])), base + axises.mix(boxdim*float3::bits(ys[i])), base + axises.mix(boxdim*float3::bits(zs[i])));
			}
		}
		~Triangles(){
			delete mesh;
		};

		bool intersect(const Ray& ray, HitRecord &record) {
			if (useTree){
				rayBoxCnt = 0;
				return traverse(0, ray, rayBoxCnt, record);
			}
			else{
				bool hitted = false;
				for (int i = 0; i < faceCnt; i++){
					int3 indices = mesh->faces_v[i];
					float3 a = (transform * float4(mesh->vertices[indices.x], 1.0f)).vec3();
					float3 ab = (transform * float4(mesh->vertices[indices.y], 1.0f)).vec3() - a;
					float3 ac = (transform * float4(mesh->vertices[indices.z], 1.0f)).vec3() - a;
					float dist = ray.intersect(a, ab, ac);
					if (dist>0 && dist < record.distance){
						record.hitIndex = i;
						record.distance = dist;
						record.normal = (ab^ac).unit();
						hitted = true;
					}
				}
				return hitted;
			}
		}
		void buildTree(){
			if (faceCnt <= 0) return;
			useTree = true;
			hooks.resize(faceCnt);
			boxes.resize(faceCnt);
			const std::vector<float3>& vertices = mesh->vertices;
			const std::vector<int3>& faces_v = mesh->faces_v;
			for (int i = 0; i < faceCnt; i++){
				boxes[i].reset();
				for (int j = 0; j < 3; j++) boxes[i] += (transform*float4(vertices[faces_v[i][j]], 1.0f)).vec3();
				hooks[i] = std::make_pair(i, &(boxes[i]));
			}
			mtree.buildTree(&hooks);
		}
		const Material& getMaterial(int index/*faceId*/) const{
			return mesh->getMaterial(index);
		}

		float2 texcoord(int faceId, const float3& pos) const{
			const Material& mtl = mesh->getMaterial(faceId);
			const Mesh::FGroup& groupInfo = (mesh->groups[mesh->faceGroupId[faceId]]);
			const std::vector<float3>& vertices = mesh->vertices;
			const std::vector<float3>& texcoords = mesh->texcoords;
			const std::vector<int3>& faces_v = mesh->faces_v;
			const std::vector<int3>& faces_vt = mesh->faces_vt;

			int3 faceIndice = faces_v[faceId];
			float3 bary = GeoUtil::barycoord((transform*float4(vertices[faceIndice.x], 1.0f)).vec3(), (transform*float4(vertices[faceIndice.y], 1.0f)).vec3(), (transform*float4(vertices[faceIndice.z], 1.0f)).vec3(), pos);

			int3 texIndice = faces_vt[faceId - groupInfo.fsid + groupInfo.tsid];
			float3 texcoord = (texcoords[texIndice.x] * bary.x + texcoords[texIndice.y] * bary.y + texcoords[texIndice.z] * bary.z);

			texcoord.x -= int(texcoord.x), texcoord.y -= int(texcoord.y);
			if (texcoord.x < 0) texcoord.x = 1 + texcoord.x;
			if (texcoord.y < 0) texcoord.y = 1 + texcoord.y;
			return redips::float2(texcoord.x,texcoord.y);
		}

		//add a triangle to a new group, need to modify
		void addTriangle(float3 a, float3 b, float3 c){
			int base = mesh->vertices.size();

			mesh->vertices.push_back(a);
			mesh->vertices.push_back(b);
			mesh->vertices.push_back(c);

			float3 norm = ((b - a) ^ (c - a)).unit();
			mesh->normals.push_back(norm);
			mesh->normals.push_back(norm);
			mesh->normals.push_back(norm);

			mesh->faces_v.push_back(int3(base + 0, base + 1, base + 2));
			mesh->faces_vn.push_back(int3(base + 0, base + 1, base + 2));

			mesh->faceGroupId.push_back(0);   //add to default group
			mesh->groups[0].faceCnt++;

			vertCnt = mesh->vertices.size();
			faceCnt = mesh->faces_v.size();
		
			aabb_raw += a;
			aabb_raw += b;
			aabb_raw += c;
		}
		const Mesh* mesh_ptr() const { return mesh; };
	public:
		int rayBoxCnt;
		int vertCnt, faceCnt;
	private:
		Mesh *mesh;
		//for kdtree
		std::vector<HOOK> hooks;
		std::vector<BOX> boxes;

        void setup(){
			useTree = false;
			type = MODEL_TYPE::_triangle_;
			this->mesh = new Mesh();

			vertCnt = faceCnt = 0;
		}

		//check if ray intersect with mesh
		bool traverse(int boxId, const Ray& ray, int& boxCnt, HitRecord& records){
			float tmin, tmax;
			if (!ray.intersect(mtree.boxes[boxId], tmin, tmax)){
				return false;
			}
			if (mtree.gcnts[boxId] <= mtree.MAX_LEAF_SIZE) {
				boxCnt++;
				bool hitted = false;

				const std::vector<float3>& vertices = mesh->vertices;
				const std::vector<int3>& faces_v = mesh->faces_v;

				for (int i = mtree.gspos[boxId]; i < mtree.gspos[boxId] + mtree.gcnts[boxId]; i++){
					int3 face = faces_v[hooks[i].first];
					float3 dota = (transform*float4(vertices[face.x], 1.0)).vec3();
					float3 edge1 = (transform*float4(vertices[face.y], 1.0)).vec3() - dota;
					float3 edge2 = (transform*float4(vertices[face.z], 1.0)).vec3() - dota;
					float dist = ray.intersect(dota, edge1, edge2);
					if (dist>0){
						if (dist < records.distance){
							records.distance = dist;
							records.normal = (edge1 ^ edge2).unit();
							records.hitIndex = hooks[i].first;
							hitted = true;
						}
					}
				}
				return hitted;
			}

			bool hitl = (mtree.lchild[boxId] >= 0) ? (traverse(mtree.lchild[boxId], ray, boxCnt, records)) : false;
			bool hitr = (mtree.rchild[boxId] >= 0) ? (traverse(mtree.rchild[boxId], ray, boxCnt, records)) : false;

			return (hitl || hitr);
		}
		
		//update vertices's aabb
		void getRawAABB(){
			aabb_raw.reset();
			const std::vector<float3>& vertices = mesh->vertices;
			for (int i = 0; i < vertCnt; i++) aabb_raw += vertices[i];
		}
	};
};

