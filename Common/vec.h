/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : column-major vectors and matrix
*/
#pragma once
#include <iostream>
#include "constant.h"

namespace redips{
	template<typename T>
	class Vec2{
	public:
		T x, y;
		Vec2() : x(T(0)), y(T(0)){};
		explicit Vec2(T v) : x(T(v)), y(T(v)){};
		Vec2(T tx, T ty) : x(tx), y(ty){};

		T width() const { return x; };
		T height() const { return y; };

		bool operator ==(const Vec2<T>& another) const { return x == another.x&&y == another.y; };
		bool operator !=(const Vec2<T>& another) const { return x != another.x || y != another.y; };

		Vec2<T> operator/ (const Vec2& another) const{ return Vec2<T>(x / another.x, y / another.y); };
		Vec2<T> operator/ (float v) const{ return Vec2<T>(x / v, y / v); };

		Vec2<T> operator* (const T v) const { return Vec2<T>(x*v, y*v); };
		Vec2<T> operator* (const Vec2<T> &v) const{ return Vec2<T>(x*v.x, y*v.y); };
		Vec2<T>& operator*= (const T v) { x *= v, y *= v; return *this; };
		Vec2<T>& operator*= (const Vec2<T> &v) { x *= v.x, y *= v.y; return *this; };

		Vec2<T> operator+ (const Vec2<T> &v) const{ return Vec2<T>(x + v.x, y + v.y); };
		Vec2<T>& operator+=(const Vec2<T> &v) { x += v.x, y += v.y; return *this; }

		Vec2<T> operator- (const Vec2<T> &v) const{ return Vec2<T>(x - v.x, y - v.y); };
		Vec2<T>& operator-=(const Vec2<T> &v) { x -= v.x, y -= v.y; return *this; }

		T operator^ (const Vec2<T> &v) const { return (T)(x*v.y - y*v.x); };

		T dot(const Vec2<T> &v) const { return x*v.x + y*v.y ; };

		float length() const { return sqrt((float)x*x + y*y); };
		T length2() const { return x*x + y*y; };

		Vec2<float> unit() const {
			float scale = 1.0 / length();
			return (*this)*scale;
		}
	};
	typedef Vec2<int> int2;
	typedef Vec2<float > float2;
	typedef Vec2<double > double2;
	typedef Vec2<unsigned int> uint2;

	template<typename T>
	class Mat33;

	template<typename T>
	class Mat44;

	template<typename T>
	class Vec3{
	public:
		T x, y, z;
		Vec3() :x(T(0)), y(T(0)), z(T(0)){};
		explicit Vec3(T v) :x(T(v)), y(T(v)), z(T(v)){};
		explicit Vec3(T* v) :x(v[0]), y(v[1]), z(v[2]){};
		Vec3(T tx, T ty, T tz) :x(T(tx)), y(T(ty)), z(T(tz)){};

		float length() const { return sqrt((float)x*x + y*y + z*z); };
		T length2() const { return x*x + y*y + z*z; };

		Vec3<float> unit() const{
			float scale = 1.0 / length();
			return Vec3<float>(x*scale, y*scale, z*scale);
		}
		
		template <class Q>
		T dot(const Vec3<Q> &v) const { return x*v.x + y*v.y + z*v.z; };

		Vec3<T> square() const{ return Vec3<T>(x*x, y*y, z*z); };

		Vec3<T> bgr() const { return Vec3<T>(z,y,x); };

		unsigned char maxdim(){
			unsigned char dim = 0;
			if (x < y){ dim = 1; if (z > y) return 2; }
			else{ if (z > x) return 2; }
			return dim;
		}

		static Vec3<float> random(){ return Vec3<float>(rand() % 10 * 0.1, rand() % 10 * 0.1, rand() % 10 * 0.1); }

		Vec3<unsigned char> color() const{
			return Vec3<unsigned char>(CLAMP((x * 255), 0, 255), CLAMP((y * 255), 0, 255), CLAMP((z * 255), 0, 255));
		}

		T& operator[] (std::size_t idx) { return *((&x) + idx); };
		const T& operator[] (std::size_t idx) const { return *((&x) + idx); };

		bool operator<(const Vec3<T>& another) const {
			if (x == another.x) {
				if (y == another.y) {
					return z < another.z;
				}
				return y < another.y;
			}
			return x < another.x;
		}

		//friend bool operator< <> (const Vec3<T>& a, const Vec3<T>& b);
		/*
		bool operator<(const Vec3<float>& a, const Vec3<float>& b) {
			std::cout << ("float") << std::endl;;
			if (fabs(a.x - b.x) < 1e-5) {
				if (fabs(a.y - b.y) < 1e-5) {
					if (fabs(a.z - b.z) < 1e-5) {
						return false;
					}
					else {
						return a.z < b.z;
					}
				}
				else {
					return a.y < b.y;
				}
			}
			else {
				return a.x < b.x;
			}
		}
		*/

		bool operator!=(const Vec3<float>& another) const {
			if (fabs(x - another[0]) > 1e-5) return true;
			if (fabs(y - another[1]) > 1e-5) return true;
			if (fabs(z - another[2]) > 1e-5) return true;
			return false;
		}

		bool operator==(const Vec3<float>& another) const {
			return !((*this) != another);
		}

		Vec3<T> operator/ (const Vec3<T>& another) const{ return Vec3<T>(x / another.x, y / another.y, z / another.z); };
		Vec3<T> operator/= (const Vec3<T>& another) {x /= another.x, y /= another.y, z /= another.z; return *this;};
		Vec3<T> operator/ (float v) const{ return Vec3<T>(x / v, y / v, z / v);};
		Vec3<T> operator/= (float v) {x /= v, y /= v, z /= v; return *this;};

		Vec3<T> operator^ (const Vec3<T> &v) const { return Vec3<T>((y*v.z - z*v.y), (z*v.x - x*v.z), (x*v.y - y*v.x)); };

		Vec3<T> operator* (const T v) const { return Vec3<T>(x*v, y*v, z*v); };
		Vec3<T> operator* (const Vec3<T> &v) const{ return Vec3<T>(x*v.x, y*v.y, z*v.z); };
		Vec3<T>& operator*= (const T v) { x *= v, y *= v, z *= v; return *this; };
		Vec3<T>& operator*= (const Vec3<T> &v) { x *= v.x, y *= v.y, z *= v.z; return *this; };

		Vec3<T> operator+ (const T &v) const { return Vec3<T>(x + v, y + v, z + v); };
		Vec3<T> operator+ (const Vec3<T> &v) const{ return Vec3<T>(x + v.x, y + v.y, z + v.z); };
		Vec3<T>& operator+= (const T v) { x += v, y += v, z += v; return *this; };
		Vec3<T>& operator+= (const Vec3<T> &v) { x += v.x, y += v.y, z += v.z; return *this; };

		Vec3<T> operator- (const T &v) const { return Vec3<T>(x - v, y - v, z - v); };
		Vec3<T> operator- (const Vec3<T> &v) const{ return Vec3<T>(x - v.x, y - v.y, z - v.z); };
		Vec3<T>& operator-= (const T v) { x -= v, y -= v, z -= v; return *this; };
		Vec3<T>& operator-= (const Vec3<T> &v) { x -= v.x, y -= v.y, z -= v.z; return *this; };

		static Vec3<T> bits(unsigned int value){ return Vec3<T>(T((value & 4) > 0), T((value & 2) > 0), T((value & 1) > 0)); }

		/*
		Vec3<T> operator* (const Mat33<T>& mat) const{
		return Vec3<T>((x*mat[0][0]+y*mat[1][0]+z*mat[2][0]),(x*mat[0][1]+y*mat[1][1]+z*mat[2][1]),(x*mat[0][2]+y*mat[1][2]+z*mat[2][2]));
		};
		*/
		const Vec2<T> vec2() const { return Vec2<T>(x, y); };
	};
	typedef Vec3<unsigned int> uint3;
	typedef Vec3<int > int3;
	typedef Vec3<float > float3;
	typedef Vec3<double > double3;

	template<typename T>
	class Vec4{
	public:
		T x, y, z, w;
		Vec4() : x(T(0)), y(T(0)), z(T(0)), w(T(0)){};
		explicit Vec4(T v) : x(v), y(v), z(v), w(v){};
		explicit Vec4(T* v) : x(v[0]), y(v[1]), z(v[2]), w(v[3]){};
		Vec4(T tx, T ty, T tz, T tw) : x(tx), y(ty), z(tz), w(tw){};
		Vec4(const Vec3<T> &v, T w) : x(v.x), y(v.y), z(v.z), w(w){};

		/*
		Vec4<T> operator* (const Mat44<T>& mat) const{
		return Vec4<T>((x*mat[0][0]+y*mat[1][0]+z*mat[2][0]+w*mat[3][0]),(x*mat[0][1]+y*mat[1][1]+z*mat[2][1]+w*mat[3][1]),(x*mat[0][2]+y*mat[1][2]+z*mat[2][2]+w*mat[3][2]),(x*mat[0][3]+y*mat[1][3]+z*mat[2][3]+w*mat[3][3]));
		};
		*/
		T& operator[] (std::size_t idx) { return *((&x) + idx); };
		const T& operator[] (std::size_t idx) const { return *((&x) + idx); }

		Vec3<T> vec3() const{ return Vec3<T>(x / w, y / w, z / w); }
	};
	typedef Vec4<int > int4;
	typedef Vec4<float > float4;
	typedef Vec4<double > double4;

	template<typename T>
	class Mat33{
	private:
		T m[3][3];
	public:
		Mat33() { memset(m, 0, sizeof(T)* 9); }
		Mat33(T* data){ for (int i = 0; i < 9; i++) m[i / 3][i % 3] = data[i]; };

		Vec3<T> x() const { return Vec3<T>(m[0][0], m[0][1], m[0][2]); }
		Vec3<T> y() const { return Vec3<T>(m[1][0], m[1][1], m[1][2]); }
		Vec3<T> z() const { return Vec3<T>(m[2][0], m[2][1], m[2][2]); }

		Vec3<T> col(int idx) const { return Vec3<T>(m[0][idx], m[1][idx], m[2][idx]); }
		Vec3<T> row(int idx) const { return Vec3<T>(m[idx][0], m[idx][1], m[idx][2]); }

		Vec3<T> mix(const Vec3<T>& coords) const{
			return x()*coords.x + y()*coords.y + z()*coords.z;
		};

		const T* ptr() const { return (T*)(&(m[0][0])); };
		T* operator[] (std::size_t idx){ return m[idx]; };
		const T* operator[] (std::size_t idx) const { return m[idx]; };

		Mat33<T> operator* (const Mat33<T>& v) const{ Mat33<T> ret; for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++){ ret[i][j] = T(0); for (int k = 0; k < 3; k++) ret[i][j] += (m[i][k] * v[k][j]); } return ret; }
		Vec3<T> operator* (const Vec3<T>& v) const{
			Vec3<T> ret;
			for (int i = 0; i < 3; i++) 	for (int j = 0; j < 3; j++)
				ret[i] += m[i][j] * v[j];
			return ret;
		}

		void setrow(const Vec3<T>& data, std::size_t idx){ for (int i = 0; i < 3; i++) m[idx][i] = data[i]; };
		void setcol(const Vec3<T>& data, std::size_t idx){ for (int i = 0; i < 3; i++) m[i][idx] = data[i]; };

		Mat33<T> transpose() const{ Mat33<T> ret;  for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) ret[i][j] = m[j][i];  return ret; }
		static Mat33<T> eye(){ Mat33<T> ret;	for (int i = 0; i < 3; ++i) ret[i][i] = T(1); return ret; };

		//column-major transform
		//rotate around x
		static Mat33<T> tilt(float angle){ 
			Mat33<T> ret = Mat33<T>::eye();
			ret[1][1] = ret[2][2] = cos(angle);
			ret[2][1] = ret[1][2] = sin(angle);
			ret[1][2] *= -1;
			return ret;
		}
		//rotate around y
		static Mat33<T> pan(float angle){
			Mat33<T> ret = Mat33<float>::eye();
			ret[0][0] = ret[2][2] = cos(angle);
			ret[0][2] = ret[2][0] = sin(angle);
			ret[2][0] *= -1;
			return ret;
		}
		//rotate around z
		static Mat33<T> roll(float angle){
			Mat33<T> ret = Mat33<float>::eye();
			ret[0][0] = ret[1][1] = cos(angle);
			ret[0][1] = ret[1][0] = sin(angle);
			ret[0][1] *= -1;
			return ret;
		}
		static Mat33<T> xyz(){
			Mat33<T> ret;
			ret.setcol(Vec3<T>(1, 0, 0), 0);
			ret.setcol(Vec3<T>(0, 1, 0), 1);
			ret.setcol(Vec3<T>(0, 0, 1), 2);
			return ret;
		}
		static Mat33<T> zxy(){
			Mat33<T> ret;
			ret.setcol(Vec3<T>(0, 1, 0), 0);
			ret.setcol(Vec3<T>(0, 0, 1), 1);
			ret.setcol(Vec3<T>(1, 0, 0), 2);
			return ret;
		}
		static Mat33<T> yzx(){
			Mat33<T> ret;
			ret.setcol(Vec3<T>(0, 0, 1), 0);
			ret.setcol(Vec3<T>(1, 0, 0), 1);
			ret.setcol(Vec3<T>(0, 1, 0), 2);
			return ret;
		}
	};

	template<typename T>
	class Mat44{
	private:
		T m[4][4];
	public:
		Mat44() { memset(m, 0, sizeof(T)* 16); };
		Mat44(T* data){ for (int i = 0; i < 16; i++) m[i / 4][i % 4] = data[i]; };
		Mat44(Mat33<T> mat3){
			memset(m, 0, sizeof(T)* 16);
			for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m[i][j] = mat3[i][j];
			m[3][3] = T(1);
		}

		T* operator[] (std::size_t idx){ return m[idx]; };
		const T* operator[] (std::size_t idx) const { return m[idx]; };
		const T* ptr() const { return (T*)(&(m[0][0])); };

		Mat44<T> operator* (const Mat44<T>& v) const{ Mat44<T> ret; 	for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++){ ret[i][j] = T(0); for (int k = 0; k < 4; k++) ret[i][j] += (m[i][k] * v[k][j]); } return ret; }
		Vec4<T> operator* (const Vec4<T>& v) const{ Vec4<T> ret; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) ret[i] += m[i][j] * v[j];	return ret; }

		Mat44<T> transpose() const { Mat44<T> ret; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) ret[i][j] = m[j][i]; return ret; }

		operator Mat33<T>() const {
			Mat33<T> ret;
			for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j){
				ret[i][j] = m[i][j];
			}
			return ret;
		};

		Vec4<T> col(int idx) const { return Vec4<T>(m[0][idx], m[1][idx], m[2][idx], m[3][idx]); }
		Vec4<T> row(int idx) const { return Vec4<T>(m[idx][0], m[idx][1], m[idx][2], m[idx][3]); }

		void setrow(const Vec4<T>& data, std::size_t idx) { for (int i = 0; i < 4; i++) m[idx][i] = data[i]; };
		void setcol(const Vec4<T>& data, std::size_t idx){ for (int i = 0; i < 4; i++) m[i][idx] = data[i]; };
		void setUpperLeft(const Mat33<T>& upperLeft){ for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m[i][j] = upperLeft[i][j]; }
		
		//column-major transform
		static Mat44<T> eye(){ Mat44<T> ret; for (int i = 0; i < 4; i++) ret[i][i] = T(1); return ret; }
		static Mat44<T> translation(const float3& offset){
			Mat44<T> ret = Mat44<T>::eye(); ret.setcol(float4(offset, T(1)), 3); return ret;
		}
		static Mat44<T> scale(float3 ratio){ Mat44<T> ret = Mat44<T>::eye(); for (int i = 0; i < 3; i++) ret[i][i] = ratio[i];	return ret; }
		static Mat44<T> rotatex(float angle){
			Mat44<T> ret = Mat44<T>::eye();
			ret[1][1] = ret[2][2] = cos(angle);
			ret[2][1] = ret[1][2] = sin(angle);
			ret[1][2] *= -1;
			return ret;
		}
		static Mat44<T> rotatey(float angle){
			Mat44<T> ret = Mat44<T>::eye();
			ret[0][0] = ret[2][2] = cos(angle);
			ret[0][2] = ret[2][0] = sin(angle);
			ret[2][0] *= -1;
			return ret;
		}
		static Mat44<T> rotatez(float angle){
			Mat44<T> ret = Mat44<T>::eye();
			ret[0][0] = ret[1][1] = cos(angle);
			ret[0][1] = ret[1][0] = sin(angle);
			ret[0][1] *= -1;
			return ret;
		}
	};

	typedef Mat33<float > Mat33f;
	typedef Mat44<float > Mat44f;

	template<typename T, int n>
	class VecXd{
		T val[n];
	public:
		const static int Dim = n;
	public:
		VecXd(std::initializer_list<T> values){
			int mindim = std::min(n, int(values.size()));
			int id = 0;
			for (auto iter = values.begin(); iter != values.end() && id < n; ++iter, ++id){
				val[id] = *iter;
			}
			for (; id < n; ++id) val[id] = T(0);
		}
		explicit VecXd(T v = T(0), int k = n){
			for (int i = 0; i < k; ++i) val[i] = T(v);
			for (int i = k; i < n; ++i) val[i] = T(0);
		}
		VecXd(const T* ary){ for (int i = 0; i < n; ++i) val[i] = ary[i]; }

		const T& operator[](size_t idx) const {
			_RUNTIME_ASSERT_(idx >= 0 && idx < Dim, "idx>=0&&idx<Dim");
			return val[idx];
		}
		T& operator[](size_t idx) {
			_RUNTIME_ASSERT_(idx >= 0 && idx < Dim, "idx>=0&&idx<Dim");
			return val[idx];
		}

		T* ptr() { return &val[0]; };

		void clear() { for (int i = 0; i < Dim; ++i) val[i] = T(0); }

		template<class Q = T>
		T dot(const VecXd<Q, n> &v) const {
			T ret = 0;
			for (int i = 0; i < n; ++i) ret += val[i] * v[i];
			return ret;
		};

		T unit() const {
			auto inv = 1.0 / norm();
			return this->operator*(inv);
		}

		void normalize() {
			auto inv = 1.0 / norm();
			*this *= inv;
		}

		T norm2() const { return this->dot(*this); }

		T norm() const { return sqrt(norm2()); }

		VecXd<T, n> operator*(const VecXd<T, n>& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] * another[i];
			return ret;
		}
		VecXd<T, n> operator*(const T& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] * another;
			return ret;
		}
		const VecXd<T, n>& operator*=(const VecXd<T, n>& another){
			for (int i = 0; i < n; ++i) val[i] *= another[i];
			return *this;
		}
		const VecXd<T, n>& operator*=(const T& another){
			for (int i = 0; i < n; ++i) val[i] *= another;
			return *this;
		}

		VecXd<T, n> operator+(const VecXd<T, n>& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] + another[i];
			return ret;
		}
		VecXd<T, n> operator+(const T& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] + another;
			return ret;
		}
		const VecXd<T, n>& operator+=(const VecXd<T, n>& another) {
			for (int i = 0; i < n; ++i) val[i] += another[i];
			return *this;
		}
		const VecXd<T, n>& operator+=(const T& another) {
			for (int i = 0; i < n; ++i) val[i] += another;
			return *this;
		}

		VecXd<T, n> operator-(const VecXd<T, n>& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] - another[i];
			return ret;
		}
		VecXd<T, n> operator-(const T& another) const {
			VecXd<T, n> ret;
			for (int i = 0; i < n; ++i) ret[i] = val[i] - another;
			return ret;
		}
		const VecXd<T, n>& operator-=(const VecXd<T, n>& another) {
			for (int i = 0; i < n; ++i) val[i] -= another[i];
			return *this;
		}
		const VecXd<T, n>& operator-=(const T& another) {
			for (int i = 0; i < n; ++i) val[i] -= another;
			return *this;
		}

	};
};
