#include <common/glfwApp.h>
#include <openglWrappers/DemoMeshes/texLightMesh.h>

auto application = redips::glfw::getInstance();

//conventional pinhole camera
redips::PhC *phc;
TexLightMesh* mesh;

//the following function show some interfaces provided by glfwApp
void movement(){
	if (application->keydown(GLFW_KEY_X))
		application->closeWindow();
	if (application->keydown(GLFW_KEY_M))
		application->acceptMouseControl(false);
	if (application->keydown(GLFW_KEY_V))
		application->changeMouSensitivity(0.9);
	if (application->keydown(GLFW_KEY_O))
		application->setWindowTitle("hello opengl");
	if (application->keydown(GLFW_KEY_F))
		application->showFps(false);
	if (application->keydown(GLFW_KEY_L))
		auto window = application->getGlfWindow();
}

//need register a display-callback function to tell glfwApp what to render
void display(){
	movement();

	mesh->uniformFloat3("lightColor",redips::float3(1,1,1));
	mesh->uniformFloat3("lightPos",phc->pos());
	mesh->uniformFloat3("cameraPos",phc->pos());
	mesh->uniformMat44f("projection",phc->glProjection().ptr());
	mesh->uniformMat44f("view", phc->glView().ptr());
	mesh->uniformMat44f("model",redips::Mat44f::eye().ptr());

	mesh->draw();
}

void initialize(){
	using namespace redips;
	mesh = new TexLightMesh(new redips::Triangles("E:/Documents/models/Garden/garden.obj"),"E:/Documents/CG/CGLib/redips/OpenglWrappers/DemoMeshes/texLight");
	//mesh->useShader("E:/Documents/CG/CGLib/redips/OpenglWrappers/DemoMeshes/texLight");  //can setup shader in constructor or use this function
	phc = new redips::PhC(70,0.7,0.1,1000);
	phc->lookAt(float3(51.166, 30.326, 31.030), float3(10, 0, 15),float3(-0.527, 0.825, -0.205));
}

int main(){
	initialize();
	application->registerDisplayCallback(display);
	application->bindCamera(phc);
	application->loop();
	return 0;
}
