#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint w;
uniform uint h;
uniform uint d;
uniform float scale;

layout(rgba32f, binding = 0) uniform image2D in2d;
layout(rgba32f, binding = 1) uniform image3D in3d;
layout(rgba32f, binding = 2) uniform image2D img2d;
layout(rgba32f, binding = 3) uniform image3D img3d;

#define GI gl_GlobalInvocationID

#define CURLA 3.0
#define CURLB 0.5
vec3 gn3(ivec3 kms) {
  float p1r = imageLoad(in3d, ivec3(kms.r + 1 + 1, kms.g + 1, kms.b + 1)).r;
  float p2r = imageLoad(in3d, ivec3(kms.r + 1 - 1, kms.g + 1, kms.b + 1)).r;
  float p1g = imageLoad(in3d, ivec3(kms.r + 1, kms.g + 1 + 1, kms.b + 1)).r;
  float p2g = imageLoad(in3d, ivec3(kms.r + 1, kms.g + 1 - 1, kms.b + 1)).r;
  float p1b = imageLoad(in3d, ivec3(kms.r + 1, kms.g + 1, kms.b + 1 + 1)).r;
  float p2b = imageLoad(in3d, ivec3(kms.r + 1, kms.g + 1, kms.b + 1 - 1)).r;
  return vec3((p1r - p2r) * CURLA + CURLB, 
              (p1g - p2g) * CURLA + CURLB, 
              (p1b - p2b) * CURLA + CURLB);
}

void tmkp3c() { 
  ivec3 texelCoord = ivec3(GI.xyz);

  //imageStore(img3d, texelCoord, vec4(1.0)); return;
  imageStore(img3d, texelCoord, vec4(gn3(texelCoord), 1.0));
}
        ///G(im->v, i, j, w).b = (octave_perlin(0.0f, i / scale, (j + eps) / scale, octaves, persistence) - octave_perlin(0.0f, i / scale, j / scale, octaves, persistence)) / (2 * eps) * CURLA + CURLB;

void tmkp2c() { 
  ivec2 texelCoord = ivec2(GI.xy);

  imageStore(img2d, texelCoord, vec4(1.0));
}

void main() {
  if (d == 1) { tmkp2c(); } 
         else { tmkp3c(); }
}

