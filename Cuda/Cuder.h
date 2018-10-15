#pragma once
#include <map>
#include <string>
#include <vector>
#include <cuda.h>
#include <nvrtc.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cudaProfiler.h>
#include <cuda_runtime.h>
#include <helper_cuda_drvapi.h>

namespace redips{
	class Cuder{
		CUcontext context;
		std::map <std::string, CUmodule> modules;
		std::map <std::string, CUdeviceptr> devptrs;
		
		Cuder(){ 
			checkCudaErrors(cuCtxCreate(&context, 0, cuDevice)); 
		}
		void release(){
			//for (auto module : modules) delete module.second;
			for (auto dptr : devptrs) cuMemFree(dptr.second);
			devptrs.clear();
			modules.clear();
			cuCtxDestroy(context);
		}
	public:
		class ValueHolder{
		public:
			void * value = nullptr;
			bool is_string = false;
			ValueHolder(const char* str){
				value = (void*)str;
				is_string = true;
			}
			template <typename T>
			ValueHolder(const T& data){
				value = new T(data);
			}
		};

		static Cuder getInstance(){
			if (!cuda_enviroment_initialized) initialize();
			return Cuder();
		}

		//forbidden copy-constructor and assignment function
		Cuder(const Cuder&) = delete;
		Cuder& operator= (const Cuder& another) = delete;

		Cuder(Cuder&& another){
			this->context = another.context;
			another.context = nullptr;
			this->devptrs = std::map<std::string, CUdeviceptr>(std::move(another.devptrs));
			this->modules = std::map<std::string, CUmodule>(std::move(another.modules));
		}
		Cuder& operator= (Cuder&& another) {
			if (this->context == another.context) return *this;
			release();
			this->context = another.context; 
			another.context = nullptr;
			this->devptrs = std::map<std::string, CUdeviceptr>(std::move(another.devptrs));
			this->modules = std::map<std::string, CUmodule>(std::move(another.modules));
			return *this;
		}
		
		virtual ~Cuder(){ release();	};
		
	public:
		bool launch(dim3 gridDim, dim3 blockDim, size_t sharedMemSize, std::string module, std::string kernel_function, std::initializer_list<ValueHolder> params){
			//get kernel address
			if (!modules.count(module)){
				std::cerr << "[Cuder] : error: doesn't exists an module named " << module << std::endl; return false;
			}
			CUfunction kernel_addr;
			if (CUDA_SUCCESS != cuModuleGetFunction(&kernel_addr, modules[module], kernel_function.c_str())){
				std::cerr << "[Cuder] : error: doesn't exists an kernel named " << kernel_function << " in module " << module << std::endl; return false;
			}
			//setup params
			std::vector<void*> pamary;
			for (auto v : params){
				if (v.is_string){
					if (devptrs.count((const char*)(v.value))) pamary.push_back((void*)(&(devptrs[(const char*)(v.value)])));
					else{
						std::cerr << "[Cuder] : error: launch failed. doesn't exists an array named " << (const char*)(v.value) << std::endl;;
						return false;
					}
				}
				else pamary.push_back(v.value);
			}

			cudaEvent_t start, stop;
			float elapsedTime = 0.0;
			cudaEventCreate(&start);
			cudaEventCreate(&stop);
			cudaEventRecord(start, 0);

			bool result = (CUDA_SUCCESS == cuLaunchKernel(kernel_addr,/* grid dim */gridDim.x, gridDim.y, gridDim.z, /* block dim */blockDim.x, blockDim.y, blockDim.z, /* shared mem, stream */ sharedMemSize, 0, &pamary[0], /* arguments */0));
			cuCtxSynchronize();

			cudaEventRecord(stop, 0);
			cudaEventSynchronize(stop);
			cudaEventElapsedTime(&elapsedTime, start, stop);
			//std::cout << "[Cuder] : launch finish. cost " << elapsedTime << "ms" << std::endl;
			return result;
		}
		bool addModule(std::string cufile){
			if (modules.count(cufile)){
				std::cerr << "[Cuder] : error: already has an modules named " << cufile << std::endl;;
				return false;
			}

			std::string ptx = get_ptx(cufile);
			
			if (ptx.length() > 0){
				CUmodule module;
				checkCudaErrors(cuModuleLoadDataEx(&module, ptx.c_str(), 0, 0, 0));
				modules[cufile] = module;
				return true;
			}
			else{
				std::cerr << "[Cuder] : error: add module " << cufile << " failed!\n";
				return false;
			}
		}
		void applyArray(const char* name, size_t size, const void* h_ptr=nullptr){
			if (devptrs.count(name)){
				std::cerr << "[Cuder] : error: already has an array named " << name << std::endl;;
				return;
			}
			CUdeviceptr d_ptr;
			checkCudaErrors(cuMemAlloc(&d_ptr, size));
			if (h_ptr) 
				checkCudaErrors(cuMemcpyHtoD(d_ptr, h_ptr, size));
			devptrs[name] = d_ptr;
		}
		void transferArray(const char* name, size_t size, const void* h_ptr = nullptr) {
			if (!h_ptr) return;
			if (!devptrs.count(name)) {
				std::cerr << "[Cuder] : error: cannot find an array named " << name << std::endl;;
				return;
			}
			CUdeviceptr d_ptr = devptrs[name];
			checkCudaErrors(cuMemcpyHtoD(d_ptr, h_ptr, size));
		}
		void fetchArray(const char* name, size_t size, void * h_ptr){
			if (!devptrs.count(name)){
				std::cerr << "[Cuder] : error: doesn't exists an array named " << name << std::endl;;
				return;
			}
			checkCudaErrors(cuMemcpyDtoH(h_ptr, devptrs[name], size));
		}
		
	private:
		static int devID;
		static CUdevice cuDevice;
		static bool cuda_enviroment_initialized;
		static void initialize(){
			// picks the best CUDA device [with highest Gflops/s] available
			devID = gpuGetMaxGflopsDeviceIdDRV();
			checkCudaErrors(cuDeviceGet(&cuDevice, devID));
			// print device information
			{
				char name[100]; int major = 0, minor = 0;
				checkCudaErrors(cuDeviceGetName(name, 100, cuDevice));
				checkCudaErrors(cuDeviceComputeCapability(&major, &minor, cuDevice));
				printf("[Cuder] : Using CUDA Device [%d]: %s, %d.%d compute capability\n", devID, name, major, minor);
			}
			//initialize
			checkCudaErrors(cuInit(0));

			cuda_enviroment_initialized = true;
		}
		//如果是ptx文件则直接返回文件内容，如果是cu文件则编译后返回ptx
		std::string get_ptx(std::string filename){
			std::ifstream inputFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
			if (!inputFile.is_open()) {
				std::cerr << "[Cuder] : error: unable to open " << filename << " for reading!\n";
				return "";
			}

			std::streampos pos = inputFile.tellg();
			size_t inputSize = (size_t)pos;
			char * memBlock = new char[inputSize + 1];

			inputFile.seekg(0, std::ios::beg);
			inputFile.read(memBlock, inputSize);
			inputFile.close();
			memBlock[inputSize] = '\x0';

			if (filename.find(".ptx") != std::string::npos) 
				return std::string(std::move(memBlock));
			// compile
			nvrtcProgram prog;
			if (nvrtcCreateProgram(&prog, memBlock, filename.c_str(), 0, NULL, NULL) == NVRTC_SUCCESS){
				delete memBlock;
				if (nvrtcCompileProgram(prog, 0, nullptr) == NVRTC_SUCCESS){
					// dump log
					size_t logSize; 
					nvrtcGetProgramLogSize(prog, &logSize);
					if (logSize>0){
						char *log = new char[logSize + 1];
						nvrtcGetProgramLog(prog, log);
						log[logSize] = '\x0';
						std::cout << "[Cuder] : compile [" << filename << "] " << log << std::endl;
						delete(log);
					}
					else std::cout << "[Cuder] : compile [" << filename << "] finish" << std::endl;

					// fetch PTX
					size_t ptxSize;
					nvrtcGetPTXSize(prog, &ptxSize);
					char *ptx = new char[ptxSize+1];
					nvrtcGetPTX(prog, ptx);
					nvrtcDestroyProgram(&prog);
					return std::string(std::move(ptx));
				}
			}
			delete memBlock;
			return "";
		}
	};
	bool Cuder::cuda_enviroment_initialized = false;
	int Cuder::devID = 0;
	CUdevice Cuder::cuDevice = 0;
};