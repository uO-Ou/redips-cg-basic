#pragma once
#include <cstdlib>
#include <GL/glew.h>
#include <optixu/optixpp_namespace.h>
#include <cstdio>
#include <limits>
#include <nvModel.h>
#include <nvTime.h>
#include <framebufferObject.h>
#include <string.h>
#include <limits>
#include "sutil.h"
#include "GBuffer.h"
#include <geos/particles.h>

#define CHECK_GL_ERROR(s) {if(glGetError()!=GL_NO_ERROR){printf("glError %s\n",(s));exit(-1);};}

class OptixBlocker{
public:
	OptixBlocker(int width,int height,const char* model_path,redips::GBuffer& gbuffer,const redips::Particles* lights){
		   CHECK_GL_ERROR("bad optix blocker enviroment.");
		   scr_width = width;  scr_height = height;
		   positionTex = gbuffer.getTexture(redips::GBuffer::_position_).texId;
		   normalTex = gbuffer.getTexture(redips::GBuffer::_normal_).texId;
		   
		   //lights buffer
		   glGenBuffers(1, &lightsAB);
		   glBindBuffer(GL_ARRAY_BUFFER,lightsAB);
		   glBufferData(GL_ARRAY_BUFFER,sizeof(float)*3*lights->spheres.size(),lights->ptr(),GL_STATIC_READ);
		   glBindBuffer(GL_ARRAY_BUFFER,0);
		   //shadow map //for rendering
		   glGenBuffers(1, &shadowMapAB);
		   glBindBuffer(GL_ARRAY_BUFFER, shadowMapAB);
		   glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*scr_width*scr_height, NULL, GL_DYNAMIC_COPY);
		   glBindBuffer(GL_ARRAY_BUFFER, 0);
		   //visibilities      //for compare
		   glGenBuffers(1, &visibilitiesAB);
		   glBindBuffer(GL_ARRAY_BUFFER, visibilitiesAB);
		   glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*scr_width*scr_height*((lights->spheres.size()- 1) / 32 + 1), NULL, GL_DYNAMIC_COPY);
		   glBindBuffer(GL_ARRAY_BUFFER, 0);
		   CHECK_GL_ERROR("allocate gl buffer.");

		   if (!initialize(model_path,lights->spheres.size())){
			   puts("[optix blocker] : initialize failed");
		   }
		   else {
			   puts("[optix blocker] : initialize success");
			   initialized = true;
		   }
	};
	~OptixBlocker(){
		if (modelVB) glDeleteBuffers(1,&modelVB);
		if (modelIB) glDeleteBuffers(1,&modelIB);
		if (lightsAB) glDeleteBuffers(1, &lightsAB);
		if (shadowMapAB) glDeleteBuffers(1, &shadowMapAB);
		if (visibilitiesAB) glDeleteBuffers(1, &visibilitiesAB);
	};
	
	void launch(){
		if (!initialized) return;
		printf("[optix] : launching");
		rtContext->launch(0,scr_width,scr_height);
		puts("finish");
	}
	GLuint smvbo(){ return shadowMapAB; }
	GLuint visvbo(){ return visibilitiesAB; }
private:
	bool initialized = false;
	GLuint scr_width, scr_height;
	//模型相关
	nv::Model* model;
	GLuint modelVB=0, modelIB=0;
	//纹理数据
	GLuint positionTex=0, normalTex=0;
	//gl buffer
	GLuint lightsAB=0, shadowMapAB=0, visibilitiesAB=0;
	//optix
	optix::Context        rtContext;
	optix::TextureSampler postexSampler,normtexSampler;
	optix::Buffer         rtLightsBuffer;
	optix::Buffer         rtShadowMapBuffer,rtVisiblilitiesBuffer;

	float scene_epsilon = 0.3f;

	std::string ptxpath(const char* filename){
		return std::string("./ptx/")+std::string(filename)+".ptx";
	}

	bool load(const char* model_path){
		model = new nv::Model();
		puts("[optix] : loading nvmodel ... ");
		if (!model->loadModelFromFile(model_path)) {
			std::cerr << "[optix] : ! Unable to load model '" << model_path << "'" << std::endl;
			return false;
		}
		model->removeDegeneratePrims();
		model->clearTexCoords();
		model->clearTangents();
		model->clearColors();
		model->computeNormals();
		model->compileModel();

		glGenBuffers(1, &modelVB);
		glBindBuffer(GL_ARRAY_BUFFER, modelVB);
		glBufferData(GL_ARRAY_BUFFER, model->getCompiledVertexCount()*model->getCompiledVertexSize()*sizeof(float), model->getCompiledVertices(), GL_STATIC_READ);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &modelIB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIB);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->getCompiledIndexCount()*sizeof(int), model->getCompiledIndices(), GL_STATIC_READ);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		return true;
	}

	bool initialize(const char* model_path, int lightCnt){
		//load model
		if (!load(model_path)){
			puts("[optix] : load model failed"); return false;
		}
		else puts("[optix] : load model finish");

		//init optix
		try {
			rtContext = optix::Context::create();
			rtContext->setRayTypeCount(1);
			rtContext->setEntryPointCount(1);

			rtContext["scr_width"]->setUint(scr_width);
			rtContext["light_cnt"]->setUint(lightCnt);
			rtContext["shadow_ray_type"]->setUint(0u);
			rtContext["scene_epsilon"]->setFloat(scene_epsilon);

			// Limit number of devices to 1 as this is faster for this particular sample.
			std::vector<int> enabled_devices = rtContext->getEnabledDevices();
			rtContext->setDevices(enabled_devices.begin(), enabled_devices.begin() + 1);
			
			//lightexSampler = rtContext->createTextureSamplerFromGLImage(lightsTexture.texId, RT_TARGET_GL_TEXTURE_1D);
			//lightexSampler->setWrapMode(0, RT_WRAP_REPEAT);
			//lightexSampler->setIndexingMode(RT_TEXTURE_INDEX_ARRAY_INDEX);
			//lightexSampler->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE);
			//lightexSampler->setMaxAnisotropy(1.0f);
			//lightexSampler->setFilteringModes(RT_FILTER_NEAREST, RT_FILTER_NEAREST, RT_FILTER_NONE);
			//rtContext["lights_texture"]->setTextureSampler(lightexSampler);
			
			//lights buffer        //input
			rtLightsBuffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT,lightsAB);
			rtLightsBuffer->setSize(lightCnt*3);
			rtLightsBuffer->setFormat(RT_FORMAT_FLOAT);
			rtContext["lights_buffer"]->setBuffer(rtLightsBuffer);

			//texture sampler   //input
			postexSampler = rtContext->createTextureSamplerFromGLImage(positionTex, RT_TARGET_GL_TEXTURE_2D);
			postexSampler->setWrapMode(0, RT_WRAP_REPEAT);
			postexSampler->setWrapMode(1, RT_WRAP_REPEAT);
			postexSampler->setIndexingMode(RT_TEXTURE_INDEX_ARRAY_INDEX);
			postexSampler->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE);
			postexSampler->setMaxAnisotropy(1.0f);
			postexSampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
			rtContext["position_texture"]->setTextureSampler(postexSampler);
			
			normtexSampler = rtContext->createTextureSamplerFromGLImage(normalTex, RT_TARGET_GL_TEXTURE_2D);
			normtexSampler->setWrapMode(0, RT_WRAP_REPEAT);
			normtexSampler->setWrapMode(1, RT_WRAP_REPEAT);
			normtexSampler->setIndexingMode(RT_TEXTURE_INDEX_ARRAY_INDEX);
			normtexSampler->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE);
			normtexSampler->setMaxAnisotropy(1.0f);
			normtexSampler->setFilteringModes(RT_FILTER_LINEAR, RT_FILTER_LINEAR, RT_FILTER_NONE);
			rtContext["normal_texture"]->setTextureSampler(normtexSampler);
			
			//shadow&visibility buffer  //output
			rtShadowMapBuffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT_OUTPUT,shadowMapAB);
			rtShadowMapBuffer->setSize(scr_width*scr_height);
			rtShadowMapBuffer->setFormat(RT_FORMAT_UNSIGNED_INT);
			rtContext["shadowMap_buffer"]->setBuffer(rtShadowMapBuffer);
			
			rtVisiblilitiesBuffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT_OUTPUT,visibilitiesAB);
			rtVisiblilitiesBuffer->setSize(scr_width*scr_height*((lightCnt - 1) / 32 + 1));
			rtVisiblilitiesBuffer->setFormat(RT_FORMAT_UNSIGNED_INT);
			rtContext["visibilities_buffer"]->setBuffer(rtVisiblilitiesBuffer);
    
			//program
			rtContext->setRayGenerationProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "calculate_visibilities"));    //发射光线的函数
			rtContext->setExceptionProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "exception"));                          //异常函数

			optix::Material opaque = rtContext->createMaterial();                                                                                                       //创建一个材质
			opaque->setAnyHitProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "any_hit_shadow"));                        //hit 函数

			//rt model
			optix::Geometry rtModel = rtContext->createGeometry();
			rtModel->setPrimitiveCount(model->getCompiledIndexCount() / 3);
			rtModel->setIntersectionProgram(rtContext->createProgramFromPTXFile(ptxpath("mesh"), "mesh_intersect"));                  //判断相交的函数
			rtModel->setBoundingBoxProgram(rtContext->createProgramFromPTXFile(ptxpath("mesh"), "mesh_bounds"));                 //用于创建包围盒

			int num_vertices = model->getCompiledVertexCount();
			optix::Buffer vertex_buffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT, modelVB);
			vertex_buffer->setFormat(RT_FORMAT_USER);
			vertex_buffer->setElementSize(3 * 2 * sizeof(float));
			vertex_buffer->setSize(num_vertices);
			rtModel["vertex_buffer"]->setBuffer(vertex_buffer);

			optix::Buffer index_buffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT, modelIB);
			index_buffer->setFormat(RT_FORMAT_INT3);
			index_buffer->setSize(model->getCompiledIndexCount() / 3);
			rtModel["index_buffer"]->setBuffer(index_buffer);

			//setup geometry instance
			optix::GeometryInstance instance = rtContext->createGeometryInstance();
			instance->setMaterialCount(1);
			instance->setMaterial(0, opaque);
			instance->setGeometry(rtModel);

			optix::GeometryGroup geometrygroup = rtContext->createGeometryGroup();
			geometrygroup->setChildCount(1);
			geometrygroup->setChild(0, instance);
			geometrygroup->setAcceleration(rtContext->createAcceleration("Bvh", "Bvh"));

			rtContext["shadow_casters"]->set(geometrygroup);

			rtContext->setStackSize(2048);
			rtContext->validate();
		}
		catch (optix::Exception& e) {
			sutilReportError(e.getErrorString().c_str());
			return false;
		}
		return true;
	}
};

