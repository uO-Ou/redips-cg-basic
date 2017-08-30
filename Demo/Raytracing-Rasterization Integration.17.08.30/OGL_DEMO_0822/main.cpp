#include <scene.h>
#include <ogl/glMeshWrapper.h>
#include <ogl/shaderManager.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define WIN_WIDTH 768
#define WIN_HEIGHT 576
#define MAZE_PATH "E:/Documents/models/maze.obj"

//glfw
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void movement();

//scene setup
Scene scene;
Triangles *maze = new Triangles(MAZE_PATH);
glMeshWrapper *terrianWrapper;
ShaderManager *shaderManager;
PHC phc(60.0f,768.0f/576.0f,1.0f,100.0f,"mainphc");
float3 lightPos;

void myInit(){
	scene.addObject(maze);
	scene.updateSceneBox();
	phc.lookAt(scene.sceneBox.heart()+float3(0,20,50),scene.sceneBox.heart(),float3(0,1,0));
	//lightPos = scene.sceneBox.heart() + float3(0,20,0);
	lightPos = phc.pos();
	scene.addLight(Light(lightPos, float3(0.0f, 0.0f, 1.0f)));

	/*for opengl*/
	//gl wrapper , copy model data to gpu
	terrianWrapper = new glMeshWrapper(*(maze->mesh));
	//create all shaders
	shaderManager = new ShaderManager("./shaders");
	/*for ray tracing*/
	//kdtree
	maze->buildTree();
	//scene.project(phc);
	//cv::imshow("projection",scene.image);
}
int main(){
	// Init GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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

	myInit();

	Mat44f model = Mat44f::eye();
	Shader * plain = shaderManager->shader("plain");
	while (!glfwWindowShouldClose(window)){
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		glfwPollEvents();
		movement();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		plain->Use();
		glUniformMatrix4fv(glGetUniformLocation(plain->Program, "projection"), 1, GL_FALSE, phc.glProjection().ptr());
		glUniformMatrix4fv(glGetUniformLocation(plain->Program, "view"), 1, GL_FALSE, phc.glView().ptr());
		glUniformMatrix4fv(glGetUniformLocation(plain->Program, "model"), 1, GL_FALSE, model.transpose().ptr());

		glUniform3f(glGetUniformLocation(plain->Program, "lightColor"), 0.2f, 0.8f, 0.2f);
		//glUniform3f(glGetUniformLocation(plain->Program, "lightPos"), phc.pos().x, phc.pos().y, phc.pos().z);
		glUniform3f(glGetUniformLocation(plain->Program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(plain->Program, "cameraPos"),phc.pos().x,phc.pos().y,phc.pos().z);


		for (int i = 0; i < maze->mesh->groups.size(); i++){
			glBindVertexArray(terrianWrapper->vaos[i]);
			glDrawArrays(GL_TRIANGLES,0, 3 * maze->mesh->groups[i].faceCnt);
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

	//camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	//camera.ProcessMouseScroll(yoffset);
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
glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,maze->faceCnt*3*size*sizeof(float),NULL,GL_DYNAMIC_COPY);
glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,feedbackBuf);
static const char* const vars[] = {"pos"};
glTransformFeedbackVaryings(plain->Program,1,vars,GL_INTERLEAVED_ATTRIBS);
glLinkProgram(plain->Program);
2.
//glEnable(GL_RASTERIZER_DISCARD);
//glBeginTransformFeedback(GL_TRIANGLES);
draw
//glEndTransformFeedback();
3.copy
//glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, maze->faceCnt * 3 * size * sizeof(float), lbuffer);
*/
