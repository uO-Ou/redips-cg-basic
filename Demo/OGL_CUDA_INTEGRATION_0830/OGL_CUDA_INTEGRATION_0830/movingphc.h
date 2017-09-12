#pragma once
#include <camera/phc.h>
#include <vector>
#include <fstream>
#include <iostream>
using namespace std;
class MovingPhc : public redips::PHC{
public:
	MovingPhc(float hAov, float aspect/*width/height*/, float nearp, float farp,const char * file):PHC(hAov,aspect,nearp,farp,"moving phc"){
		ifstream fin(file);
		fin >> pcnt;  params.resize(pcnt);
	    redips::float3 tmp;
		for (int i = 0; i < pcnt; i++){
			for (int j = 0; j < 3; j++)  {
				fin >> tmp[0] >> tmp[1] >> tmp[2];
				params[i].setrow(tmp,j);
			}
		}
		intervals.resize(pcnt); intervals[0] = 0;
		for (int i = 1; i < pcnt; i++) {
			int tint; fin >> tint;
			intervals[i] = tint + intervals[i-1];
		}
		fin.close();
		total = intervals[pcnt - 1];
		update();
	}
	void update(){
		float rate = (ticker - intervals[curc]) * 1.0f / (intervals[curc+1]-intervals[curc]);
		float3 pos = params[curc].x()*(1 - rate) + params[curc + 1].x()*rate;
		float3 focus = params[curc].y()*(1 - rate) + params[curc + 1].y()*rate;
		float3 up = params[curc].z()*(1 - rate) + params[curc + 1].z()*rate;
		this->lookAt(pos,focus,up);
		ticker++; if (ticker >= total){
			ticker = 0; curc = 0;
		}
		if (ticker >= intervals[curc + 1]){
			curc++;
		}
	}
	~MovingPhc(){}
private:
	std::vector<redips::Mat33f> params;
	std::vector<int > intervals;
	int pcnt = 0;
	int ticker = 0;
	int curc = 0;
	int total = 0;
};