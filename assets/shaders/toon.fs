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
in vec4 vs_lightSpacePos;

// uniforms
uniform vec3 camera;
uniform Light light;
uniform palette pal;
uniform Material material; //shininess is alpha
uniform sampler2D shadowMap;
uniform sampler2D txGradient;
uniform float minBias;
uniform float maxBias;


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

float calculateShadow(vec4 lightSpacePos, vec3 lightDir)
{
    // convert from clip space (-1 to 1) to texture space (0 to 1)
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // sample the closest depth from the light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // current fragment depth from light
    float currentDepth = projCoords.z;

    // if current depth is greater than closest, it's in shadow
    float bias = max(maxBias * (1.0 - dot(vs_normal, lightDir)), minBias);
    return (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
}

void main()
{
  vec3 ambient = material.ambient;
  vec3 lighting = toonShading(vs_normal, vs_position, light) + ambient * 0.5;
  float shadow = calculateShadow(vs_lightSpacePos, normalize(light.position - vs_position));
  vec3 final_color = lighting * (1.0 - shadow * 0.5);
  FragColor = vec4(final_color, 1.0);
}