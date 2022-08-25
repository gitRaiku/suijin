#version 460 core
#define PI 3.1415

out vec4 FragColor;

in vec2 tc;

uniform vec2 sunPos;

vec3 h2c3(int c, int x) {
  return vec3(((c & 0xFF00) >> 8) / 255.0f, ((c & 0x00FF) >>  0) / 255.0f, ((x & 0x00FF) >>  0) / 255.0f);
}

void main() {
  vec2 ac = {0.0f, 0.0f};

  /*
  vec3 c = {1.0f, 1.0f, 1.0f};
  if (tc.x >= 0.75) {
    if (tc.y >= 0.66) {
      c = h2c3(0xcccc, 0xcc);
    } else if (tc.y >= 0.33) {
      c = h2c3(0xfff3, 0xa6);
    } else {
      c = h2c3(0x1212, 0x12);
    }
  } else if (tc.x >= 0.5) {
    if (tc.y >= 0.66) {
      c = h2c3(0xa1e6, 0xb1);
    } else if (tc.y >= 0.33) {
      c = h2c3(0x02ff, 0x00);
    } else {
      c = h2c3(0xff40, 0x00);
    }
  } else if (tc.x >= 0.25) {
    if (tc.y >= 0.66) {
      c = h2c3(0x8990, 0xea);
    } else if (tc.y >= 0.33) {
      c = h2c3(0x0013, 0xff);
    } else {
      c = h2c3(0xffd6, 0x00);
    }
  } else {
    if (tc.y >= 0.66) {
      c = h2c3(0x924e, 0xc5);
    } else if (tc.y >= 0.33) {
      c = h2c3(0xff00, 0xd6);
    } else {
      c = h2c3(0x00ff, 0xff);
    }
  }
  FragColor = vec4(c, 1.0f);
  */

  vec3 point = {10000.0f, 10000.0f, 0.0f};

  if (tc.x <= 0.25) { /// 1
    point.x = tc.x * 8 - 1;
    point.y = (tc.y - 0.33) * 6 - 1;
    point.z = -1;
  } else if (tc.x < 0.50) {
    if (tc.y <= 0.33) { /// 5
      point.x = (tc.y) * 6 - 1;
      point.y = -1;
      point.z = (tc.x - 0.25) * 8 - 1;
    } else if (tc.y <= 0.66) { /// 2
      point.x = 1;
      point.y = (tc.y - 0.33) * 6 - 1;
      point.z = (tc.x - 0.25) * 8 - 1;
    } else { /// 6
      point.x = -((tc.y - 0.66) * 6 - 1);
      point.y = 1;
      point.z = ((tc.x - 0.25) * 8 - 1);
    }
  } else if (tc.x <= 0.75) { /// 3
    point.x = -((tc.x - 0.50) * 8 - 1);
    point.y = (tc.y - 0.33) * 6 - 1;
    point.z = 1; 
  } else if (tc.x <= 1.0) { /// 4
    point.x = -1;
    point.y = (tc.y - 0.33) * 6 - 1;
    point.z = -((tc.x - 0.75) * 8 - 1);
  }

  point = normalize(point);
  vec3 vu = vec3(0.0f, 0.0f, 1.0f);
  vec3 vl = vec3(1.0f, 0.0f, 0.0f);
  vec2 pos;
  pos.x = acos(dot(vu, normalize(vec3(0.0f, 0.0f, point.z))));
  pos.y = acos(dot(vl, normalize(vec3(0.0f, point.yz))));



  if (abs(pos.x - 1.57079f) < 0.0001f) {
    FragColor = vec4(vec3(1.0f), 1.0f);
  } else {
    FragColor = vec4(vec3(0.0f), 1.0f);
  }
  FragColor = vec4(vec3(pos.x), 1.0f);
  return;


  vec3 fcol = vec3(0.0f);
  vec3 horizon = h2c3(0xa5c1, 0xd7);
  vec3 sky = h2c3(0x0b0c, 0x8c);
  if (pos.x >= 0.5) {
    fcol = mix(horizon, sky, (pos.x - 0.5));
  } else {
    float x = pos.x * 2;
    float f = x*x*x*x* -3.71435 + x*x*x * 7.49902 - x*x * 5.10417 + x * 2.31549 + 0.00360439;
    fcol = mix(horizon, sky, 1 - f);
  }



  FragColor = vec4(fcol, 1.0f);
}
