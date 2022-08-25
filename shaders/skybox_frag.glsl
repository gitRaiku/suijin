#version 460 core
#define PI 3.1415

out vec4 FragColor;

in vec2 tc;

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

  if (tc.x <= 0.25) {
    point.x = tc.x * 8 - 1;
    point.z = -1;
  } else if (tc.x < 0.50) {
    //  point.y = 0.4f;
    if (tc.y <= 0.33) {

    } else if (tc.y <= 0.66) {
      point.x = 1;
      point.z = (tc.x - 0.25) * 8 - 1;
    } else {

    }
  } else if (tc.x <= 0.75) {
    point.x = 1 - ((tc.x - 0.50) * 8 - 1);
    point.z = 1;
  } else if (tc.x <= 1.0) {
    point.x = -1;
    point.z = (tc.x - 0.75) * 8 - 1;
  }

  /*if (tc.x >= 0.25 && tc.x <= 0.50) {
    ac.y = tc.y * 0.75;
  } else if (tc.x >= 0.75) {
    ac.y = 1.2475 - tc.y * 0.75;
  }*/

  if (point.x == 10000.0f) {
    FragColor = vec4(vec3(0.2f), 1.0f);
    return;
  }


  //FragColor = vec4(ac.x, ac.x, ac.x, 1.0f);
  FragColor = vec4(point.x / 2 + 1, point.z / 2 + 1, 0.0f, 1.0f);
}
