#include "stdafx.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using glm::vec3;
using glm::vec4;
using glm::mat4;

/*
** シェーダオブジェクト
*/
static GLuint vertShader;
static GLuint fragShader;
static GLuint gl2Program;
static const double pi = 3.141592653589793;

const int g_width = 640;
const int g_height = 480;

/*
** attribute 変数 position の頂点バッファオブジェクト
*/
static GLuint buffer[3];

GLuint crateShader()
{
	GLint compileId,linked;

	//バーテックスシェーダとフラグメントシェーダのシェーダオブジェクトを作成
	//作成したそれぞれのシェーダオブジェクトに対してソースプログラムを読み込み
	//読み込んだソースプログラムをコンパイル

	//バーテックスシェーダのコンパイル
	GLuint vShaderId = glCreateShader(GL_VERTEX_SHADER);
	string vertexShader = R"#(
                            attribute vec3 position;
                            attribute vec4 color;
                            out vec4 vcolor;
                            uniform mat4 projectionMatrix;
                            void main(void){
                                gl_Position =projectionMatrix * vec4(position, 1.0);
                            }
                            )#";
	const char* vs = vertexShader.c_str();
	glShaderSource(vShaderId, 1, &vs, NULL);
	glCompileShader(vShaderId);
	glGetShaderiv(vShaderId, GL_COMPILE_STATUS, &compileId);

	
	//check for errors
	if (!compileId)
	{
		// ログを取得
		if (compileId == GL_FALSE) {
			fprintf(stderr, "Compile error in vertex shader.\n");
			//exit(1);
			abort();
		}

		glDeleteShader(vShaderId);
		return false;
	}
	
	//フラグメントシェーダのコンパイル
	GLuint fShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	string fragmentShader = R"#(
out vec4 outColor;
in vec4 vcolor;
void main(void){
       outColor=vec4(1.0);

    }
                            
                            )#";
	const char* fs = fragmentShader.c_str();
	glShaderSource(fShaderId, 1, &fs, NULL);
	glCompileShader(fShaderId);
	glGetShaderiv(fShaderId, GL_COMPILE_STATUS, &compileId);

	//check for errors
	if (!compileId)
	{
		// ログを取得
		if (compileId == GL_FALSE) {
			fprintf(stderr, "Compile error in vertex shader.\n");
			//exit(1);
			abort();
		}

		glDeleteShader(fShaderId);
		return false;
	}

	//プログラムオブジェクトの作成
	GLuint programId = glCreateProgram();
	//プログラムオブジェクトに対してシェーダオブジェクトを登録
	glAttachShader(programId, vShaderId);
	glAttachShader(programId, fShaderId);

	// リンク
	//シェーダプログラムをリンク
	glLinkProgram(programId);
	glGetProgramiv(programId, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &info_log_length);
		glDeleteProgram(programId);
		return 0;
	}

	//シェーダプログラムを適用
	glUseProgram(programId);

	return programId;
}

int main()
{
	if (!glfwInit()) {
		return -1;
	}

	GLFWwindow* window = glfwCreateWindow(g_width, g_height, "Simple", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// モニタとの同期
	glfwMakeContextCurrent(window);
	glewInit();

	glfwSwapInterval(1);

	GLuint programId = crateShader();

	mat4 modelMat,viewMat, projectionMat;

	// View行列を計算
	viewMat = glm::lookAt(
		vec3(3.0, 4.0, 5.0), // ワールド空間でのカメラの座標
		vec3(0.0, 0.0, 0.0), // 見ている位置の座標
		vec3(0.0, 0.0, 1.0)  // 上方向を示す。(0,1.0,0)に設定するとy軸が上になります
	);

	// Projection行列を計算
	projectionMat = glm::perspective(
		glm::radians(45.0f), // ズームの度合い(通常90～30)
		4.0f / 3.0f,		// アスペクト比
		0.1f,		// 近くのクリッピング平面
		100.0f		// 遠くのクリッピング平面
	);

	mat4 mvpMat = projectionMat * viewMat;

	// VBOの生成
	glGenBuffers(2, buffer);
	
	float points[] = 
	{
	0,0,0,  //v0
	1,0,0,  //v1
	1,1,0,  //v2
	0,1,0,  //v3

	0,0,-1, //v4
	1,0,-1, //v5
	1,1,-1, //v6
	0,1,-1, //v7
	};

	/* 頂点バッファオブジェクトに８頂点分のメモリ領域を確保する */
	glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);

	//頂点番号を知らせる
	GLuint index[] = 
	{
	0,1,2,3,
	1,5,6,2,
	5,4,7,6,
	4,0,3,7,
	4,5,1,0,
	3,2,6,7
	};  //インデックスの合計は24

	float colors[] = 
	{
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 1.0f, 1.0f
	};

	/* 頂点バッファオブジェクトに１２稜線分のメモリ領域を確保する */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW);

	// 頂点色をバッファオブジェクトに転送
	glBindBuffer(GL_ARRAY_BUFFER, buffer[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colors) , colors, GL_STATIC_DRAW);

	/* 頂点バッファオブジェクトのメモリをプログラムのメモリ空間から切り離す */
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	/* 頂点バッファオブジェクトを解放する */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	/* 頂点バッファオブジェクトのメモリをプログラムのメモリ空間から切り離す */
	glUnmapBuffer(GL_ARRAY_BUFFER);

	/* 頂点バッファオブジェクトを解放する */
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// ゲームループ
	while (!glfwWindowShouldClose(window)) {

		// 画面の初期化
		glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);

		/* uniform 変数 projectionMatrix に行列を設定する */
		glUniformMatrix4fv(glGetUniformLocation(programId, "projectionMatrix"), 1, GL_FALSE, &mvpMat[0][0]);

		/* attribute 変数 position の index を 0 に指定する */
		glBindAttribLocation(programId, 0, "position");

		// attribute属性を有効にする
		glEnableVertexAttribArray(0);
		//glEnableVertexAttribArray(1);

		/* 頂点バッファオブジェクトとして buffer[0] を指定する */
		glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);

		// attribute属性を登録
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0);

		/* 頂点バッファオブジェクトの指標として buffer[1] を指定する */
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		//glBindAttribLocation(programId, 2, "color");
		//glEnableVertexAttribArray(2);
		//glBindBuffer(GL_ARRAY_BUFFER, buffer[2]);
		//glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glPolygonMode(GL_FRONT, GL_FILL);

		// モデルの描画
		glDrawElements(GL_QUADS, 24, GL_UNSIGNED_INT, 0);

		// バッファの入れ替え
		glfwSwapBuffers(window);

		/* 頂点バッファオブジェクトを解放する */
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Poll for and process events
		glfwPollEvents();           // (i,j)の位置のピクセルを「マンデルブロ集合でない色」で塗りつぶして
	}


	glfwTerminate();

	return 0;
}