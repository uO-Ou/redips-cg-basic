#pragma once
#include "../glMeshWrapper.h"
#include "../../Cameras/phc.h"
#include "../../Geometry/sh.h"

namespace redips {
	template<int SH_BAND_WIDTH = 2>
	class SkyBox {
		class SkyBoxMesh : public glMeshWrapper {
		public:
			SkyBoxMesh(const redips::Triangles* model): glMeshWrapper(model, redips::ShaderSource()) {
				bindVaoAttribData(0, -1, -1);
			};
			SkyBoxMesh(const glMeshWrapper&) = delete;
			SkyBoxMesh(const SkyBoxMesh&) = delete;
		};

		Shader cubemapShader;
		glTexture cubemapTexture;
		Triangles* box = nullptr;
		SkyBoxMesh* mesh = nullptr;

		const char* vertex_shader = ""
			"#version 430 core\n"
			"layout(location = 0) in vec3 position;\n"
			"out vec3 TexCoords;\n"
			"uniform mat4 projection;\n"
			"uniform mat4 view;\n"
			"void main(){\n"
			"TexCoords = vec3(position.x,position.y,position.z);\n"
			"vec4 pos = projection * view * vec4(position, 1.0);\n"
			"gl_Position = pos.xyww;\n"
			"}";

		const char* fragment_shader = ""
			"#version 430 core\n"
			"out vec4 color;\n"
			"in vec3 TexCoords;\n"
			"uniform samplerCube skybox;\n"
			"void main() {\n"
			"color = vec4(texture(skybox, TexCoords).bgr,1);\n"
			"}";

		void initialize() {
			box = new redips::Triangles(float3(1, 1, 1));
			cubemapShader.CreateFromCode(vertex_shader, fragment_shader);
			mesh = new SkyBoxMesh(box);
			mesh->useShader(cubemapShader.Program);

			cubemapShader.uniformInt1("skybox", 0);
		}
	public:
		SkyBox(const char* faces[6]) {
			cubemapTexture.createCubemap(faces);
			initialize();
		}

		
		SkyBox(redips::VecXd<float, (SH_BAND_WIDTH+1)*(SH_BAND_WIDTH+1)>& light_r, 
			   redips::VecXd<float, (SH_BAND_WIDTH+1)*(SH_BAND_WIDTH+1)>& light_g, 
			   redips::VecXd<float, (SH_BAND_WIDTH+1)*(SH_BAND_WIDTH+1)>& light_b){
			
			cubemapTexture.createCubemap<SH_BAND_WIDTH>(light_r, light_g, light_b);
			
			initialize();
		}

		void render(const redips::PhC* phc) {
			glDepthFunc(GL_LEQUAL);  
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			auto view = phc->w2c()*Mat44f::translation(phc->pos());
			//auto view = phc->w2c();
			mesh->uniformMat44f("view", view.transpose().ptr());
			//mesh->uniformMat44f("view", phc->glView().ptr());
			mesh->uniformMat44f("projection",phc->glProjection().ptr());
			mesh->draw();
			glDepthFunc(GL_LESS); // set depth function back to default
		}

		void updateCubemapTexture(redips::VecXd<float, (SH_BAND_WIDTH + 1)*(SH_BAND_WIDTH + 1)>& light_r, redips::VecXd<float, (SH_BAND_WIDTH + 1)*(SH_BAND_WIDTH + 1)>& light_g, redips::VecXd<float, (SH_BAND_WIDTH + 1)*(SH_BAND_WIDTH + 1)>& light_b) {
			cubemapTexture.updateCubemap<SH_BAND_WIDTH>(light_r, light_g, light_b);
		}
	};
}
