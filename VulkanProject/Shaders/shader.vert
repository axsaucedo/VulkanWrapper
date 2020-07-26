#version 450 // Use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

// This is a different binding than the one above
layout(binding = 0) uniform UboViewProjection {
	 mat4 projection;
	 mat4 view;
} uboViewProjection;

// NOT IN USE, LEFT ONLY FOR REFERENCE / SHOW 
// 	(This is what we were using before adding push_constant below)
//   layout(binding = 1) uniform UboModel {
// 	   mat4 model;
//   } uboModel;

layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragCol;

void main() {
	gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);

	fragCol = col;
}