#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

const char* world_vertex_shader =
R"zzz(#version 410 core

// input from render
layout (location = 0) in vec4 vertex_position;

// input from instances
layout (location = 1) in vec3 instance_offset;
layout (location = 2) in uint direction;
layout (location = 3) in uint texture_index;

uniform mat4 light_space;

out uint vs_direction;
out uint vs_texture_index;
out vec4 vs_light_space_position;

void main()
{
  vec3 pos = vertex_position.xyz;
  switch(direction) {
    case 0: // +X
      pos.z *= -1; break;
    case 1: // +Y
      pos = pos.yxz; break;
    case 2: // +Z
      pos = pos.zyx; break;
    case 3: // -X
      pos = -pos; 
      break;
    case 4: // -Y
      pos = -pos.yxz; 
      pos.x *= -1;
      break;
    case 5: // -Z
      pos = -pos.zyx; 
      pos.x *= -1;
      break;
  }
  vs_direction = direction;
  vs_texture_index = texture_index;
	gl_Position = vec4(instance_offset, 0) + vec4(pos, 1);
  vs_light_space_position = light_space * gl_Position;
}
)zzz";

const char* world_geometry_shader =
R"zzz(#version 410 core

// layout description between vertex and fragment shader
layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;

// input uniform
uniform mat4 projection;
uniform mat4 view;

// input from vertex shader
in uint vs_direction[];
in uint vs_texture_index[];
in vec4 vs_light_space_position[];

// pass along from vertex shader
flat out uint sq_direction;
flat out uint sq_texture_index;

// output to fragment shader
flat out vec4 normal;
out vec4 bary_coord;
flat out float perimeter;
out vec2 tex_coord;

out vec4 world_position;
out vec4 light_space_position;

void main()
{
  sq_direction = vs_direction[0];
  sq_texture_index = vs_texture_index[0];
  
	vec3 AB = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 AC = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	normal = normalize(vec4(cross(AB, AC), 0));

	vec3 A = gl_in[0].gl_Position.xyz;
	vec3 B = gl_in[1].gl_Position.xyz;
	vec3 C = gl_in[2].gl_Position.xyz;
  vec3 D = A + (C - B);

	perimeter = length(A - B) + length(B - C) + length(C - A);
	
  world_position = vec4(A, 1);
  light_space_position = vs_light_space_position[0];
  gl_Position = projection * view * world_position;
  bary_coord = vec4(1, 0, 0, 0);
  tex_coord = vec2(0, 0);
  EmitVertex();

  world_position = vec4(D, 1);
  light_space_position = vs_light_space_position[0] + (vs_light_space_position[2] - vs_light_space_position[1]);
  gl_Position = projection * view * world_position;
  bary_coord = vec4(0, 0, 0, 1);
  tex_coord = vec2(0, 1);
  EmitVertex();

  world_position = vec4(B, 1);
  light_space_position = vs_light_space_position[1];
  gl_Position = projection * view * world_position;
  bary_coord = vec4(0, 1, 0, 0);
  tex_coord = vec2(1, 0);
  EmitVertex();

  world_position = vec4(C, 1);
  light_space_position = vs_light_space_position[2];
  gl_Position = projection * view * world_position;
  bary_coord = vec4(0, 0, 1, 0);
  tex_coord = vec2(1, 1);
  EmitVertex();
  
	EndPrimitive();
}
)zzz";

// must use every input from geometry shader
const char* world_fragment_shader =
R"zzz(#version 410 core

uniform bool wireframe; 
uniform vec4 light_position;

uniform vec4 base_colors[10];
uniform vec4 off_colors[10];

uniform sampler2D shadowMap;

// input from vertex shader passed through geometry shader
flat in uint sq_direction;
flat in uint sq_texture_index;

flat in vec4 normal;
in vec4 bary_coord;
flat in float perimeter;
in vec2 tex_coord;
flat in vec3 flag_color;

in vec4 world_position;
in vec4 light_space_position;

out vec4 fragment_color;

float calculateShadow() {
  // divide by homogeneous component
  vec3 proj_pos = light_space_position.xyz / light_space_position.w;

  // [-1, 1] -> [0, 1]
  proj_pos = proj_pos * 0.5 + 0.5;
         
  float closest_to_light_d = texture(shadowMap, proj_pos.xy).r;

  float closest_to_camera_d = proj_pos.z;

  vec4 light_dir = normalize(world_position - light_position);
  // float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
  float bias = 0.003;

  float shadow = 0.0;
  vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
  for(int x = -1; x <= 1; ++x) {
    for(int y = -1; y <= 1; ++y) {
      float pcf_d = texture(shadowMap, proj_pos.xy + vec2(x, y) * texelSize).r; 
      shadow += closest_to_camera_d - bias > pcf_d ? 1.0 : 0.0;        
    }
  }
  shadow /= 9.0;
  
  // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
  if(proj_pos.z > 1.0)
    shadow = 0.0;

  return shadow;
}


float rand(vec2 co){
  return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 49.5428);
}

float blerp (float i00, float i10, float i01, float i11, float u, float v) {
    float lerp_x0 = mix(i00,i10, u);
    float lerp_x1 = mix(i01,i11, u);

    return mix(lerp_x0, lerp_x1, v);
}

float get_noise_level(float u, float v, float grit_level) {
  // 3 grits 8x8, 16x16, 32x32
  float grit00 = rand(floor(vec2(u,v) * grit_level) + vec2(0,0));
  float grit10 = rand(floor(vec2(u,v) * grit_level) + vec2(1,0));
  float grit01 = rand(floor(vec2(u,v) * grit_level) + vec2(0,1));
  float grit11 = rand(floor(vec2(u,v) * grit_level) + vec2(1,1));

  float new_u = (u - (floor(u * grit_level) / grit_level)) * grit_level;
  float new_v = (v - (floor(v * grit_level) / grit_level)) * grit_level;

  float grit = blerp(grit00, grit10, grit01, grit11, new_u, new_v);
  return grit;
}

float cross_lines(float u, float v, float rand) {
  return sin(u - v + rand);
}

float ProceduralNoise(float u, float v) {
  // 3 grits 8x8, 16x16, 32x32
  float high_grit = get_noise_level(u, v, 8);
  float mid_grit = get_noise_level(u, v, 16);
  float low_grit = get_noise_level(u, v, 32);

  
  return (high_grit + 2/3.f * mid_grit + 1/3.f * low_grit) / 2;
}

float TWO_PI = 6.28318530717958647692528676655900576;
#define WATER 3

vec3 get_gradient(int i, int j, int k) {
  float theta = rand(vec2(i, j));
  float phi   = rand(vec2(i + j, k));
  return vec3(cos(theta), sin(theta), tan(phi));
}

float perlin(float x, float y, float z) {

  vec3 pos = vec3(x, y, z) + vec3(5);

  int x0 = int(pos.x);
  int x1 = x0 + 1;
  int y0 = int(pos.y);
  int y1 = y0 + 1;
  int z0 = int(pos.z);
  int z1 = z0 + 1;

  vec3 g000 = get_gradient(x0, y0, z0);
  vec3 g010 = get_gradient(x0, y1, z0);
  vec3 g100 = get_gradient(x1, y0, z0);
  vec3 g110 = get_gradient(x1, y1, z0);

  vec3 g001 = get_gradient(x0, y0, z1);
  vec3 g011 = get_gradient(x0, y1, z1);
  vec3 g101 = get_gradient(x1, y0, z1);
  vec3 g111 = get_gradient(x1, y1, z1);

  // corners
  vec3 c000 = vec3(x0, y0, z0);
  vec3 c100 = vec3(x1, y0, z0);
  vec3 c010 = vec3(x0, y1, z0);
  vec3 c110 = vec3(x1, y1, z0);

  vec3 c001 = vec3(x0, y0, z1);
  vec3 c101 = vec3(x1, y0, z1);
  vec3 c011 = vec3(x0, y1, z1);
  vec3 c111 = vec3(x1, y1, z1);

  // distances
  vec3 d000 = pos - c000;
  vec3 d010 = pos - c010;
  vec3 d100 = pos - c100;
  vec3 d110 = pos - c110;

  vec3 d001 = pos - c001;
  vec3 d011 = pos - c011;
  vec3 d101 = pos - c101;
  vec3 d111 = pos - c111;

  // products
  float p000 = dot(g000, d000);
  float p010 = dot(g010, d010);
  float p100 = dot(g100, d100);
  float p110 = dot(g110, d110);

  float p001 = dot(g001, d001);
  float p011 = dot(g011, d011);
  float p101 = dot(g101, d101);
  float p111 = dot(g111, d111);

  // heights and interpolation
  float h_00 = mix(p000, p100, d000.x);
  float h_10 = mix(p010, p110, d000.x);
  float h_0 = mix(h_00, h_10, d000.y);

  float h_01 = mix(p001, p101, d000.x);
  float h_11 = mix(p011, p111, d000.x);
  float h_1 = mix(h_01, h_11, d000.y);

  float h = mix(h_0, h_1, d000.z);

  return h;
}

vec4 axis_color() {
  uint dir = sq_direction;
  if (dir >= 3) {
    dir -= 3;
  } 

  vec4 color = vec4(0);
  color[dir] = 1;

  return color;
}

vec4 height_atten(vec4 color) {
  float w = (120 - world_position.y)/150;
  return mix(vec4(0, 0, 0, 0), color, 1 - w);
}

vec4 menger_color(vec4 color) {
  // vec4 color = abs(normal);

	vec4 light_direction = normalize(world_position - light_position);
	float dot_nl = dot(normalize(light_direction), normalize(normal));
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	return mix(clamp(dot_nl * color, 0.0, 1.0), color, .7);
}

void main()
{
  // fragment_color = vec4(tex_coord, 0, 1);

  // float alpha = 10;
  // float beta = 1;

  // float vert_line_weight = 17;
  // float hori_line_weight = 17;

  // float noise = cross_lines(hori_line_weight * tex_coord.x, vert_line_weight * tex_coord.y , alpha * ProceduralNoise(beta * tex_coord.x, beta * tex_coord.y) );
  // fragment_color = mix(fragment_color, vec4(noise, noise, noise, 1), 0.5);

  // fragment_color = mix(vec4(0, 0, 0, 1), vec4(0, 1, 0, 1), perlin(world_position.x, world_position.y));

  vec3 i = world_position.xyz;

  float k = 8;
  i = vec3(floor(i * 8))/8;

  float p = perlin(2 * i.x, 2 * i.y, 2 * i.z);
  fragment_color = mix(base_colors[sq_texture_index], off_colors[sq_texture_index], p);
  fragment_color = height_atten(fragment_color);
	
  fragment_color = menger_color(fragment_color);
  if (sq_texture_index == WATER) fragment_color =vec4(.75f,.75f,.75f,1) * vec4(0.05f,0.2f,.7f,.6f); //FIXME: water should be see-through
  // if (sq_texture_index == WATER) fragment_color.w = 1;
  else fragment_color.w = 1;

  if (light_space_position.w != 0) {

    float shadow_ratio = calculateShadow();
    vec4 shadow_color = mix(vec4(0, 0, 0, 1), fragment_color, 1 - shadow_ratio);
    fragment_color = mix(shadow_color, fragment_color, shadow_ratio > 0.5 ? 0.5 : 1);
    float depth = gl_FragCoord.z;
    
    // depth = clamp((depth - 0.99) * 100, 0, 1);
    // fragment_color = mix(vec4(0.5, 0.5, 0.5, 1), fragment_color, 1 - depth);
  } else {
    // fragment_color = vec4(gl_FragCoord.zzz, 1);
  }  

  if (wireframe) {
    bool is_frame = min(bary_coord.x, min(bary_coord.y, bary_coord.z)) * perimeter < 0.05;
    if (is_frame) {
      fragment_color = vec4(0, float(is_frame), 0, 1);
    }
  }
}
)zzz";
