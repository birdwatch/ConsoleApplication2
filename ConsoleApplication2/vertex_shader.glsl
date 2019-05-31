#version 330 core

attribute vec3 position;
attribute vec3 vnormal;
attribute vec2 uv;
uniform mat4 vpMat;
uniform mat4 modelMat;
uniform mat3 NormalMatrix;
out vec3 color;
out vec2 vuv;
const vec3 lightDirection = normalize(vec3(-10.0, 6.0, 10.0));

void main(void)
{
    vec3 tnorm = normalize(NormalMatrix * vnormal);
	if(NormalMatrix==mat3(0.0f))
	{
	color=vec3(1.0f);
	}
	else
	{
    color = vec3(dot(tnorm, lightDirection));
	}
    gl_Position =vpMat * modelMat * vec4(position,1);
    vuv=uv;

}