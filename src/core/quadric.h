#pragma once
#include "mesh.h"

// Given black box: stores a sum of squared distances to planes.
// Adding two quadrics adds their planes. Use evaluate(Q, position).
// Storage details below are only needed for the optional competition idea.
// A symmetric 4x4 matrix needs only these ten upper-triangle entries.
// Order: xx xy xz xw yy yz yw zz zw ww. Use double for plane sums.
struct Quadric {
    double a[10]{};
};

inline Quadric operator+(const Quadric& left, const Quadric& right) {
    Quadric result;
    for (int i = 0; i < 10; ++i) result.a[i] = left.a[i] + right.a[i];
    return result;
}

// Return p * p^T for the plane vector p = (a,b,c,d).
inline Quadric outer(double a, double b, double c, double d) {
    return {{a * a, a * b, a * c, a * d, b * b, b * c, b * d, c * c, c * d, d * d}};
}

// Evaluate (x,y,z,1)^T Q (x,y,z,1): the sum of squared plane distances.
inline double evaluate(const Quadric& quadric, vec3 position) {
    double x = position.x;
    double y = position.y;
    double z = position.z;
    const double* a = quadric.a;
    return a[0] * x * x + 2 * a[1] * x * y + 2 * a[2] * x * z + 2 * a[3] * x
         + a[4] * y * y + 2 * a[5] * y * z + 2 * a[6] * y + a[7] * z * z + 2 * a[8] * z + a[9];
}

// Read Q = [A b; b^T c]. The minimum solves A * v = -b.
inline void quadric_parts(const Quadric& Q, double A[3][3], double b[3]) {
    A[0][0] = Q.a[0];
    A[0][1] = A[1][0] = Q.a[1];
    A[0][2] = A[2][0] = Q.a[2];
    A[1][1] = Q.a[4];
    A[1][2] = A[2][1] = Q.a[5];
    A[2][2] = Q.a[7];
    b[0] = Q.a[3];
    b[1] = Q.a[6];
    b[2] = Q.a[8];
}

inline double determinant3(const double A[3][3]) {
    return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1])
         - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0])
         + A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
}

// Solve A * x = b by Cramer's rule. A nearly singular matrix has no safe answer.
inline bool solve3x3(const double A[3][3], const double b[3], double x[3]) {
    double determinant = determinant3(A);
    if (fabs(determinant) < 1e-10) return false;
    for (int column = 0; column < 3; ++column) {
        double replaced[3][3];
        for (int row = 0; row < 3; ++row) {
            for (int j = 0; j < 3; ++j) replaced[row][j] = j == column ? b[row] : A[row][j];
        }
        x[column] = determinant3(replaced) / determinant;
    }
    return true;
}

// A unit normal and offset define signed distance dot(normal, v) + d.
// The all-zero plane safely ignores a degenerate triangle.
struct Plane {
    vec3 normal{};
    double d = 0;
};
// Given: find a unit plane from two edges; ignore degenerate triangles.
inline Plane triangle_plane(vec3 p0, vec3 p1, vec3 p2) {
    vec3 normal = cross(p1 - p0, p2 - p0);
    if (length(normal) < 1e-12f) return {};
    normal = normalize(normal);
    double d = -dot(normal, p0);
    return {normal, d};
}
// Given: turn the given plane into the internal quadric.
inline Quadric plane_quadric(vec3 p0, vec3 p1, vec3 p2) {
    Plane plane = triangle_plane(p0, p1, p2);
    return outer(plane.normal.x, plane.normal.y, plane.normal.z, plane.d);
}
double collapse_cost(const Quadric&, vec3, vec3, vec3*);
double extra_cost(const Mesh&, int, int, vec3);
