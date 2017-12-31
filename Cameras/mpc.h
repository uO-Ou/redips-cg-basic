/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.31
* Description : Multiperspective camera
*/
#pragma once
#include "Cameras/phc.h"
#include "Cameras/glc.h"
#include <vector>
namespace redips{
	const int _GLC_ANCHOR_INDEXES_[5][4] {{ 0, 1, 2, 3 }, { 4, 5, 1, 0 }, { 1, 5, 6, 2 }, { 3, 2, 6, 7 }, { 4, 0, 3, 7 }};
	class MPC : public redips::PhC{
	public:
		MPC(float vAov, float aspect/*width/height*/, float nearp, float farp,
			int2 res,
			float2 fpanel_size_in_pixel,
			float f_pos_ratio,
			float t_range_ratio,float f_range_ratio,
			float tpanel_size = 2.0f,
			std::string label = "mpc")
			:PhC(vAov, aspect, nearp, farp, label), img_center(res.x/2,res.y/2){
			setResolution(res.x,res.y);
			//focus-center pos in camera-space
			focus = redips::float3(0, 0, farp*f_pos_ratio + nearp*(1 - f_pos_ratio));
			//transition region clip panel
			near_t = focus.z*t_range_ratio + nearp*(1 - t_range_ratio);
			far_t = farp*t_range_ratio + focus.z*(1 - t_range_ratio);
			//focus region clip panel
			near_f = focus.z*f_range_ratio + near_t*(1 - f_range_ratio);
			far_f = far_t*f_range_ratio + focus.z*(1 - f_range_ratio);

			fpanel = fpanel_size_in_pixel;/// float2(resolution.x,resolution.y) * canvaSize;
			tpanel = fpanel * tpanel_size;

			//calculate anchors
			for (int i = 0; i < 8; i++) anchors[0][i] = getPos(i, nearp);
			for (int i = 0; i < 8; i++) anchors[1][i] = getPos(i, near_t);
			for (int i = 0; i < 8; i++) anchors[2][i] = getPos(i, near_f);
			for (int i = 0; i < 8; i++) anchors[3][i] = getPos(i, far_f);
			for (int i = 0; i < 8; i++) anchors[4][i] = getPos(i, far_t);
			for (int i = 0; i < 8; i++) anchors[5][i] = getPos(i, farp);

			//build camera layers
			std::vector<GLC> frontLayer; {
				frontLayer.push_back(GLC(anchors[0][4], anchors[0][5], anchors[0][6], anchors[0][7],
					                     anchors[1][4], anchors[1][5], anchors[1][6], anchors[1][7]));
				layers.push_back(frontLayer);
			}
			std::vector<GLC> middleLayer; {
				for (int l = 1; l < 4; l++){
					middleLayer.clear();
					for (int i = 0; i < 5; i++){
						middleLayer.push_back(GLC(anchors[l+0][_GLC_ANCHOR_INDEXES_[i][0]],
												  anchors[l+0][_GLC_ANCHOR_INDEXES_[i][1]],
							                      anchors[l+0][_GLC_ANCHOR_INDEXES_[i][2]],
							                      anchors[l+0][_GLC_ANCHOR_INDEXES_[i][3]],
							                      anchors[l+1][_GLC_ANCHOR_INDEXES_[i][0]],
							                      anchors[l+1][_GLC_ANCHOR_INDEXES_[i][1]],
							                      anchors[l+1][_GLC_ANCHOR_INDEXES_[i][2]],
							                      anchors[l+1][_GLC_ANCHOR_INDEXES_[i][3]]));
					}
					layers.push_back(middleLayer);
				}
				
			}
			std::vector<GLC> backLayer; {
				backLayer.push_back(GLC(anchors[4][4], anchors[4][5], anchors[4][6], anchors[4][7],
										anchors[5][4], anchors[5][5], anchors[5][6], anchors[5][7]));
				layers.push_back(backLayer);
			}

		}
		
	
		redips::float3 focus;
		redips::float2 fpanel, tpanel;
		redips::int2 img_center;
		std::vector<std::vector<GLC>> layers;
	private:
		float  near_t, near_f, far_f, far_t;
		float3 anchors[6][8];

		//get position in camera-space
		redips::float3 getPos(int id, float dist){
			float2 panel = (id / 4) ? tpanel : fpanel;
			id %= 4;

			float px = img_center.x + panel.x * ((id == 1 || id == 2) ? 0.5 : -0.5);
			float py = img_center.y + panel.y * ((id / 2) ? -0.5 : 0.5);

			return float3((px / resolution.x - 0.5f)*canvaSize.x*dist / nearp,
				(py / resolution.y - 0.5f)*canvaSize.y*dist / nearp, dist);
		}
	};
};
