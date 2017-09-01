#include <scene.h>
#include <ogl/glMeshWrapper.h>
#include <ogl/shaderManager.h>
#include <ogl/voxelizer.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define WIN_WIDTH 768
#define WIN_HEIGHT 576
#define MODEL_PATH "E:/Documents/papers/如何写好论文/小论文/multi-perspect/mp_0814/scene/scenes.obj"
#define MAZE_PATH "E:/Documents/models/maze.obj"
#define GARDEN_PATH "E:/Documents/models/Garden/garden.obj"
#define SPONZA_PATH "E:/Documents/models/Sponza/sponza.obj"

extern "C" bool cudaInit();

//glfw
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void movement();
MPC *mpc;
//scene setup
Scene scene;
Triangles *model = new Triangles(SPONZA_PATH);
glMeshWrapper *glModelWrapper;
ShaderManager *shaderManager;
PHC phc(60.0f, 768.0f / 576.0f, 1.0f, 1000.0f, "mainphc");
float3 lightPos;
Voxelizer voxellizer;
float yangle = 0;
float xangle = 0;
//flags
uchar renderMode = 1;
void myInit(){
	scene.addObject(model);
	scene.updateSceneBox();
	phc.lookAt(scene.sceneBox.heart() + float3(0, 20, 50), scene.sceneBox.heart(), float3(0, 1, 0));
	lightPos = scene.sceneBox.heart() + float3(0,20,0);
	scene.addLight(Light(lightPos, float3(0.8f, 0.8f, 1.0f)));

	/*for opengl*/
	//gl wrapper , copy model data to gpu
	glModelWrapper = new glMeshWrapper( model );
	//create all shaders
	shaderManager = new ShaderManager("./shaders");
	/*for ray tracing*/
	//kdtree
	model->buildTree();
	//scene.project(phc);
	//cv::imshow("projection",scene.image);

	//other
	voxellizer.onedVoxelization(glModelWrapper, shaderManager->shader("voxelizer"), 9);
	voxellizer.multidVoxelization(90,90,8);
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
	
	Shader * plainShader = shaderManager->shader("plain");
	Shader * voxelShader = shaderManager->shader("rendervoxels");
	Shader * othorgonalShader = shaderManager->shader("orthogonal");
	while (!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		glfwPollEvents();
		movement();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (renderMode == 0){
			plainShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(plainShader->Program, "projection"), 1, GL_FALSE, phc.glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(plainShader->Program, "view"), 1, GL_FALSE, phc.glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(plainShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform3f(glGetUniformLocation(plainShader->Program, "lightColor"), 0.8f, 0.8f, 1.0f);
			glUniform3f(glGetUniformLocation(plainShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(plainShader->Program, "cameraPos"), phc.pos().x, phc.pos().y, phc.pos().z);
			glModelWrapper->draw(true, false);
		}
		else if (renderMode == 1){
			voxelShader->Use();
			glUniformMatrix4fv(glGetUniformLocation(voxelShader->Program, "projection"), 1, GL_FALSE, phc.glProjection().ptr());
			glUniformMatrix4fv(glGetUniformLocation(voxelShader->Program, "view"), 1, GL_FALSE, phc.glView().ptr());
			glUniformMatrix4fv(glGetUniformLocation(voxelShader->Program, "model"), 1, GL_FALSE, Mat44f::eye().transpose().ptr());

			glUniform3f(glGetUniformLocation(voxelShader->Program, "lightColor"), 0.8f, 0.8f, 1.0f);
			glUniform3f(glGetUniformLocation(voxelShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(voxelShader->Program, "cameraPos"), phc.pos().x, phc.pos().y, phc.pos().z);
			
			voxellizer.renderVoxels();
		}
		else if (renderMode==2){
			othorgonalShader->Use();
			glUniform3f(glGetUniformLocation(othorgonalShader->Program, "lightColor"), 0.2f, 0.8f, 0.2f);
			glUniform3f(glGetUniformLocation(othorgonalShader->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			
			glUniformMatrix4fv(glGetUniformLocation(othorgonalShader->Program, "projection"), 1, GL_FALSE, GeoUtil::glOrtho(model->aabb()).transpose().ptr());
			glUniformMatrix4fv(glGetUniformLocation(othorgonalShader->Program, "model"), 1, GL_FALSE, model->Transform().transpose().ptr());

			glModelWrapper->draw(true,false);
		}
		
		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}

bool keys[1024];
bool keysPressed[1024];
// Moves/alters the camera positions based on user input
void movement(){
	if (keys[GLFW_KEY_W]){
		phc.translate(phc.cameraZ*-0.2);
		return;
	}
	if (keys[GLFW_KEY_S]){
		phc.translate(phc.cameraZ*0.2);
		return;
	}
	if (keys[GLFW_KEY_A]){
		phc.translate(phc.cameraX*-0.2);
		return;
	}
	if (keys[GLFW_KEY_D]){
		phc.translate(phc.cameraX*0.2);
		return;
	}
	if (keys[GLFW_KEY_Y]){
		phc.translate(phc.cameraY*0.2);
		return;
	}
	if (keys[GLFW_KEY_H]){
		phc.translate(phc.cameraY*-0.2);
		return;
	}
	if (keys[GLFW_KEY_R]){
		scene.raytracing(phc);
		cv::imshow("raytracing", scene.image);
		return;
	}
	if (keys[GLFW_KEY_0]){
		renderMode = 0;
		return;
	}
	if (keys[GLFW_KEY_1]){
		renderMode = 1;
		return;
	}
	if (keys[GLFW_KEY_2]){
		renderMode = 2;
		return;
	}
}
GLfloat lastX = 900, lastY = 300;
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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
	phc.cameraX = rot * float3(1, 0, 0);
	phc.cameraY = rot * float3(0, 1, 0);
	phc.cameraZ = rot * float3(0, 0, 1);
	phc.updateExtrinsic();
	//camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	//camera.ProcessMouseScroll(yoffset);
	phc.zoom(yoffset*0.1);
}

/*
transform feedback:
1.
GLuint feedback;
glGenTransformFeedbacks(1,&feedback);
glBindTransformFeedback(GL_TRANSFORM_FEEDBACK,feedback);
GLuint feedbackBuf;
glGenBuffers(1,&feedbackBuf);
glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER,feedbackBuf);
glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,model->faceCnt*3*size*sizeof(float),NULL,GL_DYNAMIC_COPY);
glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,feedbackBuf);
static const char* const vars[] = {"pos"};
glTransformFeedbackVaryings(plainShader->Program,1,vars,GL_INTERLEAVED_ATTRIBS);
glLinkProgram(plainShader->Program);
2.
//glEnable(GL_RASTERIZER_DISCARD);
//glBeginTransformFeedback(GL_TRIANGLES);
绘制函数
//glEndTransformFeedback();
3.拷贝回来
//glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, model->faceCnt * 3 * size * sizeof(float), lbuffer);
*/
