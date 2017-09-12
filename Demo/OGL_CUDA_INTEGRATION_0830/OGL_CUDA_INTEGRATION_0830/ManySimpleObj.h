#pragma once
#include <ogl/glMeshWrapper.h>
#include <geos/triangles.h>
class ManySimpleObj : public redips::glMeshWrapper{
public:
	ManySimpleObj(const redips::Triangles* model,bool setup_type = true) : glMeshWrapper(model, setup_type){
		bindVaoAttribData(0, 1, -1);
	};
	void draw(redips::Shader& shader){
		if (!isPosSetted) { puts("[ManySimpleObj]!warnnings : translations not set"); return;	}
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 3 * meshFaceCnt[i],objCnt);
		}
	}
	void setPositions(GLuint vbo,int cnt){
		this->transvbo = vbo;
		this->objCnt = cnt;
		isPosSetted = true;
		needReleaseBuf = false;

		bindTransvbo();
	}
	void setPositions(const float* poses, int cnt){
		glGenBuffers(1, &transvbo);
		glBindBuffer(GL_ARRAY_BUFFER, transvbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)* 3 * cnt, poses, GL_STATIC_DRAW);
		this->objCnt = cnt;
		isPosSetted = true;
		needReleaseBuf = true;

		bindTransvbo();
	};
	~ManySimpleObj(){
		if (needReleaseBuf) glDeleteBuffers(1, &transvbo);
	};
private:
	int objCnt;
	GLuint transvbo;
	bool needReleaseBuf = false;
	bool isPosSetted = false;
	void bindTransvbo(){
		for (int i = 0; i < meshCnt; i++){
			glBindVertexArray(vaos[i]);
			glBindBuffer(GL_ARRAY_BUFFER, transvbo);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glEnableVertexAttribArray(3);
			glVertexAttribDivisor(3, 1);
		}
	}
};

