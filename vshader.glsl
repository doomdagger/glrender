#version 130
// we are going to be getting an attribute from the main program, named
// "vPosition", one for each vertex.
in vec4 vPosition;

// we are going to be getting an attribute from the main program, named
// "vNorm", one for each vertex.
in vec4 vNorm;

// camera transform matrix
uniform mat4 ctm;

// projective transform matrix
uniform mat4 ptm;

// camera position
uniform vec4 pos;

// light position
uniform vec4 lpos;

varying vec4 norm;
varying vec4 light_dir;
varying vec4 viewer_dir;

void main()
{
  norm = vNorm;
  light_dir = lpos - vPosition;
  viewer_dir = pos - vPosition;

  gl_Position = ptm * ctm * vPosition;
} 
