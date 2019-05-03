#include "stdafx.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <sstream>

using namespace std;

using glm::vec3;
using glm::vec4;
using glm::mat4;

int t = 0; // time
static const double pi = 3.141592653589793;
const int g_width = 640;
const int g_height = 480;

// shader object
static GLuint vertShader;
static GLuint fragShader;
static GLuint gl2Program;

GLuint programId;

GLFWwindow* window;

// buffer object
static GLuint vbo[2];
static GLuint vao;
// Index Buffer Object
static GLuint ibo;

mat4 modelMat, viewMat, projectionMat, mvpMat;

std::vector<glm::vec4> meshVertices;  //頂点座標
std::vector<glm::vec3> meshNormals;  //頂点のindex
std::vector<GLushort>  meshElements; //法線vector

// shader program
GLuint crateShader()
{
	GLint compileId, linked;

	//バーテックスシェーダとフラグメントシェーダのシェーダオブジェクトを作成
	//作成したそれぞれのシェーダオブジェクトに対してソースプログラムを読み込み
	//読み込んだソースプログラムをコンパイル

	//vertex shaderのコンパイル
	GLuint vShaderId = glCreateShader(GL_VERTEX_SHADER);
	string vertexShader = R"#(
                            
attribute vec3 position;
attribute vec3 vnormal;                         
uniform mat4 projectionMatrix;
uniform mat3 NormalMatrix;
out vec3 color;
const vec3 lightDirection = normalize(vec3(10.0, 6.0, 3.0));


void main(void)
{
vec3 tnorm = normalize(NormalMatrix * vnormal);
color = vec3(dot(tnorm, lightDirection));
gl_Position =projectionMatrix * vec4(position,1);
                            
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

	//fragment shaderのコンパイル
	GLuint fShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	string fragmentShader = R"#(

out vec3 outColor;
in vec3 color;

void main(void)
{
outColor=color;
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
	// シェーダプログラムをリンク
	glLinkProgram(programId);
	glGetProgramiv(programId, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		int info_log_length = 0;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &info_log_length);
		glDeleteProgram(programId);
		return 0;
	}

	// シェーダプログラムを適用
	glUseProgram(programId);

	return programId;
}

void initialGL() 
{
	// 各shaderをリンクさせたprogram objectから該当の変数のindexを取得
	GLint attLocation = glGetAttribLocation(programId, "position");
	GLint attNormal = glGetAttribLocation(programId, "vnormal");

	// VBOの生成
	// vboは頂点情報を格納するGPU側のbuffer
	glGenBuffers(2, vbo);

	// 生成したbufferをbind、bindしたbufferにデータを登録
	// position attribute
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, meshVertices.size() * sizeof(meshVertices[0]), meshVertices.data(), GL_STATIC_DRAW);

	// normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, meshNormals.size() * sizeof(meshNormals[0]), meshNormals.data(), GL_STATIC_DRAW);

	// IBOの生成
	glGenBuffers(1, &ibo);
	// element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshElements.size() * sizeof(meshElements[0]), meshElements.data(), GL_STATIC_DRAW);

	// vaoで複数のvboを一つにまとめる
	// vaoはどれが頂点座標でどれが法線なのかわかるようになっている
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// bufferを解放
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// attribute属性を有効にする
	// (= bindしたbufferのindexを有効化)
	glEnableVertexAttribArray(attLocation);
	glEnableVertexAttribArray(attNormal);

	/* 頂点バッファオブジェクトとして buffer[0] を指定する
	   bufferをbind
	   有効化した変数に、頂点属性番号、頂点情報の要素数、データ型などを渡して通知 */
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(attLocation, 4, GL_FLOAT, false, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(attNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// 登録してきた頂点情報群をどういったindexでレンダリングするのかの指標として buffer[1] を指定する
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
}

void initialMat() 
{
	// View行列を計算
	viewMat = glm::lookAt(
		vec3(0.0, 8.0, 0.0), // ワールド空間でのカメラの座標
		vec3(0.0, 0.0, 0.0), // 見ている位置の座標
		vec3(0.0, 0.0, -1.0)  // 上方向を示す。(0,1.0,0)に設定するとy軸が上になります
	);

	// Projection行列を計算
	projectionMat = glm::perspective(
		glm::radians(45.0f), // ズームの度合い(通常90～30)
		4.0f / 3.0f,		// アスペクト比
		0.1f,		// 近くのクリッピング平面
		100.0f		// 遠くのクリッピング平面
	);

	mvpMat = projectionMat * viewMat * modelMat;
}

void loopGL() 
{
	// ゲームループ
	while (!glfwWindowShouldClose(window)) 
	{

		// 画面の初期化
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);

		modelMat = mat4(
			1, 0, 0, 0,
			0, cos(pi / 18000 * t), -sin(pi / 18000 * t), 0,
			0, sin(pi / 18000 * t), cos(pi / 18000 * t), 0,
			0, 0, 0, 1
		);

		t++;

		mvpMat = projectionMat * viewMat * modelMat;

		mat4 mv = viewMat * modelMat;
		glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(glm::mat3(mv)));

		// uniform 変数の位置を取得、programオブジェクトにデータを通知
		glUniformMatrix4fv(glGetUniformLocation(programId, "projectionMatrix"), 1, GL_FALSE, &mvpMat[0][0]);
		glUniformMatrix3fv(glGetUniformLocation(programId, "NormalMatrix"), 1, GL_FALSE, &m_3x3_inv_transp[0][0]);

		glBindVertexArray(vao);

		int size;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		// モデルの描画
		glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

		// バッファの入れ替え
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
	}
}

// obj file loader
void load_obj(const char* filename, std::vector<glm::vec4> &vertices, std::vector<glm::vec3> &normals, std::vector<GLushort> &elements)
{
	std::ifstream in(filename, std::ios::in);
	if (!in)
	{
		std::cerr << filename << "を開けませんでした" << std::endl;
		std::exit(1);
	}

	std::string line;
	while (getline(in, line))
	{
		if (line.substr(0, 2) == "v ")
		{
			std::istringstream s(line.substr(2));
			glm::vec4 v;
			s >> v.x;
			s >> v.y;
			s >> v.z;
			v.w = 1.0f;
			vertices.push_back(v);
		}
		else if (line.substr(0, 2) == "f ")
		{
			std::istringstream s(line.substr(2));
			GLushort a, b, c;
			s >> a;
			s >> b;
			s >> c;
			a--;
			b--;
			c--;
			elements.push_back(a);
			elements.push_back(b);
			elements.push_back(c);
		}
	}

	normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
	for (int i = 0; i < elements.size(); i += 3)
	{
		GLushort ia = elements[i];
		GLushort ib = elements[i + 1];
		GLushort ic = elements[i + 2];
		glm::vec3 normal = glm::normalize(glm::cross(
			glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
			glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
		normals[ia] = normals[ib] = normals[ic] = normal;
	}
}

int main()
{
	if (!glfwInit()) {
		return -1;
	}

	window = glfwCreateWindow(g_width, g_height, "Simple", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// モニタとの同期
	glfwMakeContextCurrent(window);
	glewInit();

	glfwSwapInterval(1);

	load_obj("monkey3.obj", meshVertices, meshNormals, meshElements);

	programId = crateShader();

	initialMat();

	initialGL();

	loopGL();

	glfwTerminate();

	return 0;
}