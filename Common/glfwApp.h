/*
* Author : redips redips.xin@gmail.com
* Date : 2017.12.9
* Description : encapsulated GLFW window system
*/
#pragma once
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "../Cameras/phc.h"
namespace redips{
	namespace glfw{
		static bool keys[1024];
		static bool keysPressed[1024];

		//class KeyEvent{
		//public:
		//	static void key_callback(int _key, int action){
		//		if (action != GLFW_PRESS){ _key = 0; return;}
		//		key = _key;
		//	}
		//	inline int getKey()const { return key; };
		//private:
		//	static int key;
		//};
		//int KeyEvent::key = 0;

		static GLfloat curfps = 0.0f;
		static GLfloat deltaTime = 0.0f;
		static GLfloat lastFrame = 0.0f;
		
		static GLuint win_width = 512, win_height = 512;
		
		static bool show_fps = true;
		static bool firstMouse = true;
		static bool enableMouse = true;
		static float xangle = 0, yangle = 0;  // for camera's Euler angles
		static double lastX = 256, lastY = 256, mouSensitivity = 0.05, scrollSensitivity = 0.05f, keyboardSensitivity = 0.0005f, min_depth_bound = 0.5;

		//a camera binded to current window
		static Camera *bindedCamera = nullptr;
		//call backs		
		static void(*displayCallback)() = nullptr;
		static void(*mouseCallback)(double, double) = nullptr;
		static void(*scrollCallback)(double) = nullptr;
		static void(*mouseClickCallback)(int) = nullptr;
		static char strbuf[512], windowTitle[256];

		class App{
			//KeyEvent keyRecorder;
		public:
			static App* getInstance(int width=512,int height = 512,const char* title = "redips"){
				if (instance == nullptr) instance = new App(width, height, title); return instance;
			};
			static GLFWwindow* getGlfWindow(){ return window; };
			void loop(){
				while (!glfwWindowShouldClose(window)){
					GLfloat currentFrame = glfwGetTime();
					deltaTime = currentFrame - lastFrame;
					lastFrame = currentFrame;
					curfps = 1 / deltaTime;
					if (show_fps){
						sprintf_s(strbuf, "%s - %.1f fps, %.1f ms\n", windowTitle, curfps, deltaTime * 1000);
						glfwSetWindowTitle(window, strbuf);
					}
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glfwPollEvents();
					if (bindedCamera){
						if (bindedCamera->type == CAMERA_TYPE::_phc_){
							redips::PhC* cam = ((redips::PhC*)bindedCamera);
							if (keys[GLFW_KEY_A]) cam->translate(cam->cameraX*-keyboardSensitivity);
							if (keys[GLFW_KEY_D]) cam->translate(cam->cameraX*keyboardSensitivity);
							if (keys[GLFW_KEY_H]) cam->translate(cam->cameraY*-keyboardSensitivity);
							if (keys[GLFW_KEY_Y]) cam->translate(cam->cameraY*keyboardSensitivity);
							if (keys[GLFW_KEY_W]) cam->translate(cam->cameraZ*-keyboardSensitivity);
							if (keys[GLFW_KEY_S]) cam->translate(cam->cameraZ*keyboardSensitivity);

							if (keys[GLFW_KEY_UP]) keyboardSensitivity += 0.01;
							if (keys[GLFW_KEY_DOWN]) { 
								keyboardSensitivity -= 0.01; 
								if (keyboardSensitivity < 0.02)
									keyboardSensitivity = 0.02;
							}
						}
					}

					if (displayCallback != nullptr) 	(*displayCallback)();

					glfwSwapBuffers(window);
				}
				glfwTerminate();
			}
			void registerDisplayCallback(void(*func)()){
				displayCallback = func;
			}
			void registerMouseCallback(void(*func)(double, double)){
				mouseCallback = func;
			}
			void registerMouseClickCallback(void(*func)(int)){
				mouseClickCallback = func;
			}
			void registerScrollCallback(void(*func)(double)){
				scrollCallback = func;
			}
			void bindCamera(Camera* camera = nullptr){ bindedCamera = camera; }

			void showFps(bool flag) { 
				show_fps = flag;  if (!flag) glfwSetWindowTitle(window,windowTitle);
			};
			void acceptMouseControl(bool flag) { enableMouse = flag; };
			void stretchMouSensitivity(float scale) { mouSensitivity *= scale; };
			void stretchscrollSensitivity(float scale) { scrollSensitivity *= scale; };
			void stretchKeyboardSensitivity(float scale) { 
				keyboardSensitivity *= scale; 
				if (keyboardSensitivity < 1e-4) keyboardSensitivity = 1e-3;
			};
			void setWindowTitle(const char* title){ strcpy_s(windowTitle, title); }
			bool keydown(int id){ 
				if (keys[id]) {
					keys[id] = false; return true;
				}
				return false;
			}
			//int keydown(){ return keyRecorder.getKey(); };
			void closeWindow(){ glfwSetWindowShouldClose(window, GL_TRUE); }

			unsigned int CURSOR_MODE = GLFW_CURSOR_DISABLED;
			void swapCursorMode() {
				if (CURSOR_MODE == GLFW_CURSOR_DISABLED) 
					glfwSetInputMode(window, GLFW_CURSOR, CURSOR_MODE = GLFW_CURSOR_NORMAL);
				else
					glfwSetInputMode(window, GLFW_CURSOR, CURSOR_MODE = GLFW_CURSOR_DISABLED);
			}

			void setCursorMode(int mode) {
				glfwSetInputMode(window, GLFW_CURSOR, CURSOR_MODE = mode);
			}

		private:
			static App* instance;
			static GLFWwindow* window;
			App(GLuint width, GLuint height, const char* title){
				win_width = width, win_height = height;
				strcpy_s(windowTitle,title);

				glfwInit();
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

				window = glfwCreateWindow(win_width, win_height, title, nullptr, nullptr); // Windowed
				glfwMakeContextCurrent(window);
			
				// Set the required callback functions
				glfwSetKeyCallback(window, key_callback);
				glfwSetScrollCallback(window, scroll_callback);
				glfwSetCursorPosCallback(window, mouse_callback);
				glfwSetMouseButtonCallback(window, mouse_click_callback);
				// Options
				//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				glfwSetInputMode(window, GLFW_CURSOR, CURSOR_MODE);

				// Initialize GLEW to setup the OpenGL Function pointers
				glewExperimental = GL_TRUE;
				glewInit();

				// Define the viewport dimensions
				glViewport(0, 0, win_width, win_height);
				glEnable(GL_DEPTH_TEST);
				//glClearColor(0.88f, 0.99f, 0.99f, 1.0f);
				//glClearColor(0.27f, 0.55f, 0.52f, 1.0f);
				glClearColor(0.0f,0.0f,0.0f,1.0f);
				//glClearColor(1,1,1,1);
				CHECK_GL_ERROR("opengl-environment setup failed");
			};

			//key callback
			static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode){
				if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
					glfwSetWindowShouldClose(window, GL_TRUE);
				if (key >= 0 && key <= 1024) {
					if (action == GLFW_PRESS) keys[key] = true;
					else if (action == GLFW_RELEASE)	{
						keys[key] = false;
						keysPressed[key] = false;
					}
				}
			}
			//mouse callbacks
			static void mouse_callback(GLFWwindow* window, double xpos, double ypos){
				if (firstMouse){
					lastX = xpos;
					lastY = ypos;
					firstMouse = false;
				}

				GLfloat xoffset = xpos - lastX;
				GLfloat yoffset = lastY - ypos;

				lastX = xpos;
				lastY = ypos;

				if (mouseCallback){
					mouseCallback(xpos,ypos);
				}
				else if (bindedCamera&&enableMouse){
					if (bindedCamera->type == CAMERA_TYPE::_phc_){
						mouseCallback4phc(xoffset*mouSensitivity, yoffset*mouSensitivity);
					}
				}
			}

			static void mouse_click_callback(GLFWwindow* window, int button, int action, int mods){
				if (action == GLFW_PRESS){
					if (mouseClickCallback) 
						mouseClickCallback(button);
					else{
						switch (button){
						case GLFW_MOUSE_BUTTON_LEFT:
							//puts("Mouse left button clicked!");
							break;
						case GLFW_MOUSE_BUTTON_MIDDLE:
							//puts("Mouse middle button clicked!");
							break;
						case GLFW_MOUSE_BUTTON_RIGHT:
							//puts("Mouse right button clicked!");
							break;
						default:
							return;
						}
					}
					return;
				}
			}

			static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
				if (scrollCallback){
					scrollCallback(yoffset*scrollSensitivity);
				}
				if (bindedCamera){
					if (bindedCamera->type == CAMERA_TYPE::_phc_){
						scrollCallback4phc(yoffset*scrollSensitivity);
					}
				}
			}
			//callbacks predefined for pinhole camera
			static void mouseCallback4phc(double xoffset, double yoffset){
				xangle -= xoffset;
				yangle -= yoffset;
				auto phcptr = ((redips::PhC*)bindedCamera);
				/*******************	1	********************/
				auto rot = redips::Mat33f::pan(RAD(xangle)) * redips::Mat33f::tilt(RAD(yangle));
				phcptr->cameraX = rot * redips::float3(1, 0, 0);
				phcptr->cameraY = rot * redips::float3(0, 1, 0);
				phcptr->cameraZ = rot * redips::float3(0, 0, 1);
				phcptr->updateExtrinsic();
				/***********************************************/
				/*******************	2	********************
				auto c2w = phcptr->c2w3()*redips::Mat33f::pan(RAD(xoffset)) * redips::Mat33f::tilt(RAD(-yoffset));
				phcptr->cameraX = c2w.col(0);
				phcptr->cameraY = c2w.col(1);
				phcptr->cameraZ = c2w.col(2);
				phcptr->updateExtrinsic();
				/***********************************************/
			}
			static void scrollCallback4phc(double offset){
				((redips::PhC*)bindedCamera)->zoom(offset);
			}
		};
		App* App::instance = nullptr;
		GLFWwindow* App::window = nullptr;

		static App* getInstance(int width = 512, int height = 512, const char* title = "redips"){
			return App::getInstance(width,height,title); 
		}
	}
}