#version 410

out vec4 FragColor;

// varyings
in vec2 vs_texcoord;

// uniforms
uniform sampler2D screen;
uniform float effect;
uniform float vingette_strength;
uniform float fisheye_radius;
uniform float fisheye_scale;
uniform float time;
uniform float buffer_line_time_split;
uniform float buffer_line_y_split;
uniform float buffer_line_strength;
uniform float kernel_strength;
uniform float buffer_circle_time_split;
uniform float buffer_circle_dist_split;
uniform float buffer_circle_strength;

uniform float duplication_screen_divisor;

const float offset = 1.0 / 300.0;

const vec2 offsets[9] = vec2[](
  vec2(-offset, offset), //top-left
  vec2(0.0, offset), //top-middle
  vec2(offset, offset), //top-right
  
  vec2(-offset, 0.0), //center-left
  vec2(0.0, 0.0), //center-middle
  vec2(offset, 0.0), //center-right 

  vec2(-offset, -offset), //bottom-left
  vec2(0.0, -offset), //bottom-middle
  vec2(offset, -offset) //bottom-right
);

const float light_ridge_kernel[9] = float[](
    0.0, -1.0,  0.0,
    -1.0, 4.0, -1.0,
    0.0, -1.0,  0.0
);

const float heavy_ridge_kernel[9] = float[](
    -1.0, -1.0, -1.0,
    -1.0,  8.0, -1.0,
    -1.0, -1.0, -1.0
);

const float sharpen_kernel[9] = float[](
    0.0, -1.0, 0.0,
    -1.0,  5.0, -1.0,
    0.0, -1.0, 0.0
);

const float box_blur_kernel[9] = float[](
    1.0, 1.0, 1.0,
    1.0, 1.0, 1.0,
    1.0, 1.0, 1.0
);

const float gaussian_blur_kernel[9] = float[](
    1.0, 2.0, 1.0,
    2.0, 4.0, 2.0,
    1.0, 2.0, 1.0
);


void main()
{
  vec3 color = texture(screen, vs_texcoord).rgb;
  
  // Switch Statements Weren't Working (sorry not sorry)
  if(effect == 0) // Light Ridge
  {
    // Apply Light Ridge Kernel
    for(int i = 0; i < 9; i++)
    {
      vec3 local = vec3(texture(screen, vs_texcoord + offsets[i]));
      color += local * light_ridge_kernel[i] * kernel_strength;
    }

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 1) // Heavy Ridge
  {
    // Apply Heavy Ridge Kernel
    for(int i = 0; i < 9; i++)
    {
      vec3 local = vec3(texture(screen, vs_texcoord + offsets[i]));
      color += local * heavy_ridge_kernel[i] * kernel_strength;
    }

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 2) // Sharpen
  {
    // Apply Sharpen Kernel
    for(int i = 0; i < 9; i++)
    {
      vec3 local = vec3(texture(screen, vs_texcoord + offsets[i]));
      color += local * sharpen_kernel[i] * kernel_strength;
    }

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }

  if(effect == 3) // Box Blur
  {
    // Apply Box Blur Kernel
    for(int i = 0; i < 9; i++)
    {
      vec3 local = vec3(texture(screen, vs_texcoord + offsets[i]));
      color += local * (box_blur_kernel[i] / 9f) * kernel_strength; // * 1/9 for blox blur effect
    }

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 4) // Gaussian Blur
  {
    // Apply Gaussian Blur Kernel
    for(int i = 0; i < 9; i++)
    {
      vec3 local = vec3(texture(screen, vs_texcoord + offsets[i]));
      color += local * (gaussian_blur_kernel[i] / 16f) * kernel_strength; // * 1/16 for gaussian blur effect
    }

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 5) // Square Vingette
  {
    float vingette_x = 0.0;
    float vingette_y = 0.0;

    // Get X Vingette
    if(vs_texcoord.x < vingette_strength)
    {
      vingette_x -= vingette_strength - vs_texcoord.x;
    }
    if(vs_texcoord.x > (1 - vingette_strength))
    {
      vingette_x -= vs_texcoord.x - (1 - vingette_strength);
    }

    // Get Y Vingette
    if(vs_texcoord.y < vingette_strength)
    {
      vingette_y -= vingette_strength - vs_texcoord.y;
    }
    if(vs_texcoord.y > (1 - vingette_strength))
    {
      vingette_y -= vs_texcoord.y - (1 - vingette_strength);
    }

    // Apply Difference Of Both Toward Black
    color += vingette_x + vingette_y;
    
    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 6) // Fisheye
  {
    // Get Distance From Center To Texcoord
    vec2 fisheye_coord = vs_texcoord;
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(center, vs_texcoord);

    // If Distance Is Less Than Selected Radius
    if(dist < fisheye_radius)
    {
      // Calculate The Percentage Of Distortion
      float percent_distortion = 1 + ((0.5 - dist) / 0.5) * -fisheye_scale;

      // Offset By Center To Apply Then Inverse Offset To Be Back In Line
      fisheye_coord -= center;
      fisheye_coord = fisheye_coord * percent_distortion;
      fisheye_coord += center;
    }

    // Grab New Texcoord 
    vec3 new_color = texture(screen, fisheye_coord).rgb;

    // Send To FragColor
    FragColor = vec4(vec3(new_color), 1.0);
  }
  
  if(effect == 7) // Buffer Lines
  {
    // Split Time, Then Get Percent In Split
    float time_amount = mod(time, buffer_line_time_split);
    float time_percent = 1 - (time_amount/buffer_line_time_split);

    // Split Y Texcoord, Then Add Time Percent To Get Moving Effect
    float y_change = mod(vs_texcoord.y+(time_percent*buffer_line_y_split), buffer_line_y_split);

    // Edit Color With Y Change & Strength
    color -= y_change * buffer_line_strength;

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }

  if(effect == 8) // Buffer Circles
  {
    // Split Time, Then Get Percent In Split
    float time_amount = mod(time, buffer_circle_time_split);
    float time_percent = 1 - (time_amount/buffer_circle_time_split);

    // Get Distance From Center To Texcoord
    vec2 fisheye_coord = vs_texcoord;
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(center, vs_texcoord);

    float dist_change = mod(dist+(time_percent*buffer_circle_dist_split), buffer_circle_dist_split);

    // Edit Color With Y Change & Strength
    color -= dist_change * buffer_circle_strength;

    // Send To FragColor
    FragColor = vec4(vec3(color), 1.0);
  }
  
  if(effect == 9)
  {
    float screen_x = mod(vs_texcoord.x, (1/duplication_screen_divisor));
    float screen_y = mod(vs_texcoord.y, (1/duplication_screen_divisor));
    vec2 new_coord = vec2(screen_x*duplication_screen_divisor, screen_y*duplication_screen_divisor);
    vec3 new_color = texture(screen, new_coord).rgb;

    // Send To FragColor
    FragColor = vec4(vec3(new_color), 1.0);
  }
}