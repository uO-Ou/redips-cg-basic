#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <scene.h>
#include <ogl/demoEffects/texLightMesh.h>
#include <ogl/demoEffects/lightMesh.h>
#include "voxelizer.h"
#include "ManySimpleObj.h"
#include "modelBoxMesh.h"

#include "compressor.h"
#include "autophc.h"
#include "propmanager.h"
#include "GBuffer.h"
#include "gBufferMesh.h"
#include "OptixBlocker.h"
#include "blocker.h"
//孟学长
#include "FrameTimer.h"

//#define CHECK_GL_ERROR(s) {if(glGetError()!=GL_NO_ERROR){printf("glError %s\n",(s));exit(-1);};}
#define WIN_WIDTH 512u
#define WIN_HEIGHT 512u

//cuda
extern "C" bool cudaInit();

//glfw
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void movement();

Scene scene;
float3 lightPos;           //场景中灯的位置
PHC* phc;

float yangle = 0;
float xangle = 0;
uchar renderMode = 1;

//properties
PropManager props("./sponza.prop");
//objs
Particles *lights;
Triangles *sphere;   //灯的模型
Triangles *model,*chassisModel;
Triangles *voxelBox;    //一个体素
Triangles *modelBox;  //模型的包围盒
//mesh wrapper
Voxelizer *voxellizer;   //用于体素化,只画三角形,不需要法线、贴图
gBufferMesh * gbufferMesh;// 渲染到gbuffer
LightMesh *litMesh;         //光照
TexLightMesh *texMesh;  //光照+纹理
ManySimpleObj *litSpheresMesh;  //画sphere0,只画三角形
ManySimpleObj *litVoxelsMesh;     //画所有的体素
ModelBoxMesh *boxMesh;            //画模型的包围盒方框
//functor
ShaderManager *shaderManager;
GBuffer *gbuffer;
Compressor * compressor;
OptixBlocker * optixBlocker;
Blocker * ourBlocker;

//switches
int frameCnter = 0;
bool pause = false;
bool enableMouse = false;

//for voxelization
uint onedv_x = 5243%90;
uint onedv_y = 5243 / 90;
//for box lines
uint rotatex = 0u;
uint rotatey = 0u;
GLuint linesVao,linesVbo=0;
//函数声明
uint getDir(float3 ray);
std::string toString(unsigned int data);
int cnt1(unsigned int data);
void updateLines(uint rx, uint ry);
void renderLights();
void saveImage();
void renderIntoGbuffer(bool draw = false);
///////////////////////////////delete
float3 debug_start = float3(7.353200, -14.688486, 10.172593) + float3(-1.000000, 0.000000, 0.000000) * 0.6;
float3 debug_end = float3(-0.274864, -6.722020, 6.713580);      //灯

void debug(uint rx, uint ry){
	float3 CENTER = model->aabb().heart();
	Mat33f rotate = Mat33f::tilt(RAD(rx*180.0f / 90))*Mat33f::pan(RAD(ry*180.0f / 90));
	float3 dim = voxellizer->getDim(rotate, true);

	char str[222]; sprintf(str, "%s/128/x%02u_y%02u.txt", props.getMdVResultPath().c_str(), rx, ry);
	freopen(str, "r", stdin);
	uint tmpd;
	vector<float3> dots;
	int totalCnt = 0;
	for (uint xx = 0; xx < 128; xx++) for (uint yy = 0; yy < 128; yy++) {
		float x = xx / 128.0f - 0.5f;
		float y = yy / 128.0f - 0.5f;
		for (uint zz = 0; zz < 4; zz++){
			scanf("%u", &tmpd);
			int tpos = 0;
			while (tmpd){
				if (tmpd & 1u){
					totalCnt++;
					float z = ((zz << 5) + tpos) / 128.0f - 0.5f;
					float3 offset = dim * float3(x, y, z);
					if (xx == 82 && yy == 56) {
						dots.push_back(CENTER + rotate.x()*offset.x + rotate.y()*offset.y + rotate.z()*offset.z);
					}

				}
				tmpd >>= 1;
				tpos++;
			}
		}
	}
	fclose(stdin);
	litVoxelsMesh->setPositions(&dots[0].x, dots.size());
	printf("[debug] : total %d\n", totalCnt);
}
//////////////////////////////////////
GLuint quardVao=0, quardVbo=0;
GLuint ssbo_4_shadowFactor = 0;
void rendWithManyLights(){
	Shader * shader = (*shaderManager)["renderWithShadow"];
	if (quardVao == 0){
		float data[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
		glGenBuffers(1,&quardVbo);
		glBindBuffer(GL_ARRAY_BUFFER,quardVbo);
		glBufferData(GL_ARRAY_BUFFER,sizeof(data),&data,GL_STATIC_DRAW);
		glGenVertexArrays(1,&quardVao);
		glBindVertexArray(quardVao);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,NULL);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		if (props.getAlgorithm() == 1){
			ssbo_4_shadowFactor = optixBlocker->smvbo();
		}
		else{
			ssbo_4_shadowFactor = ourBlocker->smvbo();
		}
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_4_shadowFactor);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		GLuint blockIndex = glGetProgramResourceIndex(shader->Program, GL_SHADER_STORAGE_BLOCK, "shadow_factor");
		glShaderStorageBlockBinding(shader->Program, blockIndex, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_4_shadowFactor);
	}

	renderIntoGbuffer();
	//get shadow factor
	if (props.getAlgorithm() == 1){
		 optixBlocker->launch();
	}
	else{
		ourBlocker->launch2();
	}

	shader->Use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (gbuffer->getTexture(GBuffer::_albedo_spec_)).texId);
	glUniform1i(glGetUniformLocation(shader->Program, "color_tex"), 0);

	glUniform1ui(glGetUniformLocation(shader->Program, "width"), WIN_WIDTH);
	glUniform1ui(glGetUniformLocation(shader->Program, "height"), WIN_HEIGHT);
	glUniform1ui(glGetUniformLocation(shader->Program, "lightCnt"), unsigned int(lights->spheres.size()));

	glBindVertexArray(quardVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	//render lights
	gbuffer->copyZBuffer();
	renderLights();
}

//计算可见性
void compare(){
	if (!props.cameraAuto()){
		cout << "[compareResult] camera must set to auto" << endl; return;
	}
	if (!props.isMdVoxelizationOn()&&!props.useCompressedResult()){
		cout << "error! : isMdVoxelizationOn() == false & dont use compressed result" << endl; return;
	}
	int frameTicker = props.getFrameRange().x;
	while (frameTicker<=props.getFrameRange().y){ //delete
		FrameTimer::instance()->setConstFrame(frameTicker);
		((AutoPhc*)phc)->update();
		frameTicker++;

		renderIntoGbuffer();
		optixBlocker->launch();
		ourBlocker->launch1(optixBlocker->visvbo());
	}
}

void myInit(){
	if ((props.getVoxelResolution() >= 8||!props.isMdVoxelizationOn()) && props.getCompressOp() < 0){
		puts("[myinit] : ERROR! (props.getVoxelResolution() >= 8||!props.isMdVoxelizationOn()) && props.getCompressOp() < 0"); exit(0);
	}
	//props.print();
	FrameTimer::instance()->clear(0);
	frameCnter = props.getFrameRange().x;
	//加载模型
	sphere = new Triangles(props.getSpherePath().c_str());
	model = new Triangles(props.getModelPath().c_str());
	if (props.drawChassis()) chassisModel = new Triangles(props.getModelWithChassisPath().c_str());
	else chassisModel = model;
	lights = new Particles(props.getStaticLightsPath().c_str());
	//设置相机
	if (props.cameraAuto()){
		const float4* param = props.getPhcIntrinsic();
		phc = new AutoPhc(param->x, param->y, param->z, param->w, props.getMovingPhcPath().c_str());
		enableMouse = false;
	}
	else {
		const float4* param1 = props.getPhcIntrinsic();
		phc = new PHC(param1->x, param1->y, param1->z, param1->w);
		const Mat33f* param2 = props.getPhcExtrinsic();
		phc->lookAt(param2->x(), param2->y(), param2->z());
		enableMouse = true;
		//phc->lookAt(model->aabb().heart() + float3(0, 0, 20), model->aabb().heart(),float3(0,1,0));
	}
	phc->setResolution(WIN_WIDTH, WIN_HEIGHT);
    /*for ray tracing*/
	scene.addObject(model);
	scene.updateSceneBox();
	lightPos = scene.sceneBox.heart() + float3(0, 20, 0);
	scene.addLight(Light(lightPos, float3(1.0f, 1.0f, 1.0f)));

	model->buildTree();

	/*for opengl*/
	//加载所有的shader
	shaderManager = new ShaderManager("./shaders");
	//初始化小灯
	litSpheresMesh = new ManySimpleObj(sphere);
	{
		float3 * tmp = new float3[lights->spheres.size()];
		float3 offset = sphere->aabb().heart() * -1.0f;
		for (int i = 0; i < lights->spheres.size(); i++) tmp[i] = lights->spheres[i] + offset;
		litSpheresMesh->setPositions((float*)tmp, lights->spheres.size());
		delete tmp;
	}
	//包围盒
	BOX box = model->aabb();
	modelBox = new Triangles(box.dim(), box.heart());
	boxMesh = new ModelBoxMesh(modelBox);
	//初始化模型相关的mesh
	voxellizer = new Voxelizer(model);
	texMesh = new TexLightMesh(*voxellizer);
	litMesh = new LightMesh(*voxellizer);
	if (props.drawChassis()){
		gbufferMesh = new gBufferMesh(chassisModel);
	}
	else gbufferMesh = new gBufferMesh(*voxellizer);

	//初始化gbuffer
	gbuffer = new GBuffer(int2(WIN_WIDTH, WIN_HEIGHT));
	
	//体素化部分
	voxellizer->setBoxType(props.getBoxType());
	voxellizer->saveFragments((*shaderManager)["voxelizer"], props.getVFragsResolution());                   //保存片元
	{    //初始化体素格子
		Mat33f rotate = Mat33f::tilt(RAD(onedv_x * 180.0f / 90))*Mat33f::pan(RAD(onedv_y * 180.0f / 90));
		float3 dim = voxellizer->getDim(rotate,props.getBoxType());
		voxelBox = new Triangles(dim*(1.0f / (1u<<props.getVoxelResolution())), rotate);
		litVoxelsMesh = new ManySimpleObj(voxelBox);
		
		voxellizer->onedVoxelization(onedv_x, onedv_y, 90, 90, props.getVoxelResolution());                    //一遍体素化
		litVoxelsMesh->setPositions(voxellizer->voxelvbo, sizeof(float) * 3, voxellizer->VOXEL_CNT);
	}
	voxellizer->multidVoxelization(90, 90, props.getVoxelResolution(),props.saveMdvResult()?props.getMdVResultPath():"", props.isMdVoxelizationOn());  //多遍体素化
	CHECK_GL_ERROR("voxellizer");

	//压缩的部分
	compressor = new Compressor(props.getMdVResultPath(),props.getVoxelResolution(),props.getCompressOp());

	//optix初始化
	optixBlocker = new OptixBlocker(WIN_WIDTH, WIN_HEIGHT, props.getModelPath().c_str(),*gbuffer,lights);

    //ourmethod初始化
	if (props.getCompressOp() >= 0){      //采用压缩后的结果
		ourBlocker = new Blocker((gbuffer->getTexture(GBuffer::_position_)).texId, (gbuffer->getTexture(GBuffer::_normal_)).texId,
			                                    lights->spheres.size(), lights->ptr(),voxellizer->rmats[0].ptr(), (float*)(&(voxellizer->radius[0])),
												compressor->topcnt, compressor->columns, compressor->indexes,
												WIN_WIDTH, WIN_HEIGHT, 90u,props.getVoxelResolution(), model->aabb().heart());
	}
	else{                                                  //采用直接体素化的结果
		ourBlocker = new Blocker((gbuffer->getTexture(GBuffer::_position_)).texId, (gbuffer->getTexture(GBuffer::_normal_)).texId,
			                                    lights->spheres.size(), lights->ptr(), voxellizer->rmats[0].ptr(), (float*)(&(voxellizer->radius[0])),
												voxellizer->result,
												WIN_WIDTH, WIN_HEIGHT, 90u, props.getVoxelResolution(), model->aabb().heart());
	}
	puts("[myinit] : finish");

	if (props.getOperation() == 0){
		compare();
	}
	renderMode = 5;
}

int main(){
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Many Lights", nullptr, nullptr); // Windowed
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize GLEW to setup the OpenGL Function pointers
	glewExperimental = GL_TRUE;
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

	// Setup some OpenGL options
	//glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
	
	cudaInit();

	myInit();
	
	Shader * lightShader = (*shaderManager)["light"];
	Shader * texLightShader = (*shaderManager)["texLight"];
	Shader * modelBoxShader = (*shaderManager)["modelBox"];
	Shader * manySobjShader = (*shaderManager)["manySimpleObj"];
	Shader * lineShader = (*shaderManager)["drawLine"];

	while (!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//F_UPDATE();
		FrameTimer::instance()->setConstFrame(frameCnter);
		if ((!pause) && (props.cameraAuto())){
			((AutoPhc*)phc)->update();
			frameCnter++; 
			if (frameCnter > props.getFrameRange().y) frameCnter = props.getFrameRange().x;
		}
		
		// Check and call events
		glfwPollEvents();
		movement();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(false){   //画lines
			updateLines(rotatex, rotatey);
			lineShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(lineShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(lineShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(lineShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glBindVertexArray(linesVao);
			glUniform3f(glGetUniformLocation(lineShader->Program, "lineColor"), 0.9f, 0.1f, 0.1f);
			glDrawArrays(GL_LINES, 0, 10);
			glUniform3f(glGetUniformLocation(lineShader->Program, "lineColor"), 0.1f, 0.9f, 0.9f);
			glDrawArrays(GL_LINES, 10, 26);
			rotatex++; if (rotatex % 90 == 0){
				rotatey = (rotatey + 1) % 90;
				rotatex = 0;
			}
		}
		if(true){   //画灯
			renderLights();
		}
		if(false){   //画box
			modelBoxShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(modelBoxShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(modelBoxShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			
			glUniform3f(glGetUniformLocation(modelBoxShader->Program,"lineColor"),0.6f,0.6f,0.1f);
			boxMesh->draw(*modelBoxShader);
		}
		if (renderMode == 1){
			texLightShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(texLightShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(texLightShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(texLightShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform3f(glGetUniformLocation(texLightShader->Program, "lightColor"), 0.6f, 0.6f, 0.8f);
			glUniform3f(glGetUniformLocation(texLightShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(texLightShader->Program, "cameraPos"), phc->pos().x, phc->pos().y, phc->pos().z);
			texMesh->draw(*texLightShader);
		}
		else if (renderMode == 2){
			manySobjShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "model"), 1, GL_FALSE, Mat44f::scale(1.0f).transpose().ptr());

			glUniform3f(glGetUniformLocation(manySobjShader->Program, "lightColor"), 0.6f, 0.6f, 0.8f);
			glUniform3f(glGetUniformLocation(manySobjShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(manySobjShader->Program, "cameraPos"), phc->pos().x, phc->pos().y, phc->pos().z);
			glUniform1i(glGetUniformLocation(manySobjShader->Program, "useLight"), 0);
			litVoxelsMesh->draw(*manySobjShader);
		}
		else if (renderMode==3){
			lightShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(lightShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(lightShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(lightShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform3f(glGetUniformLocation(lightShader->Program, "lightColor"), 0.6f, 0.6f, 0.8f);
			glUniform3f(glGetUniformLocation(lightShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(lightShader->Program, "cameraPos"), phc->pos().x, phc->pos().y, phc->pos().z);
			litMesh->draw(*lightShader);
		}
		else if (renderMode==4){
			renderIntoGbuffer(true);
		}
		else if (renderMode == 5){
			rendWithManyLights();
		}
		glfwSwapBuffers(window);
	}
	glfwTerminate();

	return 0;
}

//call backs
bool keys[1024];
bool keysPressed[1024];
// Moves/alters the camera positions based on user input
void movement(){
	if (keys[GLFW_KEY_V]){
		compare();
		return;
	}
	if (keys[GLFW_KEY_M]){
		enableMouse = !enableMouse;
		return;
	}
	if (keys[GLFW_KEY_N]){
		saveImage();
		return;
	}
	if (keys[GLFW_KEY_W]){
		phc->translate(phc->cameraZ*-0.2);
		return;
	}
	if (keys[GLFW_KEY_S]){
		phc->translate(phc->cameraZ*0.2);
		return;
	}
	if (keys[GLFW_KEY_A]){
		phc->translate(phc->cameraX*-0.2);
		return;
	}
	if (keys[GLFW_KEY_D]){
		phc->translate(phc->cameraX*0.2);
		return;
	}
	if (keys[GLFW_KEY_Y]){
		phc->translate(phc->cameraY*0.2);
		return;
	}
	if (keys[GLFW_KEY_H]){
		phc->translate(phc->cameraY*-0.2);
		return;
	}
	if (keys[GLFW_KEY_R]){
		scene.raytracing(*phc);
		cv::imshow("raytracing", scene.image);
		return;
	}
	for (int i = 0; i < 10; i++){
		if (keys[GLFW_KEY_0 + i]){
			renderMode = i; return ;
		}
	}
	
	if (keys[GLFW_KEY_SPACE]){
		pause = !pause;
		return;
	}
}
GLfloat lastX = 256, lastY = 256;
bool firstMouse = true;
// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode){
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key <= 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
			keysPressed[key] = false;
		}
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos){
	if (!enableMouse) return;
	if (firstMouse){
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	xangle -= xoffset*0.2;
	yangle -= yoffset*0.2;
	Mat33f rot = Mat33f::pan(RAD(xangle)) * Mat33f::tilt(RAD(yangle));
	phc->cameraX = rot * float3(1, 0, 0);
	phc->cameraY = rot * float3(0, 1, 0);
	phc->cameraZ = rot * float3(0, 0, 1);
	phc->updateExtrinsic();
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	phc->zoom(yoffset*0.1);
}

//获取光线对应的rotID
uint getDir(float3 ray){
	uint xdir, ydir;
	ray = ray.unit();
	float cosx = sqrt(ray.x*ray.x + ray.z*ray.z);
	if (cosx < 1e-6){ xdir = 90 / 2, ydir = 0; }
	else{
		if (ray.x>0) cosx = -cosx;
		xdir = unsigned int(acos(cosx) * 57.29578f / 180.0f * 90 + 0.5f);
		ydir = unsigned int(acos(ray.z / cosx) * 57.29578f / 180.0f * 90 + 0.5f);
		xdir = CLAMP(xdir, 0, 89);
		ydir = CLAMP(ydir, 0, 89);
	}
	return ydir * 90 + xdir;
}

//传化成01串
std::string toString(unsigned int data){
	std::string ret; ret.resize(32);
	for (int i = 0; i < 32; i++){
		if (data & 1u) ret[i] = '1'; else ret[i] = '0';
		data >>= 1;
	}
	return ret;
}

//计算1的个数
int cnt1(unsigned int data){
	int cnter = 0;
	while (data){
		if (data & 1u) cnter++;
		data >>= 1u;
	}
	return cnter;
}

//生成(rx,ty)包围盒的线
void updateLines(uint rx, uint ry){
	//////////////////////delete
	uint rid = getDir(debug_end - debug_start);
	rx = rid % 90; ry = rid / 90;
	////////////////////////////////////////

	Mat33f rotate = Mat33f::tilt(RAD(rx*180.0f / 90))*Mat33f::pan(RAD(ry*180.0f / 90));
	float3 dim = voxellizer->getDim(rotate, true);
	float3 dim2 = dim*-0.5f;
	float3 center = model->aabb().heart();
	float3 base = center + rotate.x()*dim2.x + rotate.y()*dim2.y + rotate.z()*dim2.z;

	//generate lines
	const int ids[24] = { 0, 1, 5, 4, 6, 7, 3, 2, 4, 0, 0, 2, 4, 6, 5, 7, 1, 3, 2, 6, 7, 3, 1, 5 };
	float3 axises[26];
	axises[0] = center + rotate.z()*dim2.z;
	axises[1] = center + rotate.z()*-dim2.z;

	/*****************delete*******************/
	axises[0] = debug_start;//像素
	axises[1] = debug_end;//灯
	/*****************delete*******************/

	for (int i = 0; i < 24; i++){
		float3 tdim = dim * float3::bits(ids[i]);
		axises[i + 2] = base + rotate.x()*tdim.x + rotate.y()*tdim.y + rotate.z()*tdim.z;
	}

	if (linesVbo == 0){
		glGenBuffers(1, &linesVbo);
		glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)* 3 * 26, &axises[0].x, GL_STATIC_DRAW);
		glGenVertexArrays(1, &linesVao);
		glBindVertexArray(linesVao);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
	}
	{
		glBindBuffer(GL_ARRAY_BUFFER, linesVbo);
		void* mappedbuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
		for (int i = 0; i < 26; i++){ ((float3*)mappedbuffer)[i] = axises[i]; }
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
}

//保存当前帧图像
void saveImage(){
	GLint eReadType, eReadFormat;
	unsigned char* bytes = new unsigned char[WIN_WIDTH*WIN_HEIGHT * 4];
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &eReadFormat);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &eReadType);

	glReadPixels(0, 0, WIN_WIDTH, WIN_HEIGHT, eReadFormat, eReadType, bytes);

	const char* fileName = (props.getName()+"_"+props.getResolutionStr()+"_截图.png").c_str();
	if (redips::FImage::saveImage(bytes, WIN_WIDTH, WIN_HEIGHT, 4, fileName)){
		printf("[FreeImage] : save picture [%s] success !\n", fileName);
	}
	else{
		printf("[FreeImage] : save picture [%s] failed !\n", fileName);
	}
	delete[]bytes;
}

//画灯
void renderLights(){
	Shader * manySobjShader = (*shaderManager)["manySimpleObj"];
	manySobjShader->Use();
	glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
	glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
	glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

	glUniform1i(glGetUniformLocation(manySobjShader->Program, "useLight"), 0);
	litSpheresMesh->draw(*manySobjShader);
}

//渲染到GBuffer
void renderIntoGbuffer(bool draw){
	gbuffer->bind4Writing();
	Shader* shader = (*shaderManager)["gbuffers"];
	shader->Use();
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
	glUniformMatrix4fv(glGetUniformLocation(shader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());
	gbufferMesh->draw(*shader);
	gbuffer->unbind();

	if (draw) gbuffer->render(GBuffer::_albedo_spec_);
}




