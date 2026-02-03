#version 410

precision mediump float;

out vec4 FragColor;

struct Light {
  vec3 color;
  vec3 position;
};

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

// varying;
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

// uniforms
uniform vec3 camera;
uniform Light light;
uniform Material material; //shininess is alpha
uniform sampler2D txTrippy;

vec3 blinnphong(vec3 normal, vec3 frag_position, Light light) {
  // glsl: dot(vec3, vec3)
  vec3 view_dir = normalize(camera - frag_position);
  vec3 light_dir = normalize(light.position - frag_position);
  vec3 reflect_dir = reflect(light_dir, normal);
  vec3 half_dir = normalize(light_dir + view_dir);

// apply material ; (offset) -> learnopengl material
  float NdotL = max(dot(normal, light_dir), 0.0);
  float NdotH = max(dot(normal, half_dir), 0.0);
  
  float PdotL = dot(frag_position, light.position);

  vec3 diffuse = NdotL * material.diffuse;
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
  vec3 specular = light.color * (spec * material.specular);  

  vec3 lighting = diffuse + specular;
  return (lighting * light.color);
}

void main()
{
  vec3 ambient = material.ambient;
  vec3 lighting = blinnphong(vs_normal, vs_position, light) + ambient * 0.5;
  vec3 object_color = vs_normal * 0.5 + 0.5;

  vec3 tex = texture(txTrippy, vs_texcoord).rgb;

  vec3 final_color = (object_color+tex) * lighting;
  FragColor = vec4(final_color, 1.0);
}