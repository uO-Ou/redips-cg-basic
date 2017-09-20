#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <scene.h>
#include <ogl/demoEffects/texLightMesh.h>
#include <ogl/demoEffects/lightMesh.h>
#include "voxelizer.h"
#include "ManySimpleObj.h"
#include "modelBoxMesh.h"
#include "Compare.h"
#include "ColumnChooser.h"
#include "movingphc.h"
#include "compare256.h"
#include "PropManager.h"

#define WIN_WIDTH 512
#define WIN_HEIGHT 512

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

PropManager props("./sponza.prop");
//objs
Particles *lights;
Triangles *sphere0 = new Triangles("E:/Documents/models/sphere0.obj");     //灯的模型
Triangles *model;
Triangles *voxelBox;    //一个体素
Triangles *modelBox;  //模型的包围盒
//mesh wrapper
Voxelizer *voxellizer;   //用于体素化,只画三角形,不需要法线、贴图
LightMesh *litMesh;         //光照
TexLightMesh *texMesh;  //光照+纹理
ManySimpleObj *litSpheresMesh;  //画sphere0,只画三角形
ManySimpleObj *litVoxelsMesh;     //画所有的体素
ModelBoxMesh *boxMesh;            //画模型的包围盒方框
//functor
ShaderManager *shaderManager;
Compare *vcmp; 
void startCmp(){
	double rb = 0.0;
	double rs = 0.0;
	//freopen("debug.txt","w",stdout); 
	for (int xx = 222; xx < 223/*phc.resolution.x*/; xx++) for (int yy = 0; yy < phc->resolution.y; yy++){
		//{	int xx = 2, yy = 468;
		printf("pixel %d,%d : ", xx, yy);
		float3 ppos = phc->pixelWPos(xx, yy);
		HitRecord record;
		int ourCnt, rtCnt, sameCnt;
		ourCnt = rtCnt = sameCnt = 0;
		for (int i = 0; i < lights->spheres.size(); i++){
		//{ int i = 34;
			float3 lpos = lights->spheres[i];
			bool our = vcmp->check(ppos, lpos);

			float3 dir = lpos - ppos;
			float dist = dir.length();
			Ray ray(ppos, lpos - ppos);

			bool rt = model->intersect(ray, record);
			if (rt&&record.distance > dist) rt = false;

			//printf(">%d:our %s, raytracing %s  >>  %s\n", i, our ? "true" : "false", rt ? "true" : "false", rt == our ? "":"not same");

			if (rt == our) {
				sameCnt++;
			}
			if (our) ourCnt++;
			if (rt) rtCnt++;
			record.reset();
		}
		float big = 1.0f - sameCnt*1.0f / lights->spheres.size();
		float small = abs(ourCnt - rtCnt)*1.0f / lights->spheres.size();
		printf(" > big error %f,small error %f [%d/%d]  visible[%d/%d]]\n", big, small, sameCnt, lights->spheres.size(), lights->spheres.size()-ourCnt,lights->spheres.size()-rtCnt);
		rb += big;
		rs += small;
	}
	printf("[avg] big error %f,small error %f\n", rb / (phc->resolution.x*phc->resolution.y), rs / (phc->resolution.x*phc->resolution.y));

	//fclose(stdout); freopen("CON","w",stdout);
}

int phcTicker = 0;
bool pause = true;
bool enableMouse = true;
//for box lines
uint rotatex = 0;
uint rotatey = 0;
GLuint linesVao,linesVbo=0;
void updateLines(uint rx,uint ry){
	BOX tbox;
	float3 DIM = model->aabb().dim();
	Mat33f rotate = Mat33f::tilt(RAD(rx*180.0f / 90))*Mat33f::pan(RAD(ry*180.0f / 90));
	for (unsigned int i = 0; i < 8; i++){ tbox += (rotate * (DIM*-0.5f + (DIM*float3::bits(i)))); }
	float3 dim = tbox.dim();
	
	float3 center = model->aabb().heart();
	float3 dim2 = dim*-0.5f;
	float3 base = center+rotate.x()*dim2.x+rotate.y()*dim2.y+rotate.z()*dim2.z;
    
	//generate lines
	const int ids[24] = { 0, 1, 5, 4, 6, 7, 3, 2, 4, 0, 0, 2, 4, 6, 5, 7, 1, 3, 2, 6, 7, 3, 1, 5 };
	float3 axises[26];
	axises[0] = center + rotate.z()*dim2.z;
	axises[1] = center + rotate.z()*-dim2.z;

	/*****************delete*******************/
	//axises[0] = phc->pixelWPos(2,468);
	//axises[1] = lights->spheres[34];
	/*****************delete*******************/

	for (int i = 0; i < 24; i++){
		float3 tdim =dim * float3::bits(ids[i]);
		axises[i+2] = base + rotate.x()*tdim.x + rotate.y()*tdim.y + rotate.z()*tdim.z;
	}
	
	if (linesVbo==0){
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

void debug(uint rx, uint ry){
	float3 DIM = model->aabb().dim();
	float3 CENTER = model->aabb().heart();
	Mat33f rotate = Mat33f::tilt(RAD(rx*180.0f / 90))*Mat33f::pan(RAD(ry*180.0f / 90));
	BOX tbox; for (unsigned int i = 0; i < 8; i++){ tbox += (rotate * (DIM*-0.5f + (DIM*float3::bits(i)))); }
	float3 dim = tbox.dim();

	char str[222]; sprintf(str, "E:/Documents/papers/ppp/ppp/result/garden/128/x%02u_y%02u.txt", rx, ry);
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
					dots.push_back(CENTER + rotate.x()*offset.x + rotate.y()*offset.y + rotate.z()*offset.z);
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

void myInit(){
	//props.print();
	model = new Triangles(props.getModelPath().c_str());
	lights = new Particles(props.getStaticLightsPath().c_str());
	if (props.useMovingPhc()){
		const float4* param = props.getPhcIntrinsic();
		phc = new MovingPhc(param->x, param->y, param->z, param->w, props.getMovingPhcPath().c_str());
		((MovingPhc*)phc)->update();
	}
	else {
		const float4* param1 = props.getPhcIntrinsic();
		phc = new PHC(param1->x, param1->y, param1->z, param1->w);
		const Mat33f* param2 = props.getPhcExtrinsic();
		phc->lookAt(param2->x(), param2->y(), param2->z());
	}
	phc->setResolution(WIN_WIDTH);

	scene.addObject(model);
	scene.updateSceneBox();
	lightPos = scene.sceneBox.heart() + float3(0,20,0);
	scene.addLight(Light(lightPos, float3(1.0f, 1.0f, 1.0f)));

	/*for ray tracing*/
	model->buildTree();

	/*for opengl*/
	//加载所有的shader
	shaderManager = new ShaderManager("./shaders");
	//初始化小灯
	litSpheresMesh = new ManySimpleObj(sphere0);
	litSpheresMesh->setPositions(lights->ptr(), lights->spheres.size());
	//包围盒
	BOX box = model->aabb();
	modelBox = new Triangles(box.dim(), box.heart());
	boxMesh = new ModelBoxMesh(modelBox);
	//初始化模型相关的mesh
	voxellizer = new Voxelizer(model);
	texMesh = new TexLightMesh(*voxellizer);
	litMesh = new LightMesh(*voxellizer);
	
	//体素化部分
	//正常体素化
	voxellizer->saveFragments((*shaderManager)["voxelizer"], props.getVFragsResolution());
	{      //初始化体素格子
		   voxelBox = new Triangles(model->aabb().dim()*(1.0f / (1u << props.getVoxelResolution())));
	       litVoxelsMesh = new ManySimpleObj(voxelBox);
		   voxellizer->onedVoxelization(0,0,90,90,props.getVoxelResolution());
		   litVoxelsMesh->setPositions(voxellizer->voxelvbo,sizeof(float)*3,voxellizer->VOXEL_CNT);
		   /*{//delete
			uint tax = 6480%90, tay = 6480/90;
			float3 DIM = model->aabb().dim();
			Mat33f rotate = Mat33f::tilt(RAD(tax * 180.0f / 90))*Mat33f::pan(RAD(tay * 180.0f / 90));
			BOX tbox; for (unsigned int i = 0; i < 8; i++){ tbox += (rotate * (DIM*-0.5f + (DIM*float3::bits(i)))); }
			float3 dim = tbox.dim();
			voxelBox = new Triangles(dim*(1.0f / 128), rotate);
			litVoxelsMesh = new ManySimpleObj(voxelBox);
			debug(tax, tay);
		   }*/
	}
	if (props.isMdVoxelizationOn()){
		voxellizer->multidVoxelization(90,90,props.getVoxelResolution(),props.getMdVResultPath());
	}
	//vcmp = new Compare256(model, 90, 90, 8);
	//vcmp = new Compare(model, 90, 90, 7);
}

int main(){
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Raytracing-Rasterization Integration", nullptr, nullptr); // Windowed
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

	updateLines(6480 % 90, 6480 / 90);
	while (!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if ((!pause)&&(props.useMovingPhc())){
			phcTicker++;
			if (phcTicker >= 2){
				((MovingPhc*)phc)->update();
				phcTicker = 0;
			}
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
			manySobjShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "projection"), 1, GL_FALSE, phc->glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "view"), 1, GL_FALSE, phc->glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform1i(glGetUniformLocation(manySobjShader->Program,"useLight"),0);
			litSpheresMesh->draw(*texLightShader);
		}
		{   //画box
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
			glUniformMatrix4fv(glGetUniformLocation(manySobjShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform3f(glGetUniformLocation(manySobjShader->Program, "lightColor"), 0.6f, 0.6f, 0.8f);
			glUniform3f(glGetUniformLocation(manySobjShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(manySobjShader->Program, "cameraPos"), phc->pos().x, phc->pos().y, phc->pos().z);
			glUniform1i(glGetUniformLocation(manySobjShader->Program, "useLight"), 1);
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
	if (keys[GLFW_KEY_L]){
		startCmp();
		return;
	}
	if (keys[GLFW_KEY_M]){
		enableMouse = !enableMouse;
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