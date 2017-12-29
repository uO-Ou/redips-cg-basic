#include <Common/glfwApp.h>
#include <Common/rayTracer.h>
#include <OpenglWrappers/glHelper.h>

#define WINWIDTH 720
#define WINHEIGHT 720

//glfw setup
auto application = redips::glfw::getInstance(WINWIDTH, WINHEIGHT);

//raytracer
auto rayTracer = new redips::RayTracer(WINWIDTH, WINHEIGHT);

//create a imageRender to render rayTracer's result
auto imageRender = redips::glImageRender::getInstance(redips::int2(rayTracer->imgwidth, rayTracer->imgheight), rayTracer->imgbpp);

//standard pinhole camera
redips::PhC phc(60, 1.0f, 1.0f, 10000);

//load obj model
redips::Triangles model("E:/Documents/models/tajmahal.obj");

void movement(){}

void display(){
	rayTracer->updateLight(0, redips::Light(phc.pos() + redips::float3(0, 0, 10000), redips::float3(1, 1, 1)));
	rayTracer->render(phc);
	imageRender->render(rayTracer->imgbuf);
	movement();
}

void initialize(){
	//build a kd-tree to accelerate ray-scene intersection
	model.buildTree();
	//add object to raytracer
	rayTracer->addObject(&model);
	//setup light in rayTracer
	rayTracer->addLight(redips::Light(phc.pos(), redips::float3(1, 1, 1)));
	//set up camera
	phc.lookAt(model.aabb_T().heart() + redips::float3(0, 0, 200), model.aabb_T().heart(), redips::float3(0, 1, 0));
	phc.setResolution(WINWIDTH, WINHEIGHT);
    //disable mouse control
	application->acceptMouseControl(false);
	//register display function
	application->registerDisplayCallback(display);
}

int main(){
	initialize();
	application->bindCamera(&phc);
	application->loop();
	return 0;
}