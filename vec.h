#pragma once
#include <cstdlib>
#include <iostream>
#include <constant.h>

template<typename T>
class Vec2{
public:
      T x,y;
      Vec2() : x(T(0)), y(T(0)){};
	  Vec2(T v) : x(T(v)), y(T(v)){};
	  Vec2(T tx,T ty) : x(tx), y(ty){};

	  T width() const {return x;};
	  T height() const {return y;};

	  Vec2<T> operator* (const T v) const { return Vec2<T>(x*v, y*v); };
	  Vec2<T> operator* (const Vec2<T> &v) const{ return Vec2<T>(x*v.x, y*v.y); };
	  Vec2<T>& operator*= (const T v) { x *= v, y *= v; return *this; };
	  Vec2<T>& operator*= (const Vec2<T> &v) { x *= v.x, y *= v.y; return *this; };

	  Vec2<T> operator+ (const Vec2<T> &v) const{ return Vec2<T>(x + v.x, y + v.y); };

	  Vec2<T> operator- (const Vec2<T> &v) const{ return Vec2<T>(x - v.x, y - v.y); };

	  T operator^ (const Vec2<T> &v) const { return (T)(x*v.y - y*v.x); };
};
typedef Vec2<int> int2;
typedef Vec2<float > float2;

template<typename T>
class Mat33;

template<typename T>
class Mat44;

template<typename T>
class Vec3{
public:
	T x,y,z;
	Vec3():x(T(0)), y(T(0)), z(T(0)){};
	Vec3(T v):x(T(v)),y(T(v)),z(T(v)){};
	Vec3(T tx, T ty, T tz):x(T(tx)),y(T(ty)),z(T(tz)){};
	Vec3(T* v) :x(v[0]), y(v[1]), z(v[2]){};

    float length() const { return sqrt((float)x*x + y*y + z*z); };
    T length2() const { return x*x + y*y + z*z; };

	Vec3<float> unit() const{
		float scale = 1.0 / length();
		return (*this)*scale;
	}

	T dot(const Vec3<T> &v) const { return x*v.x + y*v.y + z*v.z; };

	Vec3<T> square() const{ return Vec3<T>(x*x, y*y, z*z); };

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

	static Vec3<T> bits(unsigned int value){ return Vec3<T>(T(value&4>0),T(value&2>0),T(value&1>0)); }

	/*
    Vec3<T> operator* (const Mat33<T>& mat) const{
            return Vec3<T>((x*mat[0][0]+y*mat[1][0]+z*mat[2][0]),(x*mat[0][1]+y*mat[1][1]+z*mat[2][1]),(x*mat[0][2]+y*mat[1][2]+z*mat[2][2]));
    };
    */
	const Vec2<T> vec2() const { return Vec2<T>(x, y); };
};
typedef Vec3<float > float3;
typedef Vec3<int > int3;

template<typename T>
class Vec4{
public:
	T x, y, z, w;
	Vec4() : x(T(0)), y(T(0)), z(T(0)), w(T(0)){};
	Vec4(T v) : x(v), y(v), z(v), w(v){};
	Vec4(T tx, T ty, T tz, T tw) : x(tx), y(ty), z(tz), w(tw){};
	Vec4(T* v) : x(v[0]), y(v[1]), z(v[2]),w(v[3]){};
	Vec4(const Vec3<T> &v,T w) : x(v.x), y(v.y), z(v.z), w(w){};

	/*
    Vec4<T> operator* (const Mat44<T>& mat) const{
           return Vec4<T>((x*mat[0][0]+y*mat[1][0]+z*mat[2][0]+w*mat[3][0]),(x*mat[0][1]+y*mat[1][1]+z*mat[2][1]+w*mat[3][1]),(x*mat[0][2]+y*mat[1][2]+z*mat[2][2]+w*mat[3][2]),(x*mat[0][3]+y*mat[1][3]+z*mat[2][3]+w*mat[3][3]));
    };
	*/
	T& operator[] (std::size_t idx) { return *((&x) + idx); };
    const T& operator[] (std::size_t idx) const {return *((&x)+idx);}

	Vec3<T> vec3() const{ return Vec3<T>(x/w,y/w,z/w); }
};
typedef Vec4<int > int4;
typedef Vec4<float > float4;

template<typename T>
class Mat33{
private:
	T m[3][3];
public:
	Mat33() { memset(m, 0, sizeof(T)* 9); }
	Mat33(T* data){ for (int i=0; i<9; i++) m[i / 3][i % 3] = data[i]; };

	T* operator[] (std::size_t idx){ return m[idx]; };
	const T* operator[] (std::size_t idx) const { return m[idx]; };

	Mat33<T> operator* (const Mat33<T>& v) const{ Mat33<T> ret; for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++){ ret[i][j] = T(0); for (int k = 0; k < 3; k++) ret[i][j] += (m[i][k] * v[k][j]); } return ret; }
	Vec3<T> operator* (const Vec3<T>& v) const{ 
		Vec3<T> ret; 
		for (int i = 0; i < 3; i++) 	for (int j = 0; j < 3; j++) 
			ret[i] += m[i][j] * v[j]; 
		return ret; 
	}

	void setrow(const Vec3<T>& data, std::size_t idx){	for (int i = 0; i < 3; i++) m[idx][i] = data[i];  };
	void setcol(const Vec3<T>& data,std::size_t idx){ for (int i = 0; i < 3; i++) m[i][idx] = data[i]; };

	Mat33<T> transpose() const{ Mat33<T> ret;  for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) ret[i][j] = m[j][i];  return ret; }
	static Mat33<T> eye(){ Mat33<T> ret;	for (int i = 0; i < 3; ++i) ret[i][i] = T(1); return ret; };
	
	//column-major transform
	static Mat33<T> tilt(float angle){
		Mat33<T> ret = Mat33<T>::eye();
		ret[1][1] = ret[2][2] = cos(angle);
		ret[2][1] = ret[1][2] = sin(angle);
		ret[1][2] *= -1;
		return ret;
	}
	static Mat33<T> pan(float angle){
		Mat33<T> ret = Mat33<float>::eye();
		ret[0][0] = ret[2][2] = cos(angle);
		ret[0][2] = ret[2][0] = sin(angle);
		ret[2][0] *= -1;
		return ret;
	}
	static Mat33<T> roll(float angle){
		Mat33<T> ret = Mat33<float>::eye();
		ret[0][0] = ret[1][1] = cos(angle);
		ret[0][1] = ret[1][0] = sin(angle);
		ret[0][1] *= -1;
		return ret;
	}
};

template<typename T>
class Mat44{
private:
	T m[4][4];
public:
	Mat44() { memset(m, 0, sizeof(T) * 16); };
	Mat44(T* data){ for (int i = 0; i<16; i++) m[i / 4][i % 4] = data[i]; };

	T* operator[] (std::size_t idx){ return m[idx]; };
	const T* operator[] (std::size_t idx) const { return m[idx]; };
	const T* ptr() const { return (T*)(&(m[0][0])); };

	Mat44<T> operator* (const Mat44<T>& v) const{ Mat44<T> ret; 	for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++){ ret[i][j] = T(0); for (int k = 0; k < 4; k++) ret[i][j] += (m[i][k] * v[k][j]); } return ret; }
	Vec4<T> operator* (const Vec4<T>& v) const{ Vec4<T> ret; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) ret[i] += m[i][j] * v[j];	return ret; }

	Mat44<T> transpose() const { Mat44<T> ret; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) ret[i][j] = m[j][i]; return ret; }

	void setrow(const Vec4<T>& data, std::size_t idx) { for (int i = 0; i < 4; i++) m[idx][i] = data[i]; };
	void setcol(const Vec4<T>& data, std::size_t idx){ for (int i = 0; i < 4; i++) m[i][idx] = data[i]; };
	void setUpperLeft(const Mat33<T>& upperLeft){ for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m[i][j] = upperLeft[i][j]; }

	//column-major transform
	static Mat44<T> eye(){ Mat44<T> ret; for (int i = 0; i < 4; i++) ret[i][i] = T(1); return ret; }
	static Mat44<T> translation(float3 offset){	
		Mat44<T> ret = Mat44<T>::eye(); ret.setcol(float4(offset,T(1)), 3); return ret; 
	}
	static Mat44<T> scale(float3 ratio){ 	Mat44<T> ret = Mat44<T>::eye(); for (int i = 0; i < 3; i++) ret[i][i] = ratio[i];	return ret; }
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
