#include "stdafx.h"
#include "lodepng.h"
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
#include <algorithm>


using namespace std;

using glm::vec3;
using glm::vec4;
using glm::mat4;

vec3 camera_pos = vec3(0.0, 4.0, 15.0);

int t = 0; // time
static const double pi = 3.141592653589793;
const int g_width = 640;
const int g_height = 480;

// shader object
static GLuint vertShader;
static GLuint fragShader;
static GLuint gl2Program;

std::vector<GLuint> vao;
std::vector<GLuint> textureID;
std::vector<mat4> modelMat;

GLuint programId;

// 各shaderをリンクさせたprogram objectから該当の変数のindexを取得
GLint attLocation;
GLint attNormal;
GLint uvLocation;
GLint textureLocation;

GLFWwindow* window;

mat4 viewMat, projectionMat, mvpMat;

std::vector<unsigned char> image;
unsigned width, height;

// 位置
glm::vec3 position = glm::vec3(0, 0, 5);
// 水平角、-Z方向
//float horizontalAngle = 3.14f;
// 鉛直角、0、水平線を眺めている
//float verticalAngle = 0.0f;
// 初期視野
//float initialFoV = 45.0f;

float tspeed = 0.003f; // 3 units / second
float rspeed = 0.0003f;
float mouseSpeed = 0.005f;
float tx = 0, ty = 0, tz = 0;
float rx = 0, ry = 0, rz = 0;

typedef struct objData {
	std::vector<glm::vec4> meshVertices;  // Vertices position
	std::vector<glm::vec3> meshNormals;  // Normal vector
	std::vector<glm::vec2> meshUVs; // UV position
	std::vector<GLushort>  meshElements; // Index
};

int readShaderSource(GLuint shaderObj, std::string fileName)
{
	//ファイルの読み込み
	std::ifstream ifs(fileName);
	if (!ifs)
	{
		std::cout << "error" << std::endl;
		return -1;
	}

	std::string source;
	std::string line;
	while (getline(ifs, line))
	{
		source += line + "\n";
	}

	// シェーダのソースプログラムをシェーダオブジェクトへ読み込む
	const GLchar *sourcePtr = (const GLchar *)source.c_str();
	GLint length = source.length();
	glShaderSource(shaderObj, 1, &sourcePtr, &length);

	return 0;
}

// shader program
GLuint crateShader()
{
	GLint compileId, linked;

	//バーテックスシェーダとフラグメントシェーダのシェーダオブジェクトを作成
	//作成したそれぞれのシェーダオブジェクトに対してソースプログラムを読み込み
	//読み込んだソースプログラムをコンパイル

	//vertex shaderのコンパイル
	GLuint vShaderId = glCreateShader(GL_VERTEX_SHADER);
	
	if (readShaderSource(vShaderId, "vertex_shader.glsl")) return -1;

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
	if (readShaderSource(fShaderId, "fragment_shader.glsl")) return -1;
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

GLuint loadTexture(string filename)
{
	// テクスチャIDの生成
	GLuint texID;
	glGenTextures(1, &texID);

	//lodepng::load_file(image,filename);
	lodepng::decode(image, width, height, filename);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

	// テクスチャの設定
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// テクスチャのアンバインド
	glBindTexture(GL_TEXTURE_2D, 0);

	image.clear();

	return texID;
}

// obj file loader
objData load_obj(const char* filename)
{
	objData data;

	std::vector<glm::vec4> vertices;
	std::vector<glm::vec3> normals;
	std::vector<GLushort> elements;
	std::vector<glm::vec2> uvs;
	std::vector<GLushort>  UVElements;
	std::vector<GLushort>  NormalsElements;
	std::vector<glm::vec2> tuvs;
	std::vector<glm::vec3> tnormals;

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
			GLushort vertexIndex[3];
			GLushort uvIndex[3];
			GLushort normalIndex[3];

			std::replace(line.begin(), line.end(), '/', ' ');

			std::istringstream s(line.substr(2));


			GLushort a, b, c;
			s >> vertexIndex[0];

			s >> uvIndex[0];
			s >> normalIndex[0];
			s >> vertexIndex[1];
			s >> uvIndex[1];
			s >> normalIndex[1];
			s >> vertexIndex[2];
			s >> uvIndex[2];
			s >> normalIndex[2];

			vertexIndex[0]--;
			vertexIndex[1]--;
			vertexIndex[2]--;

			uvIndex[0]--;
			uvIndex[1]--;
			uvIndex[2]--;

			normalIndex[0]--;
			normalIndex[1]--;
			normalIndex[2]--;


			elements.push_back(vertexIndex[0]);
			UVElements.push_back(uvIndex[0]);
			NormalsElements.push_back(normalIndex[0]);
			elements.push_back(vertexIndex[1]);
			UVElements.push_back(uvIndex[1]);
			NormalsElements.push_back(normalIndex[1]);
			elements.push_back(vertexIndex[2]);
			UVElements.push_back(uvIndex[2]);
			NormalsElements.push_back(normalIndex[2]);

		}
		else if (line.substr(0, 2) == "vt")
		{
			std::istringstream s(line.substr(2));
			glm::vec2 uv;
			s >> uv.x;
			s >> uv.y;
			uv.y = 1.0 - uv.y;
			tuvs.push_back(uv);
		}
		else if (line.substr(0, 2) == "vn")
		{
			std::istringstream s(line.substr(2));
			glm::vec3 norm;
			s >> norm.x;
			s >> norm.y;
			s >> norm.z;
			tnormals.push_back(norm);
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

	uvs.resize(vertices.size(), glm::vec2(0.0, 0.0));
	for (int i = 0; i < elements.size(); i++)
	{
		GLushort ia = elements[i];
		// uv展開したラインのverticesはuv座標が二つあるので新しくverticesを生成する
		if (uvs[ia] != glm::vec2(0.0, 0.0) && uvs[ia] != tuvs[UVElements[i]])
		{
			vertices.push_back(vertices[ia]);
			uvs.push_back(tuvs[UVElements[i]]);
			normals.push_back(tnormals[NormalsElements[i]]);
			elements[i] = vertices.size() - 1;
		}
		else
		{
			uvs[ia] = tuvs[UVElements[i]];
		}
	}

	data.meshVertices = vertices;
	data.meshNormals = normals;
	data.meshUVs = uvs;
	data.meshElements = elements;

	return data;
}

GLuint makevao(const char* objname, string texturename) {
	objData monkey = load_obj(objname);

	// buffer objects
	GLuint vbo[3];

	// VBOの生成
	// vboは頂点情報を格納するGPU側のbuffer
	glGenBuffers(3, vbo);

	// 生成したbufferをbind、bindしたbufferにデータを登録
	// position attribute
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, monkey.meshVertices.size() * sizeof(monkey.meshVertices[0]), monkey.meshVertices.data(), GL_STATIC_DRAW);

	// normal attribute
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, monkey.meshNormals.size() * sizeof(monkey.meshNormals[0]), monkey.meshNormals.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, monkey.meshUVs.size() * sizeof(monkey.meshUVs[0]), monkey.meshUVs.data(), GL_STATIC_DRAW);

	// IBOの生成
	// Index Buffer Object
	GLuint ibo;
	glGenBuffers(1, &ibo);
	// element attribute
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, monkey.meshElements.size() * sizeof(monkey.meshElements[0]), monkey.meshElements.data(), GL_STATIC_DRAW);

	// textureIDの生成
	textureID.push_back(loadTexture(texturename));

	// vaoで複数のvboを一つにまとめる
	// vaoはどれが頂点座標でどれが法線なのかわかるようになっている
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// bufferを解放
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// attribute属性を有効にする
	// (= bindしたbufferのindexを有効化)
	glEnableVertexAttribArray(attLocation);
	glEnableVertexAttribArray(attNormal);
	glEnableVertexAttribArray(uvLocation);

	/* 頂点バッファオブジェクトとして buffer[0] を指定する
	   bufferをbind
	   有効化した変数に、頂点属性番号、頂点情報の要素数、データ型などを渡して通知 */
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glVertexAttribPointer(attLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glVertexAttribPointer(uvLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glVertexAttribPointer(attNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// 登録してきた頂点情報群をどういったindexでレンダリングするのかの指標として buffer[1] を指定する
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	// unbind vao
	glBindVertexArray(0);

	modelMat.push_back(mat4(1.0f));

	return vao;
}

glm::mat4 calculateModel(mat4 preMat) {

	mat4 modelMat=preMat;

	// 前へ動きます。
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		tz = (-1) * tspeed;
	}
	// 後ろへ動きます。
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		tz = tspeed;
	}
	// 前を向いたまま、右へ平行移動します。
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		tx = tspeed;
	}
	// 前を向いたまま、左へ平行移動します。
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		tx = (-1) * tspeed;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE
		&& glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
	{
		tz = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE
		&& glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE)
	{
		tx = 0;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		rz = rspeed;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		rz = (-1) *rspeed;
	}
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE
		&& glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		rz = 0;
	}

	modelMat = glm::scale(modelMat, glm::vec3(1.0f));
	modelMat = glm::rotate(modelMat, 0.0f, glm::vec3(1.0f, 0.0, 0.0f));
	modelMat = glm::rotate(modelMat, 0.0f, glm::vec3(0.0f, 1.0, 0.0f));
	modelMat = glm::rotate(modelMat, rz, glm::vec3(0.0f, 0.0, 1.0f));
	modelMat = glm::translate(modelMat, glm::vec3(tx, ty, tz));

	//mvpMat = projectionMat * viewMat * modelMat;

	return modelMat;
}

void loopGL()
{
	//GLuint* pixels = new GLuint[sizeof(float) * width * height * 3];
	//glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_INT, pixels);
	int size;

	// View行列を計算
	viewMat = glm::lookAt(
		camera_pos, // ワールド空間でのカメラの座標
		vec3(0.0, 1.0, 0.0), // 見ている位置の座標
		vec3(0.0, 1.0, 0.0)  // 上方向を示す。(0,1.0,0)に設定するとy軸が上になります
	);

	// Projection行列を計算
	projectionMat = glm::perspective(
		glm::radians(45.0f), // ズームの度合い(通常90～30)
		4.0f / 3.0f,		// アスペクト比
		0.1f,		// 近くのクリッピング平面
		100.0f		// 遠くのクリッピング平面
	);

	mat4 vpMat = projectionMat * viewMat;

	modelMat[1] = glm::scale(modelMat[1], glm::vec3(10.0f,0.9f,10.0f));
	modelMat[1] = glm::translate(modelMat[1], glm::vec3(0, -4, -5));

	modelMat[2] = glm::scale(modelMat[2], glm::vec3(4.0f));
	modelMat[2] = glm::translate(modelMat[2], camera_pos);

	// ゲームループ
	while (!glfwWindowShouldClose(window))
	{
		// View行列を計算
		viewMat = glm::lookAt(
			camera_pos, // ワールド空間でのカメラの座標
			vec3(0.0, 1.0, 0.0), // 見ている位置の座標
			vec3(0.0, 1.0, 0.0)  // 上方向を示す。(0,1.0,0)に設定するとy軸が上になります
		);

		// Projection行列を計算
		projectionMat = glm::perspective(
			glm::radians(45.0f), // ズームの度合い(通常90～30)
			4.0f / 3.0f,		// アスペクト比
			0.1f,		// 近くのクリッピング平面
			100.0f		// 遠くのクリッピング平面
		);
		// 画面の初期化
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0);
		glEnable(GL_DEPTH_TEST);

		modelMat[0] = calculateModel(modelMat[0]);

		mat4 mv = viewMat * modelMat[0];
		glm::mat3 m_3x3_inv_transp = glm::transpose(glm::inverse(glm::mat3(mv)));

		// uniform 変数の位置を取得、programオブジェクトにデータを通知
		glUniformMatrix4fv(glGetUniformLocation(programId, "vpMat"), 1, GL_FALSE, &vpMat[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(programId, "modelMat"), 1, GL_FALSE, &modelMat[0][0][0]);
		glUniformMatrix3fv(glGetUniformLocation(programId, "NormalMatrix"), 1, GL_FALSE, &m_3x3_inv_transp[0][0]);
		glUniform1i(glGetUniformLocation(programId, "texture"), 0);

		glBindTexture(GL_TEXTURE_2D, textureID[0]);
		glBindVertexArray(vao[0]);

		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		// モデルの描画
		glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

		
		m_3x3_inv_transp = glm::mat3(0.0f);
		glUniformMatrix4fv(glGetUniformLocation(programId, "modelMat"), 1, GL_FALSE, &modelMat[1][0][0]);
		glUniformMatrix3fv(glGetUniformLocation(programId, "NormalMatrix"), 1, GL_FALSE, &m_3x3_inv_transp[0][0]);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, textureID[1]);
		glBindVertexArray(vao[1]);

		//int size;
		//glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		// モデルの描画
		glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

		
		glUniformMatrix4fv(glGetUniformLocation(programId, "modelMat"), 1, GL_FALSE, &modelMat[2][0][0]);
		glUniformMatrix3fv(glGetUniformLocation(programId, "NormalMatrix"), 1, GL_FALSE, &m_3x3_inv_transp[0][0]);
		(glGetUniformLocation(programId, "NormalMatrix"), 1, GL_FALSE, &camera_pos[0]);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindTexture(GL_TEXTURE_2D, textureID[2]);
		glBindVertexArray(vao[2]);

		//int size;
		//glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		// モデルの描画
		glDrawElements(GL_TRIANGLES, size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

		// バッファの入れ替え
		glfwSwapBuffers(window);

		// Poll for and process events
		glfwPollEvents();
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

	programId = crateShader();
	attLocation = glGetAttribLocation(programId, "position");
	attNormal = glGetAttribLocation(programId, "vnormal");
	uvLocation = glGetAttribLocation(programId, "uv");
	textureLocation = glGetUniformLocation(programId, "texture");

	vao.push_back(makevao("model/untitled2.obj", "img/monkey.png"));
	vao.push_back(makevao("model/floor2.obj", "img/uv_checker.png"));
	vao.push_back(makevao("model/back.obj", "img/back_img.png"));

	loopGL();

	glfwTerminate();

	return 0;
}