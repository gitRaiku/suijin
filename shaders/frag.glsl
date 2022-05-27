#version 460 core

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 spec;
  vec3 emmisive;
  float spece;
  float transp;
  float optd;
  int illum;
};

in vec3 passNorm;

out vec4 FragColor;

uniform int shading;
uniform vec3 camPos;
uniform Material mat;

const float atten_const = 1.0f;
const float atten_linear = 0.009f;
const float atten_quad = 0.00032f;
const vec3 lpos = vec3(1.2f);

void main() {
  if (shading == 0) {
    float dist = length(lpos - gl_FragCoord.xyz);
    float attenuation = 1.0f / (atten_const + atten_linear * dist + atten_quad * dist * dist);

    vec3 ambient = mat.ambient * vec3(0.5f);

    vec3 nVec = normalize(passNorm);
    vec3 dir = normalize(lpos - gl_FragCoord.xyz);
    float diff = max(dot(nVec, dir), 0.0f);
    vec3 diffuse = mat.diffuse * (diff * vec3(0.3f));

    vec3 viewDir = normalize(camPos - gl_FragCoord.xyz);
    vec3 reflectDir = reflect(-dir, nVec);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), mat.spece);
    vec3 specular = mat.spec * spec * vec3(0.5f);

    vec3 _result = (ambient + diffuse + specular) * attenuation;
    FragColor = vec4(_result, 1.0f);
  } else {
    FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
  }
}
