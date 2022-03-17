#version 460 core

in vec3 passCol;

out vec4 FragColor;

void main() {
    FragColor = vec4(passCol, 0.5f);
}
