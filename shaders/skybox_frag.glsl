#version 460 core
#define PI 3.14159265
#define EP 0.00000001
#define UT 0.33333333
#define P2(x) ((x)*(x))

out vec4 FragColor;

in vec2 tc;
in vec3 pp;

uniform vec2  sunPos;
uniform vec3  sunCol;
uniform float sunSiz;
uniform sampler2D tex;

vec3 h2c3(int c, int x) {
  return vec3(((c & 0xFF00) >> 8) / 255.0f, ((c & 0x00FF) >>  0) / 255.0f, ((x & 0x00FF) >>  0) / 255.0f);
}

float gd(float x, float y) {
  return min(y - x, PI - y + x);
}

float fm(float m) {
  if (m < 0) {
    return PI + m;
  } else {
    return PI - m;
  }
}

float at2(float x, float y) {
  float r = 0.0f;
  if (x != 0) {
    r = atan(y / x);
    if (x < 0) {
      if (y >= 0) {
        r += PI;
      } else {
        r -= PI;
      }
    }
  } else {
    if (y >= 0) {
      r += PI / 2;
    } else {
      r -= PI / 2;
    }
  }
  return r;
}

void main() {
  /*
  vec3 c = {1.0f, 1.0f, 1.0f};
  if (tc.x >= 0.75) {
    if (tc.y >= 0.66) {
      c = h2c3(0xcccc, 0xcc);
    } else if (tc.y >= UT) {
      c = h2c3(0xfff3, 0xa6);
    } else {
      c = h2c3(0x1212, 0x12);
    }
  } else if (tc.x >= 0.5) {
    if (tc.y >= 0.66) {
      c = h2c3(0xa1e6, 0xb1);
    } else if (tc.y >= UT) {
      c = h2c3(0x02ff, 0x00);
    } else {
      c = h2c3(0xff40, 0x00);
    }
  } else if (tc.x >= 0.25) {
    if (tc.y >= 0.66) {
      c = h2c3(0x8990, 0xea);
    } else if (tc.y >= UT) {
      c = h2c3(0x0013, 0xff);
    } else {
      c = h2c3(0xffd6, 0x00);
    }
  } else {
    if (tc.y >= 0.66) {
      c = h2c3(0x924e, 0xc5);
    } else if (tc.y >= UT) {
      c = h2c3(0xff00, 0xd6);
    } else {
      c = h2c3(0x00ff, 0xff);
    }
  }
  FragColor = vec4(c, 1.0f);
  */

  /*vec3 np = normalize(pp);
  vec3 vl = vec3(1.0f, 0.0f, 0.0f);
  vec2 pos = vec2(0.0f, 0.0f);
  float t = np.y;
  np.y = np.z;
  np.z = t;
  pos.x = sin(np.z);
  pos.y = asin(np.y / sqrt(1 - pos.x * pos.x));

  vec3 fcol = vec3(0.0f);
  vec3 horizon = h2c3(0xa5c1, 0xd7);
  vec3 sky = h2c3(0x0b0c, 0x8c);
  if (pos.x <= 0.5) {
    fcol = mix(sky, horizon, pos.x * 2);
  } else {
    float x = (pos.x - 0.5) * 2;
    float f = x*x*x*x* -3.71435 + x*x*x * 7.49902 - x*x * 5.10417 + x * 2.31549 + 0.00360439;
    fcol = mix(sky, horizon, 1 - f);
  }

  FragColor = vec4(vec3(pos.y), 1.0f);
  return;




  float l1 = pos.y;
  float l2 = 0.7;
  float f1 = pos.x;
  float f2 = 0.6;
  float dl = abs(l2 - l1);
  float ds = acos(sin(f1) * sin(f2) + cos(f1) * cos(f2) * cos(dl));


  FragColor = vec4(vec3(ds), 1.0f);
  return;

  float hd = abs(0.2 - pos.x);
  float ld = sin(gd(0.2, pos.y)) * (cos(pos.x) * cos(hd) / 3);
  float d = sqrt(ld * ld + hd * hd);
  if (d > 0.1) {
    d = 0.0f;
  }


  FragColor = vec4(vec3(d), 1.0f);
  return;













  //fcol = mix(fcol, sunCol, d);

  FragColor = vec4(fcol, 1.0f);
   */

  FragColor = texture(tex, tc);
}
