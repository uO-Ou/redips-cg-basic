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
		MPC(float vAov, float aspect/*width/height*/, float _nearp, float _farp,
			int2 res,
			float2 fpanel_size_in_pixel,
			float f_pos_ratio,
			float t_range_ratio,float f_range_ratio,
			float tpanel_size = 3.0f,
			std::string label = "mpc")
			:PhC(vAov, aspect, _nearp, _farp, label), img_center(res.x / 2, res.y / 2), m_t_range_ratio(t_range_ratio), m_f_range_ratio(f_range_ratio){
			type = CAMERA_TYPE::_mpc_;
			setResolution(res.x,res.y);

			fpanel = fpanel_size_in_pixel;/// float2(resolution.x,resolution.y) * canvaSize;
			tpanel = fpanel * tpanel_size;

			for (int i = 0; i < 8; i++) anchors[0][i] = getPos(i, nearp);
			for (int i = 0; i < 8; i++) anchors[5][i] = getPos(i, farp);

			setFocus(f_pos_ratio);
		}
		void setFocus(float ratio_along_viewdir){
			//focus-center pos in camera-space
			focus = redips::float3(0, 0, farp*ratio_along_viewdir + nearp*(1 - ratio_along_viewdir));
			//transition region clip panel
			near_t = focus.z*(1 - m_t_range_ratio) + nearp*m_t_range_ratio;
			far_t = farp*m_t_range_ratio + focus.z*(1 - m_t_range_ratio);
			//focus region clip panel
			near_f = focus.z*(1 - m_f_range_ratio) + near_t*m_f_range_ratio;
			far_f = far_t*m_f_range_ratio + focus.z*(1 - m_f_range_ratio);

			buildCamera();
		}
		
		void rotate(float angle){
			angle = RAD(angle);
			for (int i = 0; i < 4; i++){
				anchors[2][i] = GeoUtil::rotateAroundAxis(float3(0, 1, 0), getPos(i, near_f), angle, focus);
				anchors[3][i] = GeoUtil::rotateAroundAxis(float3(0, 1, 0), getPos(i, far_f), angle, focus);
			}
			updateMiddleLayers();
		}
		float3 Focus(){
			return c2w3()*focus + pos();
		}
		redips::float2 fpanel, tpanel;
		redips::int2 img_center;
		std::vector<std::vector<GLC>> layers;
		float3 anchors[6][8];
	private:
		redips::float3 focus;
		float m_t_range_ratio, m_f_range_ratio;
		float  near_t, near_f, far_f, far_t;

		//get position in camera-space
		redips::float3 getPos(int id, float dist){
			float2 panel = (id / 4) ? tpanel : fpanel;
			id %= 4;

			float px = img_center.x + panel.x * ((id == 1 || id == 2) ? 0.5 : -0.5);
			float py = img_center.y + panel.y * ((id / 2) ? -0.5 : 0.5);

			return float3((px / resolution.x - 0.5f)*canvaSize.x*dist / nearp,
				(py / resolution.y - 0.5f)*canvaSize.y*dist / nearp, dist);
		}

		void updateMiddleLayers(){
			std::vector<GLC> middleLayer;
			for (int l = 1; l < 4; l++){
				middleLayer.clear();
				for (int i = 0; i < 5; i++){
					middleLayer.push_back(
						GLC(anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][0]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][1]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][2]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][3]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][0]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][1]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][2]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][3]]));
				}
				layers[l] = middleLayer;
			}
		}

		void buildCamera(){
			//calculate anchors
			for (int i = 0; i < 8; i++) anchors[1][i] = getPos(i, near_t);
			for (int i = 0; i < 8; i++) anchors[2][i] = getPos(i, near_f);
			for (int i = 0; i < 8; i++) anchors[3][i] = getPos(i, far_f);
			for (int i = 0; i < 8; i++) anchors[4][i] = getPos(i, far_t);

			//build camera layers
			layers.clear();
			std::vector<GLC> frontLayer; {
				frontLayer.push_back(GLC(anchors[0][4], anchors[0][5], anchors[0][6], anchors[0][7],
					anchors[1][4], anchors[1][5], anchors[1][6], anchors[1][7]));
				layers.push_back(frontLayer);
			}
			std::vector<GLC> middleLayer; {
				for (int l = 1; l < 4; l++){
					middleLayer.clear();
					for (int i = 0; i < 5; i++){
						middleLayer.push_back(GLC(anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][0]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][1]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][2]],
							anchors[l + 0][_GLC_ANCHOR_INDEXES_[i][3]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][0]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][1]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][2]],
							anchors[l + 1][_GLC_ANCHOR_INDEXES_[i][3]]));
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
	};
};
