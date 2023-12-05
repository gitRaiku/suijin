#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform uint w;
uniform uint h;
uniform uint d;
uniform float scale;
uniform uint octaves;
uniform float persistence;

layout(rgba32f, binding = 0) uniform image2D img2d;
layout(rgba32f, binding = 2) uniform image3D img3d;
layout (std430, binding = 1) buffer Pos { uint p[]; } p;

#define P2(x) ((x)*(x))
#define GI gl_GlobalInvocationID

float grad(uint hash, float x, float y, float z) {
  uint h = hash & 15;
  float u = h < 8 ? x : y;
  float v;

  if (h < 4) { v = y; } 
  else if (h == 12 || h == 14) { v = x; } 
  else { v = z; }

  return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float fade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

float perlin(float x, float y, float z) {
  uint xi = uint(x) & 0xFF;
  uint yi = uint(y) & 0xFF;
  uint zi = uint(z) & 0xFF;
  float xf = x - uint(x);
  float yf = y - uint(y);
  float zf = z - uint(z);
  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  uint aaa, aba, aab, abb, baa, bba, bab, bbb;
  aaa = p.p[p.p[p.p[xi    ] + yi    ] + zi    ];
  aba = p.p[p.p[p.p[xi    ] + yi + 1] + zi    ];
  aab = p.p[p.p[p.p[xi    ] + yi    ] + zi + 1];
  abb = p.p[p.p[p.p[xi    ] + yi + 1] + zi + 1];
  baa = p.p[p.p[p.p[xi + 1] + yi    ] + zi    ];
  bba = p.p[p.p[p.p[xi + 1] + yi + 1] + zi    ];
  bab = p.p[p.p[p.p[xi + 1] + yi    ] + zi + 1];
  bbb = p.p[p.p[p.p[xi + 1] + yi + 1] + zi + 1];

  float x1, x2, y1, y2;
  x1 = mix(grad(aaa, xf    , yf    , zf    ),
           grad(baa, xf - 1, yf    , zf    ), u);
  x2 = mix(grad(aba, xf    , yf - 1, zf    ),
           grad(bba, xf - 1, yf - 1, zf    ), u);
  y1 = mix(x1, x2, v);

  x1 = mix(grad(aab, xf    , yf    , zf - 1),
           grad(bab, xf - 1, yf    , zf - 1), u);
  x2 = mix(grad(abb, xf    , yf - 1, zf - 1),
           grad(bbb, xf - 1, yf - 1, zf - 1), u);
  y2 = mix(x1, x2, v);

  return (mix(y1, y2, w) + 1) / 2;
}

float octave_perlin(float x, float y, float z, uint octaves, float persistence) {
  //return p.p[1];
  float total = 0.0;
  float frequency = 1.0;
  float amplitude = 1.0;
  float maxValue = 0.0;
  uint i;
  for (i = 0; i < octaves; ++i) {
    total += perlin(x * frequency, y * frequency, z * frequency) * amplitude;

    maxValue += amplitude;

    amplitude *= persistence;
    frequency *= 2;
  }

  return total / maxValue;
}

void tmkp3d() { 
  float dx = GI.x / scale;
  float dy = GI.y / scale;
  float dz = GI.z / scale;
  ivec3 texelCoord = ivec3(GI.xyz);

  imageStore(img3d, texelCoord, vec4(vec3(octave_perlin(dx, dy, dz, octaves, persistence)), 0.0));
}

void tmkp2d() { 
  float dx = GI.x / scale;
  float dy = GI.y / scale;
  ivec2 texelCoord = ivec2(GI.xy);

  imageStore(img2d, texelCoord, vec4(vec3(octave_perlin(dx, dy, 0.0, octaves, persistence)), 0.0));
}

void main() {
  if (d == 1) {
    tmkp2d();
  } else {
    tmkp3d();
  }
}
