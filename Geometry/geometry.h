/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : Geometry classes
*/
#pragma once
#include "../Common/vec.h"
namespace redips{
	class BOX{
	public:
		float3 lbb, rtf;
		BOX(){ reset(); };
		BOX(const float3& a, const float3& b){ lbb = a; rtf = b; };
		void reset(){ lbb = float3(FLT_MAX); rtf = float3(-FLT_MAX); }

		const BOX& operator+= (const float3 &p){
			lbb.x = MIN(lbb.x, p.x); lbb.y = MIN(lbb.y, p.y); lbb.z = MIN(lbb.z, p.z);
			rtf.x = MAX(rtf.x, p.x); rtf.y = MAX(rtf.y, p.y); rtf.z = MAX(rtf.z, p.z);
			return *this;
		}

		const BOX& operator+= (const BOX& box){
			(*this) += box.lbb;
			(*this) += box.rtf;
			return (*this);
		}

		BOX operator+ (const float3& p){
			BOX ret = (*this);
			ret += p;
			return ret;
		}

		BOX operator+ (const BOX& box){
			BOX ret = (*this);
			ret += box;
			return ret;
		}

		float right() const { return rtf.x; }
		float top() const { return rtf.y; }
		float front() const { return rtf.z; }
		float left() const { return lbb.x; }
		float bottom() const { return lbb.y; }
		float back() const { return lbb.z; }
		float xdim() const { return right() - left(); };
		float ydim() const { return top() - bottom(); }
		float zdim() const { return front() - back(); }
		float3 dim() const { return float3(xdim(), ydim(), zdim()); };
		float3 heart() const{ return (lbb + rtf)*0.5f; }

		friend std::ostream& operator<<(std::ostream& os, const BOX& box){
			return (os << "box {" << "x : " << box.left() << "-" << box.right() << " y : " << box.bottom() << "-" << box.top() << " z : " << box.back() << "-" << box.front() << "}" << std::endl);
		}
	};

	class Light{
	public:
		float3 position;
		float3 intensity;
		Light(){
			position = float3(0.0f);
			intensity = float3{ 1.0f, 1.0f, 1.0f };
		}
		Light(float3 pos, float3 color) :position(pos), intensity(color){};
	};

	class GeoUtil{
	public:
		static float triarea(const float3& a, const float3& b, const float3& c){
			float3 ab = b - a;
			float3 ac = c - a;
			return (ab^ac).length()*0.5;
		}
		static float3 barycoord(const float3& a, const float3& b, const float3& c, const float3& p) {
			float area = triarea(a, b, c);
			float3 pa = a - p;
			float3 pb = b - p;
			float3 pc = c - p;
			return float3((pb^pc).length()*0.5 / area, (pc^pa).length()*0.5 / area, (pa^pb).length()*0.5 / area);
		}
		static float3 rotateAroundAxis(const float3& axis, const float3& pos, float theta, float3 O = float3(0.0f)){
			puts("rotate around axis may wrong , modify later");
			float cost = cos(theta);
			float3 tpos = pos - O;
			return tpos*cost + (axis ^ tpos)*sin(theta) + axis*(axis.dot(tpos))*(1 - cost) + O;
		}
		static bool pinQuadrangle2d(const float2& a, const float2& b, const float2& c, const float2& d, const float2& p){
			if (((b - a) ^ (p - a)) < 0.0f) return false;
			if (((c - b) ^ (p - b)) < 0.0f) return false;
			if (((d - c) ^ (p - c)) < 0.0f) return false;
			if (((a - d) ^ (p - d)) < 0.0f) return false;
			return true;
		}
		//glortho may wrong, debug later
		static Mat44f glOrtho(const BOX& box){
			Mat44f ret;
			ret[0][0] = 2.0f / box.xdim();      ret[0][3] = -((box.right() + box.left()) / box.xdim());
			ret[1][1] = 2.0f / box.ydim();       ret[1][3] = -((box.top() + box.bottom()) / box.ydim());
			ret[2][2] = 2.0f / box.zdim();      ret[2][3] = -((box.front() + box.back()) / box.zdim());
			ret[3][3] = 1.0f;
			return ret;
		}
		//glortho may wrong, debug later
		static Mat44f glOrtho(float l, float r, float d, float t, float b, float f){
			Mat44f ret;
			float xdim = fabs(r - l);
			float ydim = fabs(t - d);
			float zdim = fabs(f - b);
			ret[0][0] = 2.0f / xdim;      ret[0][3] = -((r + l) / xdim);
			ret[1][1] = 2.0f / ydim;       ret[1][3] = -((t + d) / ydim);
			ret[2][2] = 2.0f / zdim;      ret[2][3] = -((f + b) / zdim);
			ret[3][3] = 1.0f;
			return ret;
		}
	};

	class Ray{
	public:
		Ray(){};
		Ray(const float3 &origion, const float3 &direction){
			ori = origion;
			dir = direction.unit();
			inv_dir = float3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
		}
		float3 ori, dir, inv_dir;
	public:
		bool intersect(const BOX &box, float &tmin, float &tmax) const {
			float l1 = (box.lbb.x - ori.x) * inv_dir.x;
			float l2 = (box.rtf.x - ori.x) * inv_dir.x;
			tmin = fminf(l1, l2);
			tmax = fmaxf(l1, l2);

			l1 = (box.lbb.y - ori.y) * inv_dir.y;
			l2 = (box.rtf.y - ori.y) * inv_dir.y;
			tmin = fmaxf(fminf(l1, l2), tmin);
			tmax = fminf(fmaxf(l1, l2), tmax);

			l1 = (box.lbb.z - ori.z) * inv_dir.z;
			l2 = (box.rtf.z - ori.z) * inv_dir.z;
			tmin = fmaxf(fminf(l1, l2), tmin);
			tmax = fminf(fmaxf(l1, l2), tmax);

			return ((tmax >= tmin) && (tmax >= 0.0f));
		}
		float intersect(const float3 &v0, const float3 &edge1, const float3 &edge2) const{
			float3 tvec = ori - v0;
			float3 pvec = (dir) ^ edge2;
			float  det = 1.0f / ((edge1.dot(pvec)));
			float u = (tvec.dot(pvec)) * det;
			if (u < 0.0f || u > 1.0f)  return -1.0f;
			float3 qvec = (tvec^edge1);
			float v = (dir).dot(qvec)*det;
			if (v < 0.0f || (u + v) > 1.0f) return -1.0f;
			return (edge2.dot(qvec)) * det;
		}
		bool intersect(const float3 &sphere, float radius, float &dist) const {
			float b, c, d;
			float3 sr = ori - sphere;
			b = (sr.dot(dir));
			c = (sr.length2()) - (radius*radius);
			d = b*b - c;
			if (d > 0){
				float e = sqrt(d);
				float t0 = -b - e;
				if (t0 < 0) dist = -b + e;
				else dist = MIN((-b - e), (-b + e));
				return true;
			}
			return false;
		}
	};

	class HitRecord{
	public:
		HitRecord() { reset(); }
		void reset(){
			distance = FLT_MAX;
			hitIndex = -1;
			color = float3(float(0));
		}
		float distance;
		float3 color;
		float3 normal;
		int hitIndex;
	};
};

//used when building a kd-tree,a HOOK contains the index and the bounding-box of a geometry,such as sphere,triangle 
typedef std::pair<int, redips::BOX*> HOOK;
