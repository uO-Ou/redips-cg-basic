#include <Common/glfwApp.h>
#include <OpenglWrappers/glHelper.h>
#include <OpenglWrappers/DemoMeshes/BlinnPhongMesh.h>
#include <OpenglWrappers/DemoMeshes/DepthPeelingMesh.h>

#define WINWIDTH 720
#define WINHEIGHT 720
#define MAZE "E:/Documents/models/maze/maze.obj"
#define OPENGL_ON

//setup a glfw environment
auto application = redips::glfw::getInstance(WINWIDTH, WINHEIGHT);

//standard pinhole camera
redips::PhC phc(60, 1.0f, 1.0f, 1000);

//light attributes
redips::float3 lightPos, lightColor(1, 1, 1);

//load a obj model then wrap into a glMesh¡£
auto mesh = new redips::DepthPeelingBlinnPhongMesh(new redips::Triangles(MAZE));

//mesh center
redips::float3 heart = mesh->model_ptr()->aabb_T().heart();

//create a screenCapture to capture screen
auto screenCapture = redips::glScreenCapture::getInstance(redips::int2(WINWIDTH, WINHEIGHT));

/***********************************ABOUT FBO*************************************/
GLuint depthFBO;
redips::glTexture depthTexture,colorTexture;
int renderDepth = 1; float blendCoef = 0.0f;
void createFBO(){
	colorTexture.create2d(redips::int2(WINWIDTH, WINHEIGHT), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	depthTexture.create2d(redips::int2(WINWIDTH, WINHEIGHT), GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glGenFramebuffers(1, &depthFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
	glClearDepth(0.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClearDepth(1.0f);
	//glDrawBuffer(GL_NONE);
	//glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void copy2FBO(){
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, depthFBO);
	glBlitFramebuffer(0, 0, WINWIDTH, WINHEIGHT, 0, 0, WINWIDTH, WINHEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, WINWIDTH, WINHEIGHT, 0, 0, WINWIDTH, WINHEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (glGetError() != GL_NO_ERROR) puts("copy2FBO wrong");;
}
/*************************************************************************************/

//deal with keyboard event
void movement(){
	if (application->keydown(GLFW_KEY_M))
		screenCapture->capture("capture");
	for (int i = 0; i < 10; ++i){
		if (application->keydown(GLFW_KEY_0 + i))
			renderDepth = i;
	}
}

//need register a display-callback function to tell glfwApp what to render
void display(){
	using namespace redips;
	//set up uniforms
	mesh->uniformFloat3("lightColor", float3(1, 1, 1));
	mesh->uniformFloat3("lightPos", phc.pos());
	mesh->uniformFloat3("cameraPos", phc.pos());
	mesh->uniformMat44f("view", phc.glView().ptr());
	mesh->uniformMat44f("projection", phc.glProjection().ptr());
	mesh->uniformMat44f("model", Mat44f::eye().ptr());
	
	//clean fbo
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClearDepth(0.0f);
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glClearDepth(1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//depth rendering
	blendCoef = 0.0f;
	for (int i = 0; i < renderDepth; ++i){
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		mesh->uniformFloat1("blendCoef", blendCoef);
		mesh->draw();
		copy2FBO();
		glClear(GL_DEPTH_BUFFER_BIT);
		blendCoef += 0.2f;
		if (blendCoef>0.6f) blendCoef = 0.6f;
	}
	movement();
}

void initialize(){
	//create fbo
	createFBO();
	mesh->setTextures(depthTexture,colorTexture);

	//enable backface culling
	glEnable(GL_CULL_FACE);

	//set up camera
	phc.lookAt(heart + redips::float3(0, 0, 50), heart, redips::float3(0, 1, 0));
	phc.setResolution(WINWIDTH, WINHEIGHT);
}

int main(){
	initialize();
	application->registerDisplayCallback(display);
	application->bindCamera(&phc);
	application->loop();
	return 0;
}