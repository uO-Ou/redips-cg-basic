#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

typedef unsigned int uint;
typedef redips::Vec2<uint> uint2;

bool cmp_uint2(const uint2& a,const uint2& b){
	return a.y > b.y;
};

class Column128{
public:
	uint v[4];
	uint operator[](int idx) const { return v[idx]; };
	bool operator <(const Column128& another){
		for (int i = 0; i < 4; i++) {
			if (v[i] == another[i]) continue;
			return v[i] < another[i];
		}
		return false;
	}
	bool operator== (const Column128& another){
		for (int i = 0; i < 4; i++) if (v[i] != another[i]) return false;
		return true;
	}
	int compare(const Column128& another){
		for (int i = 0; i < 4; i++){
			if (v[i] < another[i]) return -1;
			if (v[i] > another[i]) return 1;
		}
		return 0;
	}
	void print(){
		for (int i = 0; i < 4; i++) printf("%u ", v[i]); puts("");
	}
};

class ColumnChooser{
public:
	
	uint precision;
	uint rotx, roty;
	uint resolution; 
	uint uniqueColumnCnt;
	uint chooseColumnCnt;
	std::vector<Column128> container;
	std::vector<Column128> choosedColumn;
	char strBuffer[222];

	std::string basepath;
	std::string tmpdata;
	std::string column_counter_path;
	std::string choosed_column_with_counter_path;
	std::string index_path;
public:
	ColumnChooser(std::string basepath, uint precision, uint rotx, uint roty,uint ccnt,int op):basepath(basepath),precision(precision),rotx(rotx),roty(roty){
		resolution = 1u << precision;
		chooseColumnCnt = 1u << ccnt;
		column_counter_path = basepath + "_counter_.txt";
		choosed_column_with_counter_path = basepath + "_choosed_column_with_counter_path_.txt";
		tmpdata = basepath + "_tmpdata.dat_";
		index_path = basepath + "_result_.txt";
		if (op == 0){ //load rotx*roty file, sort, then count, and write to column_counter_path, meanwhile whrite uniqueColumnCnt to tmpdata
			if (resolution == 128) {
				load128();

				//sort
				puts("[Column Chooser] : start sorting");
				std::sort(container.begin(), container.end());
				puts("[Column Chooser] : sort finish");

				//count
				int i, j;  uniqueColumnCnt = 0;
				uint columnCount = rotx*roty*resolution*resolution;
				freopen(column_counter_path.c_str(), "w", stdout);
				for (i = 0; i < columnCount;){
					for (j = i + 1; j < columnCount; j++){
						if (container[j] == container[i]) continue;
						printf("%u %u %u %u %d\n", container[i][0], container[i][1], container[i][2], container[i][3], j - i);
						uniqueColumnCnt++;
						i = j; break;
					}
					if (j == columnCount) {
						printf("%u %u %u %u %d\n", container[i][0], container[i][1], container[i][2], container[i][3], j - i);
						uniqueColumnCnt++;
						break;
					}
				}
				fclose(stdout);

				//write uniqueColumnCnt to file
				freopen(tmpdata.c_str(), "w", stdout); printf("uniqueColumnCnt %u\n", uniqueColumnCnt); fclose(stdout); freopen("CON", "w", stdout);
				puts("[Column Chooser] : counter finish");
				container.clear();

				sort128ByCounter();

				saveIndex128();
			}
		}
		else if (op == 1){
			if (resolution == 128){
				freopen(tmpdata.c_str(), "r", stdin); 
				scanf("%s %u", strBuffer, &uniqueColumnCnt);
				fclose(stdin);
				sort128ByCounter();
				saveIndex128();
			}
		}
		else if (op = 3){
			saveIndex128();
		}
		else if (op == 4){
			load128();
		}
	};
	~ColumnChooser(){
		//container.clear();
	};
	void load128(){
		container.resize(rotx*roty*resolution*resolution);
		uint idx = 0;
		Column128 tmp;
		const uint column_per_file = resolution * resolution;
		for (int y = 0; y < roty; y++) {
			printf("[Column Chooser] : loading y %d \n", y);
			for (int x = 0; x < rotx; x++) {
				sprintf(strBuffer, "%sx%02d_y%02d.txt", basepath.c_str(), x, y);
				freopen(strBuffer, "r", stdin);
				for (int z = 0; z < column_per_file; z++){
					scanf("%u %u %u %u", &tmp.v[0], &tmp.v[1], &tmp.v[2], &tmp.v[3]);
					container[idx++] = tmp;
				}
			}
		}
		fclose(stdin);
	}

	void sort128ByCounter(){
		container.resize(uniqueColumnCnt);
		std::vector<uint2> buf; buf.resize(uniqueColumnCnt);

		//load column with counter
		printf("[Column Chooser] : loading column-counter file ...");
		freopen(column_counter_path.c_str(), "r", stdin);
		Column128 tmp; uint cnt;
		for (int i = 0; i < uniqueColumnCnt; i++){
			scanf("%u %u %u %u %u", &tmp.v[0], &tmp.v[1], &tmp.v[2], &tmp.v[3], &cnt);
			buf[i] = uint2(i,cnt);
			container[i] = tmp;
		}
		fclose(stdin);
		puts("  finish");

		//sort counter then store to file
		std::sort(buf.begin(), buf.end(),cmp_uint2);
		freopen(choosed_column_with_counter_path.c_str(), "w", stdout);
		for (int i = 0; i < chooseColumnCnt; i++) {
			tmp = container[buf[i].x];
			printf("%u %u %u %u %u\n", tmp[0], tmp[1], tmp[2], tmp[3], buf[i].y);
		}
		fclose(stdout);
		freopen("CON","w",stdout);

		container.clear();
		buf.clear();
	}
	int findClosest(Column128 data){
		int low = 0, high = chooseColumnCnt - 1;
		while (low < high){
			int mid = (low + high) >> 1;
			int tag = data.compare(choosedColumn[mid]);
			if (tag == 0) return mid;
			if (tag < 0) high = mid - 1;
			low = mid + 1;
		}
		return low;
	}
	void saveIndex128(){
		choosedColumn.resize(chooseColumnCnt);
		freopen(choosed_column_with_counter_path.c_str(),"r",stdin);
		Column128 tmp; uint cnt;
		for (int i = 0; i < chooseColumnCnt; i++){
			scanf("%u %u %u %u %u", &tmp.v[0], &tmp.v[1], &tmp.v[2], &tmp.v[3], &cnt);
			choosedColumn[i] = tmp;
		}
		fclose(stdin);
		std::sort(choosedColumn.begin(),choosedColumn.end());

		load128();

		uint total = container.size();
		freopen(index_path.c_str(),"w",stdout);
		for (uint i = 0; i < total; i++){
			int index = findClosest(container[i]);//container[i]
			printf("%u ",index);
		}
		fclose(stdout);
		freopen("CON","w",stdout);
		puts("finish");
	}
};

