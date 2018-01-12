/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.10
* Description : obj-mesh loader
		a. each mesh has a default-material. call .setDefaultMaterial(your-material) to change default-material.
        b. can only process : v、vt、vn、f、mtllib、usemtl、g
        c. several limits:
			c.1. vt is 3-dimention
			c.2. f can only has 3 format : withtex(a/b/c) withnormal(a//c) single(a)   
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
			clean();
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
			puts("[mesh loader] : Loading...");  
			clock_t start = clock();

			//load file to memory
			auto _size = fin.seekg(0, std::ios::end).tellg();
			char* _buf = new char[_size];
			fin.seekg(0, std::ios::beg).read(_buf, static_cast<std::streamsize>(_size));
			fin.close();

			//parse obj file
			{
				std::stringstream sin(std::move(_buf));
				std::string curMtllib = "";
				char str[1024];
				while (sin.getline(str,1024)){
					if (STRING_UTIL.startwith(str,"v ")){
						vertices.push_back(STRING_UTIL.split2Float3(str));
						//sscanf_s(str + 2, "%f %f %f", &f3.x, &f3.y, &f3.z);
						//vertices.push_back(f3);
					}
					else if (STRING_UTIL.startwith(str, "vn ")){
						normals.push_back(STRING_UTIL.split2Float3(str));
						//sscanf_s(str + 3, "%f %f %f", &f3.x, &f3.y, &f3.z);
						//normals.push_back(f3);
					}
					else if (STRING_UTIL.startwith(str, "vt ")){
						texcoords.push_back(STRING_UTIL.split2Float3(str));
						//sscanf_s(str + 3, "%f %f %f", &f3.x, &f3.y, &f3.z);
						//texcoords.push_back(f3);
					}
					else if (STRING_UTIL.startwith(str, "f ")){
						faceGroupId.push_back(curGid);

						std::vector<std::string> strs;
						int strcnt = STRING_UTIL.split(str+2," ",strs);
						if (strcnt == 3 || strcnt == 4){
							if (groups[curGid].faceType == GROUP_FACE_TYPE::_unknown_face_type_){
								if (strs[0].find("//") != std::string::npos) groups[curGid].setType(GROUP_FACE_TYPE::_withnormal_);
								else if (strs[0].find("/") != std::string::npos) groups[curGid].setType(GROUP_FACE_TYPE::_withtex_);
								else groups[curGid].setType(GROUP_FACE_TYPE::_single_);

								if (groups[curGid].hasNormals) groups[curGid].nsid = faces_vn.size();
								if (groups[curGid].hasTexture) groups[curGid].tsid = faces_vt.size();
							}

							if (strcnt == 4) faceGroupId.push_back(curGid);
							switch (groups[curGid].faceType){
								case GROUP_FACE_TYPE::_single_: {
									   for (int ii = 0; ii < strcnt; ++ii) {
									       sscanf_s(strs[ii].c_str(), "%d", v + ii);
										   if (v[ii] < 0) v[ii] += vertices.size(); else v[ii]--;
									   }
									   faces_v.push_back(int3(v[0], v[1], v[2]));
									   if (strcnt == 4) faces_v.push_back(int3(v[0], v[2], v[3]));
									   break;
								}
								case GROUP_FACE_TYPE::_withnormal_:{
									   for (int ii = 0; ii < strcnt; ++ii) {
										   sscanf_s(strs[ii].c_str(), "%d//%d", v + ii, vn + ii);
										   if (v[ii] < 0) v[ii] += vertices.size(); else v[ii]--;
										   if (vn[ii] < 0) vn[ii] += normals.size(); else vn[ii]--;
									   }
									   faces_v.push_back(int3(v[0], v[1], v[2]));
									   faces_vn.push_back(int3(vn[0], vn[1], vn[2]));
									   if (strcnt == 4){
										   faces_v.push_back(int3(v[0], v[2], v[3]));
										   faces_vn.push_back(int3(vn[0], vn[2], vn[3]));
									   }
									   break;
								}
								case GROUP_FACE_TYPE::_withtex_:{
									   for (int ii = 0; ii < strcnt; ++ii) {
										   sscanf_s(strs[ii].c_str(), "%d/%d/%d", v + ii, vt + ii, vn + ii);
										   if (v[ii] < 0) v[ii] += vertices.size(); else v[ii]--;
										   if (vt[ii] < 0) vt[ii] += texcoords.size(); else vt[ii]--;
										   if (vn[ii] < 0) vn[ii] += normals.size(); else vn[ii]--;
									   }
									   faces_v.push_back(int3(v[0], v[1], v[2]));
									   faces_vt.push_back(int3(vt[0], vt[1], vt[2]));
									   faces_vn.push_back(int3(vn[0], vn[1], vn[2]));
									   if (strcnt == 4){
										   faces_v.push_back(int3(v[0], v[2], v[3]));
										   faces_vt.push_back(int3(vt[0], vt[2], vt[3]));
										   faces_vn.push_back(int3(vn[0], vn[2], vn[3]));
									   }
									   break;
								}
							}
						}
					}
					else if (STRING_UTIL.startwith(str, "usemtl ")){
						auto buff = STRING_UTIL.trim(str + 7);
						//put an end to last group
						groups[curGid].faceCnt = faces_v.size() - groups[curGid].fsid;
						//find material-id,new group
						groups.push_back(FGroup(buff, faces_v.size(), mtllib.getMtlId(curMtllib, buff)));
						curGid++;
					}
					else if (STRING_UTIL.startwith(str, "mtllib ")){
						curMtllib = STRING_UTIL.trim(str + 7);
						if (curMtllib[1] != ':'){ curMtllib = basepath + curMtllib; }
						mtllib.load((curMtllib).c_str());
					}
				}
				groups[groups.size() - 1].faceCnt = faces_v.size() - groups[groups.size() - 1].fsid;
			}
			clock_t finish = clock();
			printf("[mesh loader] : load finish,cost %lf ms\n", (double)(finish - start) / CLOCKS_PER_SEC * 1000);
			//std::cout << (*this);
		}
		Mesh(){
			groups.push_back(FGroup("default", 0, 0));
			groups[0].setType(GROUP_FACE_TYPE::_withnormal_);
		}
		~Mesh(){
			clean();
		};
		void clean(){
			vertices.clear(); normals.clear(); texcoords.clear();
			faces_v.clear(); faces_vn.clear(); faces_vt.clear(); faceGroupId.clear();
			groups.clear();
		}
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
		int v[4], vn[4], vt[4];
		redips::float3 f3;
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
					if (dist>record.offset && dist < record.distance){
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

		float3 texcoord(int faceId, const float3& pos) const{
			const Mesh::FGroup& groupInfo = (mesh->groups[mesh->faceGroupId[faceId]]);
			if (groupInfo.hasTexture == false) return float3();
			const Material& mtl = mesh->getMaterial(faceId);
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
			return redips::float3(texcoord.x,texcoord.y,1);
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
					if (dist>records.offset){
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

