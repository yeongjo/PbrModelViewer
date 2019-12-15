#version 420

#define MAX_N 5000
#define M 15.0

vec2 resolution = vec2(800,600);
uniform vec2 offset;
uniform float zoom;
uniform vec2 jc;

in vec3 WorldPos;

layout(location = 0) out vec4 out_color;

vec2 apply_transformations(vec2 v);
int mandelbrot_iteration_count(vec2 c);
float modulus(vec2 v);
vec2 complex_product(vec2 a, vec2 b);

void main() {
  vec2 point = apply_transformations(WorldPos.xz);
  int n = mandelbrot_iteration_count(point);
  float c;
  if (n == MAX_N) {
    c = 0.0f;
  } else {
    c = float(n)/M;
  }
  out_color = vec4(c, 0.33f*c, 0.0f, 1.0f);
  //out_color = vec4(point,0,1);
}

vec2 apply_transformations(vec2 v) {
  float ratio = 1;//resolution.x/resolution.y;
  return zoom * (v * vec2(ratio, 1.0)) + offset;
}

int mandelbrot_iteration_count(vec2 c) {
  vec2 z = c;
  int n = 0;
  while (modulus(z) < 4.0f && n < MAX_N) {
    z = complex_product(z, z) + jc;
    n++;
  }
  return n;
}

float modulus(vec2 v) {
  vec2 w = v*v;
  return w.x + w.y;
}

vec2 complex_product(vec2 a, vec2 b) {
  return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}