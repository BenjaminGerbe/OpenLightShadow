
#version 330 core

in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TexCoords;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

uniform Matrices {
	mat4 u_ViewMatrix;
	mat4 u_ProjectionMatrix;
};


void main()
{
    TexCoords = a_Position;
    gl_Position = projection * view * vec4(a_Position, 1.0);
}  