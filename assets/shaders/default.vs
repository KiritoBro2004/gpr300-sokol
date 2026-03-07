#version 410

// attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

// uniforms
uniform mat4 view_proj;
uniform mat4 model;
uniform mat4 lightSpaceMatrix;

// varyings
out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_texcoord;
out vec4 vs_lightSpacePos;


void main()
{
  vs_position = in_position;
  vs_normal = transpose(inverse(mat3(model))) * in_normal;
  vs_texcoord = in_texcoord;
  gl_Position = view_proj * model * vec4(in_position, 1.0);
  vs_lightSpacePos = lightSpaceMatrix * model * vec4(in_position, 1.0);
}

