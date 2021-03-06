#version 130

// interpolated
in vec4 norm;
in vec4 light_dir;
in vec4 viewer_dir;

// uniforms that describe the light and material parameters
uniform vec4 lamb;
uniform vec4 ldiff;
uniform vec4 lspec;

uniform vec4 mamb;
uniform vec4 mdiff;
uniform vec4 mspec;

uniform float ms;

void main() 
{

  vec4 n = normalize(norm);
  vec4 ld = normalize(light_dir);
  vec4 vd = normalize(viewer_dir);

  // compute these three terms:
  vec4 ambient_color, diffuse_color, specular_color;

  // first, ambient light
  ambient_color = lamb * mamb;

  // next, diffuse
  float dd = dot(ld, n);
  if (dd > 0.0) diffuse_color = dd * (ldiff * mdiff);
  else diffuse_color = vec4(0.0, 0.0, 0.0, 1.0);

  // last, specular
  float sd = dot(normalize(ld + vd), n);
  if (sd > 0.0) specular_color = pow(sd, ms) * (lspec * mspec);
  else specular_color = vec4(0.0, 0.0, 0.0, 1.0);

  // "gl_FragColor" is already defined for us - it's the one thing you have
  // to set in the fragment shader:
  gl_FragColor = ambient_color + diffuse_color + specular_color;
  gl_FragColor[3] = 1.0;

}

