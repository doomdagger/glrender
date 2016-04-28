//
// Created by lihe on 4/27/16.
//

#include "beziersurface.h"

void parse_bezier_surface(const std::string &file_path, std::vector<BezierSurface> &surfaces) {
    std::ifstream file(file_path.c_str());

    if (!file.good()) {
        std::cerr << "Fail to read bezier surface file " << file_path << std::endl;
        return;
    }

    surfaces.clear();

    int num_surface;
    file >> num_surface;

    while (num_surface--) {
        int u_deg, v_deg;
        file >> u_deg >> v_deg;

        float x, y, z;
        std::vector<float> control_points;
        for (int i = 0; i <= v_deg; ++i) {
            for (int j = 0; j <= u_deg; ++j) {
                file >> x >> y >> z;
                control_points.push_back(x);
                control_points.push_back(y);
                control_points.push_back(z);
            }
        }
        surfaces.push_back(BezierSurface(control_points, u_deg, v_deg));
    }

    file.close();
}

BezierSurface::BezierSurface(const std::vector<float> &points, int u_deg, int v_deg)
        : _control_points(), _u_deg(u_deg), _v_deg(v_deg) {
    int index;
    for (int i = 0; i <= v_deg; ++i) {
        std::vector<point> row;
        for (int j = 0; j <= u_deg; ++j) {
            index = i * ((u_deg + 1) * 3) + j * 3;
            row.push_back(point(points[index], points[index + 1], points[index + 2], 1));
        }
        _control_points.push_back(row);
    }
}

void BezierSurface::eval_bezier(const std::vector<point> &controlpoints, int degree, const float t, point &pnt,
                             vec4 &tangent) {
    std::vector<point> temp;
    for (auto &elem : controlpoints) {
        temp.push_back(elem);
    }

    int start_index = 0;

    point prev;

    while (start_index < degree) {
        prev = temp[start_index];
        for (int i = start_index + 1; i <= degree; ++i) {
            point cur = temp[i];
            temp[i] = point(cur.x * t + prev.x * (1 - t), cur.y * t + prev.y * (1 - t), cur.z * t + prev.z * (1 - t), 1);
            prev = cur;
        }
        ++start_index;
    }

    pnt.x = temp[degree].x;
    pnt.y = temp[degree].y;
    pnt.z = temp[degree].z;
    pnt.w = 1;

    tangent.x = prev.x - temp[degree - 1].x;
    tangent.y = prev.y - temp[degree - 1].y;
    tangent.z = prev.z - temp[degree - 1].z;
    tangent.w = 0;

}

void BezierSurface::eval_sample(float u_samp, float v_samp, point &pnt, vec4 &norm) {
    std::vector<point> controlpoints;

    // sweep out control points b0, b1, ..., bm to collect control points
    vec4 tangent;
    for (int i = 0; i <= _v_deg; ++i) {
        point temp;
        eval_bezier(_control_points[i], _u_deg, u_samp, temp, tangent);
        controlpoints.push_back(temp);
    }

    point u_v;
    vec4 v_tan;
    eval_bezier(controlpoints, _v_deg, 1 - v_samp, u_v, v_tan);

    controlpoints.clear();


    for (int i = 0; i <= _u_deg; ++i) {
        point temp;
        get_column(i);
        eval_bezier(get_column(i), _v_deg, 1 - v_samp, temp, tangent);
        controlpoints.push_back(temp);
    }

    point redundant;
    vec4 u_tan;
    eval_bezier(controlpoints, _u_deg, u_samp, redundant, u_tan);

    pnt.x = u_v.x;
    pnt.y = u_v.y;
    pnt.z = u_v.z;
    pnt.w = 1;

    vec4 ret(normalize(cross(u_tan, v_tan)), 0);
    norm.x = ret.x;
    norm.y = ret.y;
    norm.z = ret.z;
    norm.w = 0;
}

void BezierSurface::eval_surface(int samples, std::vector<vec4> &points, std::vector<vec4> &norms) {
    points.clear();
    norms.clear();

    int u_sample_num = samples * _u_deg + 1;
    int v_sample_num = samples * _v_deg + 1;

    float u_sample_step = 1.0f / (u_sample_num - 1);
    float v_sample_step = 1.0f / (v_sample_num - 1);

    float u_sample, v_sample;

    for (int i = 0; i < v_sample_num; ++i) {
        v_sample = i * v_sample_step;
        for (int j = 0; j < u_sample_num; ++j) {
            u_sample = j * u_sample_step;
            vec4 temp_p;
            vec4 temp_n;
            eval_sample(u_sample, v_sample, temp_p, temp_n);
            points.push_back(temp_p);
            norms.push_back(temp_n);
        }
    }
}






