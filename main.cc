// Very simple display triangle program, that allows you to rotate the
// triangle around the Y axis.
//
// This program does NOT use a vertex shader to define the vertex colors.
// Instead, it computes the colors in the display callback (using Blinn/Phong)
// and passes those color values, one per vertex, to the vertex shader, which
// passes them directly to the fragment shader. This achieves what is called
// "gouraud shading".

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else

#include <GL/glew.h>
#include <GL/freeglut.h>

#endif

#include "amath.h"
#include "misc.h"

// type alias
typedef amath::vec4 point4;

// variables need to be initialized
int NumVertices = 0;
point4 *vertices = NULL;
vec4 *norms = NULL;

// viewer's position, for lighting calculations
vec4 viewer;
vec4 origin(0.0, 0.0, 0.0, 1.0);

float thetax = 90.0;  // rotation around the X axis, start from horizontal
float thetay = 0.0;  // rotation around the Y (up) axis
float radius = 8.0;  // distance between fixed origin and camera

// variables for opengl
GLuint buffers[2];
GLint pos, ctm, ptm;
GLuint program; //shaders


// initialize all dynamic data
// compute all these norms
// The easiest way to compute these normals is as follows:
// 1. make an array of normals that contain the normals for each triangle: e.g. tri_norms[] (computed via crossproduct)
// 2. make an array of vectors, one for each unique vertex, each initialized to the zero vector, e.g. vert_norms[]
// 3. go through the array of triangle vertex ids:
//  if triangle i has vertex j, add tri_norms[i] to vert_nroms[j] (you will be adding each triangle's normal to 3 different vertex normals)
// 4. when done, normalize all the vert_norms.
void init_all_data(const std::string &file) {
    std::vector<int> tris;
    std::vector<float> verts;

    parseObjFile(file, tris, verts);

    NumVertices = (int) tris.size();
    vertices = new point4[NumVertices];
    norms = new vec4[NumVertices];                      // norms per vertex per triangle

    std::vector<vec4> tri_norms(tris.size() / 3);       // norms per triangle
    std::vector<vec4> vert_norms(verts.size() / 3);     // norms per vertex (vertices are unique)

    int n = NumVertices / 3;
    for (int i = 0; i < n; ++i) {
        // get the vertices
        vertices[3 * i] = point4(verts[3 * tris[3 * i]],
                                 verts[3 * tris[3 * i] + 1],
                                 verts[3 * tris[3 * i] + 2], 1.0);
        vertices[3 * i + 1] = point4(verts[3 * tris[3 * i + 1]],
                                     verts[3 * tris[3 * i + 1] + 1],
                                     verts[3 * tris[3 * i + 1] + 2], 1.0);
        vertices[3 * i + 2] = point4(verts[3 * tris[3 * i + 2]],
                                     verts[3 * tris[3 * i + 2] + 1],
                                     verts[3 * tris[3 * i + 2] + 2], 1.0);

        tri_norms[i] = normalize(
                vec4(cross(vertices[3 * i + 1] - vertices[3 * i], vertices[3 * i + 2] - vertices[3 * i + 1]), 0.0));
        vert_norms[tris[3 * i]] += tri_norms[i];
        vert_norms[tris[3 * i + 1]] += tri_norms[i];
        vert_norms[tris[3 * i + 2]] += tri_norms[i];
    }

    for (int i = 0; i < vert_norms.size(); i++) {
        vert_norms[i] = normalize(vert_norms[i]);
    }

    for (int i = 0; i < n; i++) {
        norms[3 * i] = vert_norms[tris[3 * i]];
        norms[3 * i + 1] = vert_norms[tris[3 * i + 1]];
        norms[3 * i + 2] = vert_norms[tris[3 * i + 2]];
    }
}


// initialization: set up a Vertex Array Object (VAO) and then
void init() {

    // create a vertex array object - this defines mameory that is stored
    // directly on the GPU
    GLuint vao;

    // deending on which version of the mac OS you have, you may have to do this:
#ifdef __APPLE__
    glGenVertexArraysAPPLE( 1, &vao );  // give us 1 VAO:
    glBindVertexArrayAPPLE( vao );      // make it active
#else
    glGenVertexArrays(1, &vao);   // give us 1 VAO:
    glBindVertexArray(vao);       // make it active
#endif

    // set up vertex buffer object - this will be memory on the GPU where
    // we are going to store our vertex data (that is currently in the "points"
    // array)
    glGenBuffers(1, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);  // make it active

    // specify that its part of a VAO, what its size is, and where the
    // data is located, and finally a "hint" about how we are going to use
    // the data (the driver will put it in a good memory location, hopefully)
    // vertices position, and normals
    glBufferData(GL_ARRAY_BUFFER, sizeof(point4) * NumVertices + sizeof(vec4) * NumVertices, NULL, GL_STATIC_DRAW);

    // load in these two shaders...  (note: InitShader is defined in the
    // accompanying initshader.c code).
    // the shaders themselves must be text glsl files in the same directory
    // as we are running this program:
    program = InitShader("vshader.glsl", "fshader_passthrough.glsl");

    // ...and set them to be active
    glUseProgram(program);


    // this time, we are sending TWO attributes through: the position of each
    // transformed vertex, and its normal.
    GLuint loc, loc2;

    loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);

    // the vPosition attribute is a series of 4-vecs of floats, starting at the
    // beginning of the buffer
    glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    loc2 = glGetAttribLocation(program, "vNorm");
    glEnableVertexAttribArray(loc2);

    // the vColor attribute is a series of 4-vecs of floats, starting just after
    // the points in the buffer
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * NumVertices));

    // display callback
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4) * NumVertices, vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4) * NumVertices, sizeof(vec4) * NumVertices, norms);

    // all uniform variables
    pos = glGetUniformLocation(program, "pos");
    ctm = glGetUniformLocation(program, "ctm");
    ptm = glGetUniformLocation(program, "ptm");

    // set the background color (white)
    glClearColor(1.0, 1.0, 1.0, 1.0);
}


void display(void) {

    // clear the window (with white) and clear the z-buffer (which isn't used
    // for this example).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // based on where the mouse has moved to:
    GLfloat anglex = DegreesToRadians * thetax;
    GLfloat angley = DegreesToRadians * thetay;
    viewer = vec4(sinf(anglex) * sinf(angley) * radius, cosf(anglex) * radius, sinf(anglex) * cosf(angley) * radius,
                  1.0);
    vec4 v_o = normalize(origin - viewer);
    vec4 v = normalize(cross(v_o, vec4(0.0, 1.0, 0.0, 0.0)));
    vec4 u = normalize(cross(v, v_o));

    glUniform4fv(pos, 1, viewer);

    glUniformMatrix4fv(ctm, 1, GL_TRUE, LookAt(viewer, origin, u));
    glUniformMatrix4fv(ptm, 1, GL_TRUE, Perspective(40, 1.0, 1, 50));

    // draw the VAO:
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);


    // move the buffer we drew into to the screen, and give us access to the one
    // that was there before:
    glutSwapBuffers();
}


// use this motionfunc to demonstrate rotation - it adjusts "theta" based
// on how the mouse has moved. Theta is then used the the display callback
// to generate the transformation, ctm, that is applied
// to all the vertices before they are displayed:
void mouse_move_rotate(int x, int y) {

    static int lastx = 0;// keep track of where the mouse was along x axis
    static int lasty = 0;// keep track of where the mouse was along y axis

    int amntX = x - lastx;
    int amntY = y - lasty;

    if (amntX != 0) {
        thetay += amntX;
        if (thetay > 360.0) thetay -= 360.0;
        if (thetay < 0.0) thetay += 360.0;

        lastx = x;
    }

    if (amntY != 0) {
        thetax += amntY;
        if (thetax > 175) thetax = 175;
        if (thetax < 5) thetax = 5;

        lasty = y;
    }

    // force the display routine to be called as soon as possible:
    glutPostRedisplay();

}


// the keyboard callback, called whenever the user types something with the
// regular keys.
void mykey(unsigned char key, int mousex, int mousey) {
    if (key == 'q' || key == 'Q') {
        delete [] vertices;
        delete [] norms;
        exit(0);
    }

    // and r resets the view:
    if (key == 'r') {
        thetax = 90.0;
        thetay = 0.0;
        radius = 8.0;
        glutPostRedisplay();
    }

    // move closer
    if (key == 'z' && radius > 2) {
        radius *= 0.9;
        glutPostRedisplay();
    }

    // move farther
    if (key == 'x' && radius < 50) {
        radius *= 1.1;
        glutPostRedisplay();
    }
}


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: glrender OBJ_FILE" << std::endl;
        return -1;
    }
    init_all_data(argv[1]);

    // initialize glut, and set the display modes
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);

    // give us a window in which to display, and set its title:
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rotate / Translate Triangle");

    // for displaying things, here is the callback specification:
    glutDisplayFunc(display);

    // when the mouse is moved, call this function!
    // you can change this to mouse_move_translate to see how it works
    glutMotionFunc(mouse_move_rotate);

    // for any keyboard activity, here is the callback:
    glutKeyboardFunc(mykey);

#ifndef __APPLE__
    // initialize the extension manager: sometimes needed, sometimes not!
    glewInit();
#endif

    // call the init() function, defined above:
    init();

    // enable the z-buffer for hidden surface removel:
    glEnable(GL_DEPTH_TEST);

    // once we call this, we no longer have control except through the callbacks:
    glutMainLoop();

    // clean up all the memory allocation on heap
    delete[] vertices;
    delete[] norms;
    return 0;
}