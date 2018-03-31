#include <common/glfwApp.h>
#include <openglWrappers/DemoMeshes/BlinnPhongMesh.h>

//glfw setup
auto application = redips::glfw::getInstance(720, 720);

//standard pinhole camera
redips::PhC phc(60, 1.0f, 1.0f, 10000);

//load a obj and then wrap into a glMesh。
redips::BlinnPhongMesh mesh(new redips::Triangles("E:/Documents/models/maze_with_dragon.obj"));

//deal with keyboard event and others
void movement(){
	if (application->keydown(GLFW_KEY_F))
		application->showFps(false);
}

//need register a display-callback function to tell glfwApp what to render
void display(){
	mesh.uniformFloat3("lightColor", redips::float3(1, 1, 1));
	mesh.uniformFloat3("lightPos", phc.pos());
	mesh.uniformFloat3("cameraPos", phc.pos());
	mesh.uniformMat44f("projection", phc.glProjection().ptr());
	mesh.uniformMat44f("view", phc.glView().ptr());
	mesh.uniformMat44f("model", redips::Mat44f::eye().ptr());
	mesh.draw();

	movement();
}

void initialize(){
	//mesh center
	redips::float3 heart = mesh.model_ptr()->aabb_T().heart();
	//setup camera
	phc.lookAt(heart + redips::float3(0, 0, 200), heart, redips::float3(0, 1, 0));
}

int main(){
	initialize();
	application->registerDisplayCallback(display);
	application->bindCamera(&phc);
	application->loop();
	return 0;
}