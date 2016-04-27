//
// Created by lihe on 4/27/16.
//

#ifndef GLRENDER_BEZIERSURFACE_H
#define GLRENDER_BEZIERSURFACE_H

#include <string>
#include <vector>
#include <fstream>

#include "amath.h"

class BezierSurface {
public:
    typedef amath::vec4 point;

    BezierSurface(const std::vector<float> &points, int u_deg, int v_deg);

    void eval_bezier(const std::vector<point> &controlpoints, int degree, const float t, point &pnt, vec4 &tangent);

    void eval_sample(float u_samp, float v_samp, point &pnt, vec4 &norm);

    void eval_surface(int samples, std::vector<vec4> &points, std::vector<vec4> &norms);

    inline int u_deg() const {
        return _u_deg;
    }

    inline int v_deg() const {
        return _v_deg;
    }

private:
    std::vector<point> get_column(int i) const{
        std::vector<point> column;
        for (auto &row : _control_points) {
            column.push_back(row[i]);
        }
        return std::move(column);
    }

    std::vector<std::vector<point> > _control_points;
    int _u_deg;
    int _v_deg;
};

void parse_bezier_surface(const std::string &file_path, std::vector<BezierSurface> &surfaces);

#endif //GLRENDER_BEZIERSURFACE_H
