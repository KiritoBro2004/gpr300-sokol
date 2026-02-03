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

struct palette {
  vec3 color1;
  vec3 color2;
};

// varying;
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

// uniforms
uniform vec3 camera;
uniform Light light;
uniform palette pal;
uniform Material material; //shininess is alpha

uniform sampler2D txGradient;


vec3 toonShading(vec3 normal, vec3 frag_position, Light light) {
  vec3 view_dir = normalize(camera - frag_position);
  vec3 light_dir = normalize(light.position - frag_position);
  vec3 reflect_dir = reflect(light_dir, normal);
  vec3 half_dir = normalize(light_dir + view_dir);

  //float NdotL = max(dot(normal, light_dir), 0.0); 
  float NdotL = (dot(normal, light_dir) + 1) * 0.5;
  float NdotH = max(dot(normal, half_dir), 0.0);

  
  vec3 diffuse = NdotL * material.diffuse;
  //float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
  //vec3 specular = light.color * (spec * material.specular);  
  vec3 gradient = texture(txGradient, vec2(NdotL, NdotL)).rgb;

  vec3 light_color = mix(pal.color2, pal.color1, gradient);

  return (light_color);
}

void main()
{
  vec3 ambient = material.ambient;
  vec3 lighting = toonShading(vs_normal, vs_position, light) + ambient * 0.5;
  //vec3 object_color = vs_normal * 0.5 + 0.5;
  vec3 final_color = lighting;
  FragColor = vec4(final_color, 1.0);
}