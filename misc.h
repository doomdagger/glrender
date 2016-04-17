//
// Created by lihe on 4/16/16.
//

#ifndef GLRENDER_MISC_H
#define GLRENDER_MISC_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "amath.h"

// product of components, which we will use for shading calculations:
vec4 product(vec4 a, vec4 b) {
    return vec4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
}

void parseObjFile(const std::string &file, std::vector<int> &tris, std::vector<float> &verts) {
    // clear out the tris and verts vectors:
    tris.clear();
    verts.clear();

    std::ifstream in(file.c_str());

    if (!in.good()) {
        std::cout << "Fails at reading file " << file << std::endl;
    }

    char buffer[1025];
    std::string cmd;

    for (int line = 1; in.good(); line++) {
        in.getline(buffer, 1024);
        buffer[in.gcount()] = 0;

        cmd = "";

        std::istringstream iss(buffer);

        iss >> cmd;

        if (cmd[0] == '#' or cmd.empty()) {
            // ignore comments or blank lines
            continue;
        }
        else if (cmd == "v") {
            // got a vertex:

            // read in the parameters:
            float pa, pb, pc;
            iss >> pa >> pb >> pc;

            verts.push_back(pa);
            verts.push_back(pb);
            verts.push_back(pc);
        }
        else if (cmd == "f") {
            // got a face (triangle)

            // read in the parameters:
            int i, j, k;
            iss >> i >> j >> k;

            // vertex numbers in OBJ files start with 1, but in C++ array
            // indices start with 0, so we're shifting everything down by
            // 1
            tris.push_back(i - 1);
            tris.push_back(j - 1);
            tris.push_back(k - 1);
        }
        else {
            std::cerr << "Parser error: invalid command at line " << line << std::endl;
        }

    }

    in.close();
}


#endif //GLRENDER_MISC_H
