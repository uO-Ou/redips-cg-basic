#pragma once
#include <iostream>
#include <vector>
#include <random>
#include "./triangles.h"

namespace redips {
	namespace SphericalHarmonic {

		template <int BAND_WIDTH>
		class Basic{
			VecXd<double, BAND_WIDTH * 2 + 2> Factorial;
			//for sh visulization
			BYTE   *image_buffer_uchar = nullptr;
			double *image_buffer_double = nullptr;
		private:
			double P(int l, int m, double x){
				double pmm = 1.0;
				if (m > 0){
					double fact = 1.0;
					double somx2 = sqrt((1.0 - x)*(1.0 + x));
					for (int i = 1; i <= m; ++i){
						pmm *= (-fact) * somx2;
						fact += 2.0;
					}
				}
				if (l == m) return pmm;
				
				double pmmp1 = x * (2.0*m + 1.0) * pmm;
				if (l == m + 1) return pmmp1;

				double pll = 0.0;
				for (int ll = m + 2; ll <= l; ++ll){
					pll = ((2.0*ll - 1.0)*x*pmmp1 - (ll + m - 1.0)*pmm) / (ll - m);
					pmm = pmmp1;
					pmmp1 = pll;
				}
				return pll;
			}
			double K(int l, int m){
				m = abs(m);
				double temp = ((2.0*l + 1)*Factorial[l - m]) / (4.0*PI*Factorial[l+m]);
				return sqrt(temp);
			}
			double SH(int l, int m, double theta, double phi){
				const double sqrt2 = 1.41421356237309504880;
				if (m == 0)
					return K(l, 0)*P(l, 0, cos(theta));
				if (m > 0)
					return sqrt2*K(l, m)*cos(m*phi)*P(l, m, cos(theta));
				else
					return sqrt2*K(l, -m)*sin(-m*phi)*P(l, -m, cos(theta));
			}
			int GetBand(int id){
				if (id <= 0) return 0;
				for (int i = 2; i <= BAND_WIDTH; ++i){
					if (id < i*i) return i - 1;
				}
				return 0;
			}
		public:
			const static int BandWidth = BAND_WIDTH;
			const static int Length = (BAND_WIDTH + 1)*(BAND_WIDTH + 1);

			Basic(){
				image_buffer_uchar  = new BYTE[360 * 180 * 3];
				image_buffer_double = new double[360 * 180 * 3];

				Factorial[0] = Factorial[1] = 1;
				for (int i = 2; i < BAND_WIDTH * 2 + 2; ++i){
					Factorial[i] = i * Factorial[i - 1];
				}
			}
			~Basic() {
				delete[] image_buffer_uchar;
				delete[] image_buffer_double;
			}

			double Y(int i, double theta, double phi){
				int band = GetBand(i);
				return SH(band, i - (band*(band + 1)), theta, phi);
			}

			VecXd<double, Length> YFromTP(double theta, double phi){
				int id = 0;
				VecXd<double, Length> ret;
				for (int l = 0; l <= BAND_WIDTH; ++l){
					for (int m = -l; m <= l; ++m){
						ret[id++] = SH(l, m, theta, phi);
					}
				}
				return ret;
			}

			VecXd<double, Length> YFromXYZ(double x, double y, double z){
				double inv_length = 1.0 / sqrt(x*x + y*y + z*z);
				x *= inv_length, y *= inv_length, z *= inv_length;
				auto TP = GeoUtil::XYZ_2_TP(x, y, z);
				return YFromTP(TP.x, TP.y);
			}

			void YFromPanorama(const unsigned char* Ptr, int width, int height, VecXd<double, Length>& r, VecXd<double, Length>& g, VecXd<double, Length>& b) {
				r.clear(); g.clear(); b.clear();

				std::default_random_engine generator;
				std::uniform_real_distribution<float> normal_floats(0, 1);

				const int sample_cnt = 4096;
				for (int i = 0; i < sample_cnt; ++i) {
					auto x = normal_floats(generator);
					auto y = normal_floats(generator);

					auto theta = acos(sqrt(x)) * 2; //0 - PI
					auto phi = (2 * y) * PI;        //0 - 2PI

					auto sh = sh_calculator.YFromTP(theta, phi);
					{
						int h = height * theta / PI + 0.5; h = CLAMP(h, 0, height - 1);
						int w = width * y + 0.5; w = CLAMP(w, 0, width - 1);
						auto ptr = Ptr[(width * h + w) * 3];

						r += sh * (ptr[0] / 255.0);
						g += sh * (ptr[1] / 255.0);
						b += sh * (ptr[2] / 255.0);
					}

				}
				r *= (4 * PI) / sample_cnt;
				g *= (4 * PI) / sample_cnt;
				b *= (4 * PI) / sample_cnt;
			}

			void Y2Image(const VecXd<double, Length>& ys, const char* image_path){
				auto double_ptr = image_buffer_double;
				double max_value = DBL_MIN, min_value = DBL_MAX;
				for (int T = 179; T >= 0; --T) for (int P = 0; P < 360; ++P){
					auto val = ys.dot(YFromTP(RAD(T), RAD(P)));

					max_value = std::max(max_value, val);
					min_value = std::min(min_value, val);
					*double_ptr++ = val;
					*double_ptr++ = val;
					*double_ptr++ = val;
				}
				if (max_value - min_value < 1e-4){
					memset(image_buffer_uchar, 0, sizeof(image_buffer_uchar));
					redips::FImage::saveImage(image_buffer_uchar, 360, 180, 3, image_path);
				}
				else{
					auto uchar_ptr = image_buffer_uchar;
					auto double_ptr = image_buffer_double;
					auto scale = 255 / (max_value - min_value);
					for (int T = 0; T < 180; ++T) for (int P = 0; P < 360; ++P){
						int color = int((*double_ptr - min_value)*scale);
						color = CLAMP(color, 0, 255);
						*uchar_ptr++ = color;
						*uchar_ptr++ = color;
						*uchar_ptr++ = color;
						double_ptr += 3;
					}
					redips::FImage::saveImage(image_buffer_uchar, 360, 180, 3, image_path);
				}
			}

			void Y2ImageRGB(const VecXd<double, Length>& R, const VecXd<double, Length>& G, const VecXd<double, Length>& B, const char* image_path){
				int curid = 0;
				for (int T = 179; T >= 0; --T) for (int P = 0; P < 360; ++P){
					auto Y = YFromTP(RAD(T), RAD(P));
					auto r = R.dot(Y);
					auto g = G.dot(Y);
					auto b = B.dot(Y);
					image_buffer_uchar[curid++] = CLAMP(r, 0, 255);
					image_buffer_uchar[curid++] = CLAMP(g, 0, 255);
					image_buffer_uchar[curid++] = CLAMP(b, 0, 255);
				}
				redips::FImage::saveImage(image_buffer_uchar, 360, 180, 3, image_path);
			}
		};

		template <int BAND_WIDTH>
		class SamplesOnUnitSphere {
		public:
			std::vector<redips::double3 > samples_vec3;
			std::vector<VecXd<double, (BAND_WIDTH+1)*(BAND_WIDTH+1)>> samples_vecn;

			int size() const{ return samples_vec3.size(); }

			explicit SamplesOnUnitSphere(int sampleCnt) {
				Basic<BAND_WIDTH > sh_calculator;
				std::default_random_engine generator;
				std::uniform_real_distribution<float> normal_floats(0, 1);

				samples_vec3.resize(sampleCnt);
				samples_vecn.resize(sampleCnt);

				for (int i = 0; i < sampleCnt; ++i) {
					auto x = normal_floats(generator);
					auto y = normal_floats(generator);

					auto theta = acos(sqrt(x)) * 2; //0 - PI
					auto phi = (2 * y) * PI;        //0 - 2PI
					
					samples_vec3[i] = GeoUtil::TP_2_XYZ(theta, phi);
					samples_vecn[i] = sh_calculator.YFromTP(theta, phi);
				}
			}
			
			redips::double3& operator[](size_t idx) {
				_RUNTIME_ASSERT_(idx < samples_vec3.size(), "assert [idx<samples_vec3.size()] failed");
				return samples_vec3[idx];
			}

			const redips::double3& operator[](size_t idx) const {
				_RUNTIME_ASSERT_(idx < samples_vec3.size(), "assert [idx<samples_vec3.size()] failed");
				return samples_vec3[idx];
			}
		};

		template <int BAND_WIDTH>
		class SHCubemap {
			redips::FImage* faces[6];
			//a cube with side-length 2 centered at (0,0,0)
			redips::Triangles cube = redips::float3(2, 2, 2);
		public:
			SHCubemap(const char* picnames[6]) {
				for (int i = 0; i < 6; ++i) faces[i] = new redips::FImage(picnames[i]);
			}
			~SHCubemap() { for (int i = 0; i < 6; ++i) delete faces[i]; }
			redips::float3 texture(const redips::float3& direction) {
				using namespace redips;

				//find the intersection
				redips::Ray ray({ 0,0,0 }, direction);
				redips::HitRecord recorder;
				cube.intersect(ray, recorder);
				auto hitp = ray.ori + ray.dir * recorder.distance;

				//fabs(one-dim-of-$hitp$) must equal 1, because $hitp$ is on one face of $cube$
				int FaceId = -1;
				float2 texcoord;
				const float epsi = 1e-5;
				if (fabs(hitp.x - 1) < epsi) {
					FaceId = 0;
					texcoord = float2(hitp.z, hitp.y)*0.5 + float2{0.5f, 0.5f};
				}
				else if (fabs(hitp.x + 1) < epsi) {
					FaceId = 1;
					texcoord = float2(hitp.z*-1, hitp.y)*0.5 + float2{ 0.5f, 0.5f };
				}
				else if (fabs(hitp.y - 1) < epsi) {
					FaceId = 2;
					texcoord = float2(hitp.x, hitp.z)*0.5 + float2{ 0.5f, 0.5f };
				}
				else if (fabs(hitp.y + 1) < epsi) {
					FaceId = 3;
					texcoord = float2(hitp.x, hitp.z*-1)*0.5 + float2{ 0.5f, 0.5f };
				}
				else if (fabs(hitp.z - 1) < epsi) {
					FaceId = 4;
					texcoord = float2(hitp.x*-1, hitp.y)*0.5 + float2{ 0.5f, 0.5f };
				}
				else if (fabs(hitp.z + 1) < epsi) {
					FaceId = 5;
					texcoord = float2(hitp.x, hitp.y)*0.5 + float2{ 0.5f, 0.5f };
				}

				if (FaceId < 0) {
					std::cerr << "[Cubemap] : Ops, something wrong in Cubemap.texture" << std::endl;
					return redips::float3();
				}
				return faces[FaceId]->tex2d(texcoord.x, texcoord.y);
			}

			void computeSH(SamplesOnUnitSphere<BAND_WIDTH>& samples, 
						   VecXd<double, (BAND_WIDTH + 1)*(BAND_WIDTH + 1)>& channel_r, 
						   VecXd<double, (BAND_WIDTH + 1)*(BAND_WIDTH + 1)>& channel_g, 
						   VecXd<double, (BAND_WIDTH + 1)*(BAND_WIDTH + 1)>& channel_b) 
		    {
				channel_r = 0, channel_g = 0, channel_b = 0;
				for (int i = 0; i < samples.size(); ++i) {
					auto Es_i = texture(samples[i]*-1);
					auto& Ys = samples.samples_vecn[i];
					channel_r = channel_r + Ys*Es_i.x;
					channel_g = channel_g + Ys*Es_i.y;
					channel_b = channel_b + Ys*Es_i.z;
				}
				auto scale = (4 * PI / samples.size()); {
					channel_r = channel_r * scale;
					channel_g = channel_g * scale;
					channel_b = channel_b * scale;
				}
			}
		};

	};
};

