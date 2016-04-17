#version 130
// we are going to be getting an attribute from the main program, named
// "ver_position", one for each vertex.
in vec4 ver_position;

// we are going to be getting an attribute from the main program, named
// "ver_normal", one for each vertex.
in vec4 ver_normal;

// camera position
uniform vec4 pos;

// light positions (needed for shading) and the material spec:
vec4 light_position = vec4(100.0, 100.0, 100.0, 1.0);
vec4 light_ambient  = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

vec4 material_ambient  = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse  = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;

void main()
{

  vec4 light_dir = normalize(light_position - ver_position);
  vec4 viewer_dir = normalize(pos - ver_position);

  // compute these three terms:
  vec4 ambient_color, diffuse_color, specular_color;

  // first, ambient light
  ambient_color = light_ambient * material_ambient;

  // next, diffuse
  float dd = dot(light_dir, ver_normal);
  if (dd > 0.0) diffuse_color = dd * (light_diffuse * material_diffuse);
  else diffuse_color = vec4(0.0, 0.0, 0.0, 1.0);

  // last, specular
  float sd = dot(normalize(light_dir + viewer_dir), ver_normal);
  if (sd > 0.0) specular_color = pow(sd, material_shininess) * (light_specular * material_specular);
  else specular_color = vec4(0.0, 0.0, 0.0, 1.0);

  gl_FragColor = ambient_color + diffuse_color + specular_color;
  gl_FragColor.a = 1.0;
}
