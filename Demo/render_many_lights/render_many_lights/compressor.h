#pragma once
#include <iostream>
#include <algorithm>
#include <vector>
#include <direct.h>
#include <map>
using namespace std;
/*
a. load all 8100 files, sort, cnt ,write to _cnter_path_
b. merge all files in _cnter_path_, select top * ,save to _choosed_column_
c. for each file,find corresponding column
*/
typedef unsigned int uint;
typedef redips::Vec2<uint> uint2;
//按照个数降序排列
bool cmp_uint2(const uint2& a,const uint2& b){
	return a.y > b.y;
};

class Column256{
public:
	uint v[8];
	const uint operator[] (int idx) const { return v[idx]; };
	uint& operator[] (int idx) { return v[idx]; };
	int compare(const Column256& another) const{
		for (int i = 0; i < 8; i++){
			if (v[i] < another[i]) return -1;
			if (v[i] > another[i]) return 1;
		}
		return 0;
	}
	bool operator <(const Column256& another) const{
		for (int i = 0; i < 8; i++) {
			if (v[i] == another[i]) continue;
			return v[i] < another[i];
		}
		return false;
	}
	std::string tostring() const{
		std::string ret; ret.resize(256);
		for (int i = 0; i < 8; i++){
			for (int j = 0; j < 32; j++){
				ret[i * 32 + j] = '0' + ((v[i] & (1u << j))>0);
			}
		}
		return ret;
	}
	friend std::ostream& operator<<(std::ostream& os, const Column256& c256){
		for (int i = 0; i < 8; i++) os << c256[i] << " ";
		return os;
	}
};

class Column128{
public:
	uint v[4];
	uint& operator[] (int idx) { return v[idx]; };
	const uint operator[](int idx) const { return v[idx]; };
	bool operator <(const Column128& another) const{
		for (int i = 0; i < 4; i++) {
			if (v[i] == another[i]) continue;
			return v[i] < another[i];
		}
		return false;
	}
	bool operator== (const Column128& another) const {
		for (int i = 0; i < 4; i++) if (v[i] != another[i]) return false;
		return true;
	}
	int compare(const Column128& another) const {
		for (int i = 0; i < 4; i++){
			if (v[i] < another[i]) return -1;
			if (v[i] > another[i]) return 1;
		}
		return 0;
	}
	std::string tostring() const{
		std::string ret; ret.resize(128);
		for (int i = 0; i < 4; i++){
			for (int j = 0; j < 32; j++){
				ret[i * 32 + j] = '0' + ((v[i] & (1u << j))>0);
			}
		}
		return ret;
	}
	friend std::ostream& operator<<(std::ostream& os, const Column128& c128){
		os << c128[0] << " " << c128[1] << " " << c128[2] << " " << c128[3] << " ";
		return os;
	}
};


class Compressor{
public:
	uint topcnt = 1u << 23;
	uint* columns = nullptr;
	uint* indexes = nullptr;
	typedef std::pair<Column256, uint> CPair;
private:
	char strbuf[512];
	uint precision, resolution;

	std::string basepath;
	const std::string _cnter_path_ = "cnters";
	const std::string _column_file_ = "choosed_column.txt";
	const std::string _indexes_path_ = "indexes";
	const std::string _tmp_file_ = "tmp";
public:
	Compressor(std::string basepath, uint precision, int op) :basepath(basepath), precision(precision){
		resolution = 1u << precision;
		if (resolution == 128) this->basepath += "/128"; 
		else this->basepath += "/256";
		if (op < 0) return;
		if (op == 0){
			if (resolution == 128) deal128();
			else if (resolution == 256) deal256();
			puts("[compressor] : op1 finish");
		}
		if (op <= 1){
			if (resolution == 128) generateIndex128();
			else if (resolution == 256) generateIndex256();
			puts("[compressor] : op2 finish");
		}
		if (op <= 2){
			if (resolution == 128) read(4);
			else if (resolution == 256) read(8);
			puts("[compressor] : op3 finish");
		}
		puts("[compressor] : initialize finish");
	};
	~Compressor(){};
	
	void read(int cpline){
		//read column count
		sprintf(strbuf, "%s/%s", basepath.c_str(), _tmp_file_.c_str());
		ifstream fin(strbuf);
		fin >> topcnt;
		fin.close();
		
		//read columns
		puts("[compressor] : reading columns");
		columns = new unsigned int[cpline*topcnt];
		sprintf(strbuf, "%s/%s", basepath.c_str(), _column_file_.c_str());
		fin.open(strbuf);
		uint tint;
		for (int i = 0; i < topcnt; i++){
			for (int j = 0; j < cpline; j++) fin >> columns[i*cpline + j];
			fin >> tint;
		}
		fin.close();

		//read indexes
		puts("[compressor] : reading indexes");
		uint cnt_per_file = resolution * resolution;
		indexes = new unsigned int[8100*cnt_per_file];
		for (int y = 0; y < 90; y++){
			sprintf(strbuf, "%s/%s/%02d.txt", basepath.c_str(), _indexes_path_.c_str(), y);
			fin.open(strbuf);
			for (int x = 0; x < 90; x++){
				unsigned int * ptr = indexes + (y * 90 + x)*cnt_per_file;
				for (int i = 0; i < cnt_per_file; i++){
					fin >> ptr[i];
				}
			}
			fin.close();
		}
	}
	void deal128(){
		uint file_cnt_per_process = 8100;
		std::vector<uint2> counters;
		std::vector<Column128> container;
		container.resize(file_cnt_per_process*resolution*resolution);
		
		//load
		puts("[compressor] : loading 128...");
		load128(container);

		//sort
		puts("[compressor] : sorting&counting 128...");
		std::sort(container.begin(), container.end());

		//count
		int i, j;
		for (i = 0; i < container.size(); i = j){
				j = i + 1;
				while (j < container.size() && (container[j].compare(container[i]) == 0)) j++;
				counters.push_back(uint2(i, j - i));
		}
		
		//sort count
		std::sort(counters.begin(), counters.end(), cmp_uint2);
		
		//save column with count
		sprintf(strbuf, "%s/%s", basepath.c_str(),_column_file_.c_str());
		printf("[compressor] : writing columns to %s\n",strbuf);
		ofstream fout(strbuf);
		uint tint = MIN(counters.size(),topcnt);
		for (int i = 0; i < tint; i++){
			 fout << container[counters[i].x] << " " << counters[i].y << std::endl;
		}
		fout.close();
		container.clear();

		{   // save tint --> choosed column count
			sprintf(strbuf, "%s/%s", basepath.c_str(), _tmp_file_.c_str());
			ofstream fout(strbuf);
			fout << tint << endl;
			fout.close();
		}
	}
	void deal256(){
		   sprintf(strbuf, "%s/%s", basepath.c_str(), _cnter_path_.c_str());
		   _mkdir(strbuf);
		   
		   std::vector<uint2> counters;
		   std::vector<Column256> container;
		   std::vector<uint > tints;
		   uint fcnt_per_process = 8100 / 8 + 1;
		   container.resize(fcnt_per_process*resolution*resolution);
		   
		   int fid = 0;
		   for (int sid = 0;; sid += fcnt_per_process){
			   int eid = sid + fcnt_per_process; if (eid>8100) eid = 8100;
			   printf("[compressor] : 256 iter %d-%d\n",sid,eid);
			   //load
			   puts("[compressor] : loading 256...");
			   load256(sid,eid,container);
			   //sort
			   puts("[compressor] : sorting&counting 256...");
			   std::sort(container.begin(),container.end());
			   //cnt
			   int i, j;
			   for (i = 0; i < container.size(); i = j){
				   j = i + 1;
				   while (j < container.size() && (container[j].compare(container[i]) == 0)) j++;
				   counters.push_back(uint2(i, j - i));
			   }
			   //sort count
			   std::sort(counters.begin(), counters.end(), cmp_uint2);

			   //save column with count
			   sprintf(strbuf, "%s/%s/%03d.txt", basepath.c_str(), _cnter_path_.c_str(),fid++);
			   printf("[compressor] : writing columns to %s\n", strbuf);
			   int tint = MIN(counters.size(),topcnt);
			   tints.push_back(tint);
			   ofstream fout(strbuf);
			   for (int i = 0; i < tint; i++){
				   fout << container[counters[i].x] << " " << counters[i].y << std::endl;
			   }
			   fout.close();
			   counters.clear();
			   if (eid >= 8100) break;
		   }
		   container.clear();

		   //merge 8 files
		   
		   std::vector<CPair> bucketA;  bucketA.resize(topcnt*2);
		   std::vector<CPair> bucketB;  bucketB.resize(topcnt*2);
		   std::vector<CPair> bucketC;  bucketC.resize(topcnt*2);

		   int cnta, cntb; cnta = tints[0], cntb = tints[1];
		   sprintf(strbuf, "%s/%s/%03d.txt", basepath.c_str(), _cnter_path_.c_str(), 0);
		   ifstream fin(strbuf);
		   for (int i = 0; i < cnta; i++){
			   CPair tmp;
			   for (int j = 0; j < 8; j++) fin >> tmp.first.v[j]; fin >> tmp.second;
			   bucketA[i] = tmp;
		   }
		   fin.close();

		   sprintf(strbuf, "%s/%s/%03d.txt", basepath.c_str(), _cnter_path_.c_str(), 1);
		   fin.open(strbuf);
		   for (int i = 0; i < cntb; i++){
			   CPair tmp;
			   for (int j = 0; j < 8; j++) fin >> tmp.first.v[j]; fin >> tmp.second;
			   bucketB[i] = tmp;
		   }
		   fin.close();
		   
		   int aid, bid, cid;
		   std::vector<CPair> & bucketA_ptr = bucketA;
		   std::vector<CPair> & bucketB_ptr = bucketB;
		   std::vector<CPair> & bucketC_ptr = bucketC;
		   std::map<Column256,uint> column_ids_mapper;
		   for (int fid=2;;fid++){
			   aid = bid = cid = 0;
			   column_ids_mapper.clear();
			   printf("[compressor] : merging %d\n",fid);
			   for (; aid < cnta; aid++) {
				   bucketC_ptr[cid] = bucketA_ptr[aid];
				   column_ids_mapper[bucketA_ptr[aid].first] = cid++;
			   }
			   for (; bid < cntb; bid++){
				   auto iter = column_ids_mapper.find(bucketB_ptr[bid].first);
				   if (iter == column_ids_mapper.end()){
					   bucketC_ptr[cid++] = bucketB_ptr[bid];
				   }
				   else{
					   bucketC_ptr[iter->second].second += bucketB_ptr[bid].second;
				   }
			   }
			   //sort
			   std::sort(bucketC_ptr.begin(), bucketC_ptr.begin()+cid, cmp_cpair);

			   /*   old
			   while (aid < cnta&&bid < cntb){
				   if (bucketA_ptr[aid].second >= bucketB_ptr[bid].second) 
					   bucketC_ptr[cid++] = bucketA_ptr[aid++];
				   else bucketC_ptr[cid++] = bucketB_ptr[bid++];
				   if (cid >= topcnt) break;
			   }
			   if (cid < topcnt){
				   while (aid < cnta && cid<topcnt) bucketC_ptr[cid++] = bucketA_ptr[aid++];
				   while (bid < cntb && cid<topcnt) bucketC_ptr[cid++] = bucketB_ptr[bid++];
			   }
			   */
			   std::vector<CPair> & tmp_ptr = bucketA_ptr;
			   bucketA_ptr = bucketC_ptr; cnta = MIN(cid,topcnt);
			   bucketC_ptr = bucketB_ptr;
			   bucketB_ptr = tmp_ptr; cntb = tints[fid];

			   if (fid >= 8) break; 
			   sprintf(strbuf, "%s/%s/%03d.txt", basepath.c_str(), _cnter_path_.c_str(), fid);
			   fin.open(strbuf);
			   for (int i = 0; i < cntb; i++){
				   CPair tmp;
				   for (int j = 0; j < 8; j++) fin >> tmp.first.v[j]; fin >> tmp.second;
				   bucketB_ptr[i] = tmp;
			   }
			   fin.close();
		   }
		   
		   //save top*
		   sprintf(strbuf, "%s/%s", basepath.c_str(),_column_file_.c_str());
		   printf("[compressor] : writing final columns to %s\n", strbuf);
		   ofstream fout(strbuf);
		   for (int i = 0; i < cnta; i++){
			   for (int j = 0; j < 8; j++) fout << bucketA_ptr[i].first.v[j] << " "; fout << bucketA_ptr[i].second << endl;
		   }
		   fout.close();

		   sprintf(strbuf, "%s/%s", basepath.c_str(), _tmp_file_.c_str());
		   fout.open(strbuf);
		   fout << cnta << endl;
		   fout.close();
	}
	void generateIndex128(){
		const uint stride_max = 32;
		uint debug_cnt[64]; memset(debug_cnt, 0, sizeof(uint)* 64);

		sprintf(strbuf, "%s/%s", basepath.c_str(), _tmp_file_.c_str());
		ifstream fin(strbuf);
		fin >> topcnt;
	    fin.close();
		
		std::map<std::string, uint> mapper;

		puts("[compressor] : generating mapper,128");
		Column128 c128; uint tuint;
		sprintf(strbuf, "%s/%s", basepath.c_str(),_column_file_.c_str());
		fin.open(strbuf);
		for (int i = 0; i < topcnt; i++){
			fin >> c128[0] >> c128[1] >> c128[2] >> c128[3] >> tuint;
			std::string str = c128.tostring();
			mapper[str] = i;
			for (uint stride = 2; stride <= stride_max; stride <<= 1u){
				std::string tstr = getString(str,stride);
				if (!mapper.count(tstr)) mapper[tstr] = i;
			}
		}
		fin.close();

		sprintf(strbuf, "%s/%s", basepath.c_str(),_indexes_path_.c_str());
		_mkdir(strbuf);

		uint cnt_per_file = resolution * resolution;
		for (int y = 0; y < 90; y++){
			sprintf(strbuf, "%s/%s/%02d.txt", basepath.c_str(), _indexes_path_.c_str(),y);
			printf("[compressor] : generating %s 128\n",strbuf);
			ofstream fout(strbuf);
			for (int x = 0; x < 90; x++){
				sprintf(strbuf, "%s/x%02d_y%02d.txt", basepath.c_str(), x, y);
				ifstream fin(strbuf);
				for (int i = 0; i < cnt_per_file; i++){
					fin >> c128[0] >> c128[1] >> c128[2] >> c128[3];
					std::string str = c128.tostring();
					auto iter = mapper.find(str);
					if (iter != mapper.end()) {
						fout << iter->second << endl;
						debug_cnt[1]++;/////////////////////////////////////////////////
					}
					else{
						bool finded = false;
						for (uint stride = 2; stride <= stride_max; stride <<= 1u){
							iter = mapper.find(getString(str,stride));
							if (iter != mapper.end()) {
								fout << iter->second << endl;
								debug_cnt[stride]++;////////////////////////////////////////////////////////////////
								finded = true; break;
							}
						}
						if (!finded){
							fout << -1 << endl;
							puts("[compression] : something wrong ! cant find appropriate column");
						}
					}
				}
				fin.close();
			}
			fout.close();
			for (int i = 0; i < 6; i++) printf("%u %u/%u\n", 1u << i, debug_cnt[1u << i], resolution*resolution * 8100u);
		}
	}
	void generateIndex256(){
		const uint stride_max = 64;
		uint debug_cnt[256]; memset(debug_cnt, 0, sizeof(uint)* 256);

		sprintf(strbuf, "%s/%s", basepath.c_str(), _tmp_file_.c_str());
		ifstream fin(strbuf);
		fin >> topcnt;
		fin.close();

		puts("[compressor] : generating mapper,256");
		//generat mapper
		Column256 c256; uint tuint;
		sprintf(strbuf, "%s/%s", basepath.c_str(), _column_file_.c_str());
		fin.open(strbuf);
		std::map<std::string, uint> mapper;
		for (int i = 0; i < topcnt; i++){
			for (int j = 0; j < 8; j++) fin >> c256[j]; fin >> tuint;
			std::string str = c256.tostring();
			mapper[str] = i;
			for (uint stride = 2; stride <= stride_max; stride <<= 1u){
				std::string tstr = getString(str, stride);
				if (!mapper.count(tstr)) mapper[tstr] = i;
			}
		}
		fin.close();

		sprintf(strbuf, "%s/%s", basepath.c_str(), _indexes_path_.c_str()); 
		_mkdir(strbuf);

		//process per file
		uint cnt_per_file = resolution * resolution;
		
		for (int y = 0; y < 90; y++){
			sprintf(strbuf, "%s/%s/%02d.txt", basepath.c_str(), _indexes_path_.c_str(), y);
			printf("[compressor] : generating %s 256\n", strbuf);
			ofstream fout(strbuf);
			for (int x = 0; x < 90; x++){
				sprintf(strbuf, "%s/x%02d_y%02d.txt", basepath.c_str(), x, y);
				ifstream fin(strbuf);
				for (int i = 0; i < cnt_per_file; i++){
					for (int j = 0; j < 8; j++) fin >> c256[j];
					std::string str = c256.tostring();
					auto iter = mapper.find(str);
					if (iter != mapper.end()) {
						fout << iter->second << endl;
						debug_cnt[1]++;
					}
					else{
						bool finded = false;
						for (uint stride = 2; stride <= stride_max; stride <<= 1u){
							iter = mapper.find(getString(str, stride));
							if (iter != mapper.end()) {
								fout << iter->second << endl;
								debug_cnt[stride] ++;
								finded = true; break;
							}
						}
						if (!finded){
							fout << 0 << endl;
							printf("[compression] : something wrong ! cant find appropriate column\ ---- strbuf %s i %d\n\n",strbuf,i);
						}
					}
				}
				fin.close();
			}
			fout.close();
			for (int i = 0; i <= 5; i++) printf("%u %u/%u\n", 1u << i, debug_cnt[1u << i], resolution*resolution * 8100u);
		}
	}
	void load128(std::vector<Column128>& container){
		uint cnt_per_file = resolution * resolution;
		int idx = 0;
		for (int index = 0; index < 8100; index++){
			sprintf(strbuf, "%s/x%02d_y%02d.txt",basepath.c_str(), index % 90, index / 90);
			ifstream fin(strbuf);
			for (int i = 0; i < cnt_per_file; i++){
				Column128 tmp; 
				fin >> tmp[0] >> tmp[1] >> tmp[2] >> tmp[3];
				container[idx++] = tmp;
			}
			fin.close();
		}
	}
	void load256(int sid,int eid,std::vector<Column256>& container){
		uint cnt_per_file = resolution * resolution;
		int idx = 0;
		for (int index = sid; index < eid; index++){
			sprintf(strbuf, "%s/x%02d_y%02d.txt", basepath.c_str(), index % 90, index / 90);
			ifstream fin(strbuf);
			for (int i = 0; i < cnt_per_file; i++){
				Column256 tmp;
				for (int j = 0; j < 8; j++) fin >> tmp[j];
				container[idx++] = tmp;
			}
			fin.close();
		}
	}

private:
	std::string getString(const std::string& ori, int stride){
		if (stride == 1) return ori;
		std::string ret;
		int len = ori.length() / stride;
		ret.resize(len);
		for (int i = 0; i < len; i++){
			char tmp = '0';
			for (int j = 0; j < stride; j++){
				if (ori[i*stride + j] == '1') {
					tmp='1'; break;
			    }
			}
			ret[i] = tmp;
		}
		return ret;
	}
	static bool cmp_cpair(const CPair& a,const CPair &b){
		return a.second>b.second;
	}
};

