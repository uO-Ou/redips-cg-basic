#pragma once
#include "phc.h"
#include "glc.h"
namespace redips{
	class MPC : public  Camera{
	public:
		MPC(PHC& mainphc, const float3& focus, const int2& wh_res/*rect size in pixel*/, float scale, float snear, float sfar) :mainphc(&mainphc){
			this->name = "mpc";
			this->type = _mpc_;
			int tmp1[4][2] {{ -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
			int tmp2[5][4] {{ 0, 1, 2, 3 }, { 4, 5, 1, 0 }, { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 0, 3, 7 } };
			memcpy((void*)corners, (void*)tmp1, sizeof(int)* 8);
			memcpy((void*)links, (void*)tmp2, sizeof(int)* 20);

			//generate 3*2 rects (mainphc.nearp snear sfar)
			float2 wh((wh_res.x * mainphc.canvaSize.x / mainphc.resolution.x), (wh_res.y * mainphc.canvaSize.y / mainphc.resolution.y));
			float3 dir = focus - mainphc.pos();
			float zdis = dir.dot(mainphc.cameraZ);
			float3 base1 = dir * (mainphc.nearp / zdis) + mainphc.pos();
			float3 base2 = dir * (snear / zdis) + mainphc.pos();
			float3 base3 = dir * (sfar / zdis) + mainphc.pos();

			genRect(base1, wh, mn + 0);
			genRect(base1, wh*scale, mn + 4);

			genRect(base2, wh*(snear / mainphc.nearp), sn + 0);
			genRect(base2, wh*(snear / mainphc.nearp * scale), sn + 4);

			genRect(base3, wh*(sfar / mainphc.nearp), sf + 0);
			genRect(base3, wh*(sfar / mainphc.nearp*scale), sf + 4);

			build();

			//generate 8 dots on image-panel
			wh_small = float2(wh_res.x, wh_res.y);
			wh_big = float2(wh_res.x*scale, wh_res.y*scale);
			float2 proj;  mainphc.project_old(float4(focus, 1.0f), proj);
			genRect(proj, wh_small, dots + 0);
			genRect(proj, wh_big, dots + 4);

			//backup
			for (int i = 0; i < 4; i++){ nf_backup[i] = sn[i], nf_backup[i + 4] = sf[i]; }
		};
		~MPC(){};
		Ray getRay(float u, float v) {
			for (int i = 0; i < 5; i++){
				if (GeoUtil::pinQuadrangle2d(dots[links[i][0]], dots[links[i][1]], dots[links[i][2]], dots[links[i][3]], float2(u, v))){
					float3 pos0 = mainphc->c2w3() * float3((u*1.0f / mainphc->resolution.x - 0.5f)*mainphc->canvaSize.x, (v*1.0f / mainphc->resolution.y - 0.5f)*mainphc->canvaSize.y, mainphc->nearp) + mainphc->pos();
					float3 pos1 = glcs[i][0].barycentric2cart(glcs[i][0].cart2barycentric(pos0));
					float3 pos2 = glcs[i][1].barycentric2cart(glcs[i][1].cart2barycentric(pos1));
					return Ray(pos1, pos2 - pos1);
					//return Ray(float3(0.0f), float3(0.0f));
				}
			}
			return mainphc->getRay(u, v);
		}

		PHC* mainphc;
		void build(){
			//generate 5*2 glcs
			for (int i = 0; i < 5; i++){
				glcs[i][0].set(mn[links[i][0]], mn[links[i][1]], mn[links[i][2]], mn[links[i][3]], sn[links[i][0]], sn[links[i][1]], sn[links[i][2]], sn[links[i][3]]);
				glcs[i][1].set(sn[links[i][0]], sn[links[i][1]], sn[links[i][2]], sn[links[i][3]], sf[links[i][0]], sf[links[i][1]], sf[links[i][2]], sf[links[i][3]]);
			}
		}
		void reset(){ for (int i = 0; i < 4; i++) sn[i] = nf_backup[i], sf[i] = nf_backup[i + 4]; }
	public:
		int corners[4][2], links[5][4];
		float3 mn[8], sn[8], sf[8], nf_backup[8];
		float2 dots[8];

		void genRect(const float3& center, const float2 wh, float3* ret){
			for (int i = 0; i < 4; i++){
				*ret++ = center + mainphc->cameraX*corners[i][0] * wh.width() + mainphc->cameraY*corners[i][1] * wh.height();
			}
		}
		void genRect(const float2& center, const float2 wh, float2* ret){
			for (int i = 0; i < 4; i++){
				*ret++ = center + float2(corners[i][0] * wh.width(), corners[i][1] * wh.height());
			}
		};
		GLC glcs[5][2];
		float2 wh_small, wh_big;
	};
};


