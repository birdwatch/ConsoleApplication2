#include "stdafx.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

/*
** シェーダオブジェクト
*/
static GLuint vertShader;
static GLuint fragShader;
static GLuint gl2Program;
static const double pi = 3.141592653589793;

GLfloat t=1;

float size = 4.0;                                // 描く領域の一辺の長さ
int pixel = 100;                             // 描く領域の一辺のピクセル数

GLfloat projectionMatrix[16] = {
	cos(pi / 6 * t), sin(pi / 2 * t),0,0,
	-sin(pi  /6 * t),cos( pi / 2* t),0,0,
	0,0,1,0,
	0,0,0,1
};

const int g_width = 640;
const int g_height = 480;

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
                            uniform mat4 projectionMatrix;
                            void main(void){
                                gl_Position =vec4(position, 1.0);
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
			exit(1);
		}

		glDeleteShader(vShaderId);
		return false;
	}
	
	//フラグメントシェーダのコンパイル
	GLuint fShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	string fragmentShader = R"#(
    varying vec4 vColor;
    void main(void){
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
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
			exit(1);
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

	// 頂点データ
	float vertex_position[] = {
		1.0, 1.0,
		-1.0, 1.0,
		-1.0, -1.0,
		1.0, -1.0
	};

	// VBOの生成
	GLuint vbo;
	glGenBuffers(1, &vbo);

	// バッファをバインドする
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// バッファにデータをセット
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertex_position, GL_STATIC_DRAW);
	// ゲームループ
	while (!glfwWindowShouldClose(window)) {

		// 画面の初期化
		glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);

		/* uniform 変数 projectionMatrix に行列を設定する */
		//glUniformMatrix4fv(glGetUniformLocation(programId, "projectionMatrix"), 1, GL_FALSE, projectionMatrix);

		// 何番目のattribute変数か
		int attLocation = glGetAttribLocation(programId, "position");

		// attribute属性を有効にする
		glEnableVertexAttribArray(attLocation);

		// attribute属性を登録
		glVertexAttribPointer(attLocation, 2, GL_FLOAT, false, 0, 0);

		// モデルの描画
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// バッファの入れ替え
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();           // (i,j)の位置のピクセルを「マンデルブロ集合でない色」で塗りつぶして
	}


	glfwTerminate();

	return 0;
}