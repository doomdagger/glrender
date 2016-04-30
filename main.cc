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

#include <vector>
#include "amath.h"
#include "misc.h"
#include "beziersurface.h"

// type alias
typedef amath::vec4 point4;

// variables need to be initialized
int NumVertices = 0;
point4 *vertices = nullptr;
vec4 *norms = nullptr;

// viewer's position, for lighting calculations
vec4 viewer;
vec4 origin(0.0, 0.0, 0.0, 1.0);

float thetax = 90.0;  // rotation around the X axis, start from horizontal
float thetay = 0.0;  // rotation around the Y (up) axis
float radius = 8.0;  // distance between fixed origin and camera
int lastx = 0;       // keep track of where the mouse was along x axis
int lasty = 0;       // keep track of where the mouse was along y axis
bool changed_sampling_resolution = false;
int sampling_resolution = 1;

bool bezier_file = false;

// light positions (needed for shading) and the material spec:
vec4 light_position = vec4(100.0, 100.0, 100.0, 1.0);
vec4 light_ambient  = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse  = vec4(1.0, 1.0, 1.0, 1.0);
vec4 light_specular = vec4(1.0, 1.0, 1.0, 1.0);

vec4 material_ambient  = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse  = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 0.8, 0.0, 1.0);
float material_shininess = 100.0;

// variables for opengl
GLuint buffers[2];
GLint pos, ctm, ptm, lpos, lamb, ldiff, lspec, mamb, mdiff, mspec, ms;
GLuint program; //shaders

// added bezier support
std::vector<BezierSurface> surfaces;

// initialize all dynamic data
// compute all these norms
// The easiest way to compute these normals is as follows:
// 1. make an array of normals that contain the normals for each triangle: e.g. tri_norms[] (computed via crossproduct)
// 2. make an array of vectors, one for each unique vertex, each initialized to the zero vector, e.g. vert_norms[]
// 3. go through the array of triangle vertex ids:
//  if triangle i has vertex j, add tri_norms[i] to vert_nroms[j] (you will be adding each triangle's normal to 3 different vertex normals)
// 4. when done, normalize all the vert_norms.
void init_obj_vertices_norm(const std::string &file) {
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

    for (size_t i = 0; i < vert_norms.size(); i++) {
        vert_norms[i] = normalize(vert_norms[i]);
    }

    for (int i = 0; i < n; i++) {
        norms[3 * i] = vert_norms[tris[3 * i]];
        norms[3 * i + 1] = vert_norms[tris[3 * i + 1]];
        norms[3 * i + 2] = vert_norms[tris[3 * i + 2]];
    }
}

void reload_vertices_norm() {
    int points_num = 0;
    for (auto &surf : surfaces) {
        points_num += (2 * surf.u_deg() * sampling_resolution) * ((surf.v_deg() * sampling_resolution)) * 3;
    }
    NumVertices = points_num;

    if (vertices) {
        delete[] vertices;
    }
    if (norms) {
        delete[] norms;
    }

    vertices = new point4[NumVertices];
    norms = new point4[NumVertices];

    std::vector<vec4> points_vec;
    std::vector<vec4> norm_vec;

    int vn_index = 0;

    for (auto &surf : surfaces) {
        surf.eval_surface(sampling_resolution, points_vec, norm_vec);

        int u_sample_num = sampling_resolution * surf.u_deg() + 1;
        int v_sample_num = sampling_resolution * surf.v_deg() + 1;

        for (int i = 0; i < v_sample_num - 1; ++i) {
            for (int j = 0; j < u_sample_num - 1; ++j) {
                vec4 tri1_v1 = points_vec[i * u_sample_num + j];
                vec4 tri1_v2 = points_vec[(i + 1) * u_sample_num + j + 1];
                vec4 tri1_v3 = points_vec[(i + 1) * u_sample_num + j];

                vec4 tri2_v1 = tri1_v2;
                vec4 tri2_v2 = tri1_v1;
                vec4 tri2_v3 = points_vec[i * u_sample_num + j + 1];

                vec4 tri1_n1 = norm_vec[i * u_sample_num + j];
                vec4 tri1_n2 = norm_vec[(i + 1) * u_sample_num + j + 1];
                vec4 tri1_n3 = norm_vec[(i + 1) * u_sample_num + j];

                vec4 tri2_n1 = tri1_n2;
                vec4 tri2_n2 = tri1_n1;
                vec4 tri2_n3 = norm_vec[i * u_sample_num + j + 1];

                vertices[vn_index] = tri1_v1;
                vertices[vn_index + 1] = tri1_v2;
                vertices[vn_index + 2] = tri1_v3;

                vertices[vn_index + 3] = tri2_v1;
                vertices[vn_index + 4] = tri2_v2;
                vertices[vn_index + 5] = tri2_v3;

                norms[vn_index] = tri1_n1;
                norms[vn_index + 1] = tri1_n2;
                norms[vn_index + 2] = tri1_n3;

                norms[vn_index + 3] = tri2_n1;
                norms[vn_index + 4] = tri2_n2;
                norms[vn_index + 5] = tri2_n3;

                vn_index += 6;
            }
        }
    }
}


// initialize all dynamic data
void init_bezier_vertices_norm(const std::string &file) {
    parse_bezier_surface(file, surfaces);

    reload_vertices_norm();
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
    program = InitShader("vshader.glsl", "fshader.glsl");

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
    lpos = glGetUniformLocation(program, "lpos");
    lamb = glGetUniformLocation(program, "lamb");
    ldiff = glGetUniformLocation(program, "ldiff");
    lspec = glGetUniformLocation(program, "lspec");
    mamb = glGetUniformLocation(program, "mamb");
    mdiff = glGetUniformLocation(program, "mdiff");
    mspec = glGetUniformLocation(program, "mspec");
    ms = glGetUniformLocation(program, "ms");

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
    vec4 v = normalize(vec4(cross(v_o, vec4(0.0, 1.0, 0.0, 0.0)), 0.0));
    vec4 u = normalize(vec4(cross(v, v_o), 0.0));

    glUniform4fv(pos, 1, viewer);

    glUniformMatrix4fv(ctm, 1, GL_TRUE, LookAt(viewer, origin, u));
    glUniformMatrix4fv(ptm, 1, GL_TRUE, Perspective(40, 1, 1, 51));

    // lpos, lamb, ldiff, lspec, mamb, mdiff, mspec, ms;
    glUniform4fv(lpos, 1, light_position);
    glUniform4fv(lamb, 1, light_ambient);
    glUniform4fv(ldiff, 1, light_diffuse);
    glUniform4fv(lspec, 1, light_specular);
    glUniform4fv(mamb, 1, material_ambient);
    glUniform4fv(mdiff, 1, material_diffuse);
    glUniform4fv(mspec, 1, material_specular);
    glUniform1f(ms, material_shininess);

    if (bezier_file && changed_sampling_resolution) {
        reload_vertices_norm();
        // specify that its part of a VAO, what its size is, and where the
        // data is located, and finally a "hint" about how we are going to use
        // the data (the driver will put it in a good memory location, hopefully)
        // vertices position, and normals
        glBufferData(GL_ARRAY_BUFFER, sizeof(point4) * NumVertices + sizeof(vec4) * NumVertices, NULL, GL_STATIC_DRAW);

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
        changed_sampling_resolution = false;
    }

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

void mouse_enter(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        lastx = x;
        lasty = y;
    }
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

    if (key == '<' && sampling_resolution > 1 && bezier_file) {
        sampling_resolution--;
        changed_sampling_resolution = true;
        glutPostRedisplay();
    }

    if (key == '>' && sampling_resolution < 10 && bezier_file) {
        sampling_resolution++;
        changed_sampling_resolution = true;
        glutPostRedisplay();
    }
}


int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: glrender FILE" << std::endl;
        return -1;
    }

    if (isObjFile(argv[1])) {
        bezier_file = false;
        init_obj_vertices_norm(argv[1]);
    } else {
        bezier_file = true;
        init_bezier_vertices_norm(argv[1]);
    }

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
    glutMouseFunc(mouse_enter);

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