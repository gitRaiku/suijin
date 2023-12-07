#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(r8, binding = 0) uniform image3D w1;
layout(r8, binding = 1) uniform image3D w2;
layout(r8, binding = 2) uniform image3D w3;
layout(r8, binding = 3) uniform image3D w4;
layout(r8, binding = 4) uniform image3D p;
layout(rgba32f, binding = 5) uniform image3D res;

float combine(float p, float w) { return w - w * p; } /// TODO: Make nicer
void main() { 
  ivec3 cp = ivec3(gl_GlobalInvocationID.xyz);
  //imageStore(res, cp, vec4(imageLoad(p, cp).r));
  //return;
  imageStore(res, 
      cp, 
      vec4(
        combine(
          imageLoad(p, cp).r, 
          imageLoad(w4, cp).r
          ), 
        imageLoad(w1, cp).r, 
        imageLoad(w2, cp).r, 
        imageLoad(w3, cp).r
        )); }
