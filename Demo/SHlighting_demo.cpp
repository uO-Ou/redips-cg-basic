#include <common/glfwApp.h>
#include <OpenglWrappers/glHelper.h>
#include <OpenglWrappers/DemoMeshes/Skybox.h>
#include <OpenglWrappers/DemoMeshes/SHLightingMesh.h>

//#define MODEL_PATH "E:/Documents/models/spheres/sphere0.obj"
#define MODEL_PATH "E:/Documents/models/dragon.obj"

//window size
const redips::int2 Winsize{ 960,960 };

//glfw setup
auto application = redips::glfw::getInstance(Winsize.x, Winsize.y);

//screen capture
auto screenCapture = redips::glScreenCapture::getInstance(Winsize);

//skybox
const char* skybox_faces[] = {
	"D:/Documents/resources/skyboxes/pure/black.jpg",
	"D:/Documents/resources/skyboxes/pure/blue.jpg",
	"D:/Documents/resources/skyboxes/pure/black.jpg",
	"D:/Documents/resources/skyboxes/pure/red.jpg",
	"D:/Documents/resources/skyboxes/pure/black.jpg",
	"D:/Documents/resources/skyboxes/pure/green.jpg"
};
auto skybox = redips::SkyBox(skybox_faces);

//standard pinhole camera
redips::PhC phc(60, 1.0f, 0.1f, 10000);

//load a obj and then wrap into a glMesh¡£
redips::SphericalHarmonic::SHLightingMesh * shmesh = nullptr;

//mesh center
redips::float3 heart = redips::float3{ 0,0,0 };

//keyboard event
void movement() {
	if (application->keydown(GLFW_KEY_M)) {
		screenCapture->capture("capture.bmp", 0);
	}
}

//need register a display-callback function to tell glfwApp what to render
void display() {
	using namespace redips;
	
	shmesh->uniformMat44f("model", shmesh->model_ptr()->Transform().transpose().ptr());
	shmesh->uniformMat44f("projection_view", phc.glProjectionView().ptr());
	shmesh->draw();

	skybox.render(&phc);

	movement();
}

void initialize() {
	using namespace redips;

	//setup camera
	phc.lookAt(heart + float3(0, 0, 20), heart, float3(0, 1, 0));
	phc.setResolution(Winsize.x, Winsize.y);

	//generate samples
	SphericalHarmonic::SamplesOnUnitSphere samples(500);

	//calculate light
	SphericalHarmonic::Cubemap cubemap(skybox_faces);
	SphericalHarmonic::float9 light_r, light_g, light_b;
	cubemap.computeSH(samples, light_r, light_g, light_b);

	//warp mesh
	shmesh = new redips::SphericalHarmonic::SHLightingMesh(new Triangles(MODEL_PATH), samples);
	shmesh ->littedBy(light_r, light_g, light_b);

	//transform mesh
	const_cast<Triangles*>(shmesh->model_ptr())->setTransform(
		Mat44f::scale(float3(60,60,60))*
		//Mat44f::rotatey(RAD(90))*
		Mat44f::translation(shmesh->model_ptr()->aabb_R().heart()*-1)
	);
}

int main() {
	initialize();
	application->registerDisplayCallback(display);
	application->bindCamera(&phc);
	application->loop();
	return 0;
}