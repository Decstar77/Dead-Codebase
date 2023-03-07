#include "AttoMath.h"

namespace atto
{
    bool Circle::Intersects(const Circle& circle) {
        f32 distSqrd = glm::distance2(pos, circle.pos);
        f32 radSum = rad + circle.rad;
        return distSqrd < radSum* radSum;
    }

    bool Circle::Collision(const Circle& other, Manifold& manifold) const {
        glm::vec2 normal = other.pos - pos;
        f32 dist = glm::length(normal);
        f32 radSum = rad + other.rad;
        if (dist > radSum) {
            return false;
        }

        manifold.normal = glm::normalize(normal);
        manifold.penetration = radSum - dist;
        manifold.pointA = pos + manifold.normal * rad;
        manifold.pointB = other.pos - manifold.normal * other.rad;
        return true;
    }

    void BoxBounds::Translate(const glm::vec2& translation) {
        min += translation;
        max += translation;
    }

    void BoxBounds::CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size) {
        max = center + size * 0.5f;
        min = center - size * 0.5f;
    }

    //bool RayCast::Box(const BoxBounds& b, const Ray2D& r, f32& t) {
    //    f32 tx1 = (b.min.x - r.origin.x) * (1.0f / r.direction.x);
    //    f32 tx2 = (b.max.x - r.origin.x) * (1.0f / r.direction.x);
    //
    //    f32 tmin = glm::min(tx1, tx2);
    //    f32 tmax = glm::max(tx1, tx2);
    //
    //    f32 ty1 = (b.min.y - r.origin.y) * (1.0f / r.direction.y);
    //    f32 ty2 = (b.max.y - r.origin.y) * (1.0f / r.direction.y);
    //
    //    tmin = glm::max(tmin, glm::min(ty1, ty2));
    //    tmax = glm::min(tmax, glm::max(ty1, ty2));
    //
    //    t = tmin;
    //
    //    return tmax >= tmin;
    //}

    bool BoxBounds::Intersects(const BoxBounds& other) const {
        return (max.x >= other.min.x && min.x <= other.max.x) &&
            (max.y >= other.min.y && min.y <= other.max.y);
    }

    bool BoxBounds::Collision(const BoxBounds& other, Manifold& manifold) const {
        if (Intersects(other)) {
            f32 xOverlap = glm::min(max.x, other.max.x) - glm::max(min.x, other.min.x);
            f32 yOverlap = glm::min(max.y, other.max.y) - glm::max(min.y, other.min.y);

            if (xOverlap < yOverlap) {
                if (max.x > other.max.x) {
                    manifold.normal = glm::vec2(-1.0f, 0.0f);
                }
                else {
                    manifold.normal = glm::vec2(1.0f, 0.0f);
                }

                manifold.penetration = xOverlap;
            }
            else {
                if (max.y > other.max.y) {
                    manifold.normal = glm::vec2(0.0f, -1.0f);
                }
                else {
                    manifold.normal = glm::vec2(0.0f, 1.0f);
                }

                manifold.penetration = yOverlap;
            }

            return true;
        }

        return false;
    }

    bool BoxBounds::Contains(const glm::vec2& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
            (point.y >= min.y && point.y <= max.y);
    }

    bool CollisionTests::CirclePoly(const Circle& circle, const PolygonCollider& poly, Manifold& manifold) {
        f32 minDist = FLT_MAX;
        glm::vec2 closestPoint = {};
        glm::vec2 closestNormal = {};
        f32 closestDist = FLT_MAX;
        glm::vec2 closestPointOnPoly = {};

        const i32 count = poly.vertices.GetCount();
        for (i32 i = 0; i < count; ++i) {
            glm::vec2 edge = poly.vertices[(i + 1) % count] - poly.vertices[i];
            glm::vec2 normal = glm::normalize(glm::vec2(-edge.y, edge.x));
            glm::vec2 pointOnPoly = poly.vertices[i] + normal * glm::dot(normal, circle.pos - poly.vertices[i]);

            f32 dist = glm::distance(circle.pos, pointOnPoly);
            if (dist < circle.rad) {
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPointOnPoly = pointOnPoly;
                    closestNormal = normal;
                }
            }
        }

        if (closestDist != FLT_MAX) {
            manifold.normal = closestNormal;
            manifold.penetration = circle.rad - closestDist;
            manifold.pointA = circle.pos - closestNormal * circle.rad;
            manifold.pointB = closestPointOnPoly;
            return true;
        }

        return false;
    }

    inline static f32 Determinant(glm::vec2 u, glm::vec2 v) {
        f32 result = u.x * v.y - u.y * v.x;
        return result;
    }
    
    inline bool CollisionTrianglePoint(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 point) {
        glm::vec2 ab = b - a;
        glm::vec2 bc = c - b;
        glm::vec2 ca = a - c;

        if (Determinant(ab, point - a) < 0 && Determinant(bc, point - b) < 0 && Determinant(ca, point - c) < 0) {
            return true;
        }

        return false;
    }

    void PolygonCollider::Translate(const glm::vec2& translation) {
        const i32 count = vertices.GetCount();
        for (i32 i = 0; i < count; ++i) {
            vertices[i] += translation;
        }
    }

    static i32 CompareVerticesClockwise(void* context, const void* a, const void* b) {
        const glm::vec2* va = static_cast<const glm::vec2*>(a);
        const glm::vec2* vb = static_cast<const glm::vec2*>(b);
        glm::vec2 center = *static_cast<glm::vec2*>(context);
        glm::vec2 aDir = *va - center;
        glm::vec2 bDir = *vb - center;
        return std::atan2(aDir.y, aDir.x) < std::atan2(bDir.y, bDir.x) ? -1 : 1;
    }

    void Geometry::SortPointsIntoClockWiseOrder(glm::vec2* vertices, i32 verticesCount) {
        glm::vec2 center(0.0f, 0.0f);
        for (i32 vertexIndex = 0; vertexIndex < verticesCount; ++vertexIndex) {
            center += vertices[vertexIndex];
        }
        center /= (f32)verticesCount;

        qsort_s(vertices, verticesCount, sizeof(glm::vec2), CompareVerticesClockwise, &center);
    }

    i32 Geometry::Triangulate(PolygonCollider poly, glm::vec2* outVertices, i32 outVerticesCapcity) {
        if (poly.vertices.GetCount() < 3) {
            return 0;
        }

        SortPointsIntoClockWiseOrder(poly.vertices.GetData(), poly.vertices.GetCount());

        i32 outVertexIndex = 0;
        if (poly.vertices.GetCount() == 3) {
            outVertices[outVertexIndex] = poly.vertices[0];
            outVertexIndex++;
            outVertices[outVertexIndex] = poly.vertices[1];
            outVertexIndex++;
            outVertices[outVertexIndex] = poly.vertices[2];

            return 3;
        }
        
        bool impossibleToTriangulate = false;
        bool triangleFound = true;

        while (poly.vertices.GetCount() != 0) {
            if (!triangleFound) {
                return outVertexIndex;
            }

            triangleFound = false;

            for (i32 i = 0; i < poly.vertices.GetCount() - 2; i++) {
                if (!triangleFound) {
                    const f32 d = Determinant(poly.vertices[i + 2] - poly.vertices[i + 1], poly.vertices[i + 1] - poly.vertices[i]);
                    if (d < 0) {
                        triangleFound = true;

                        if (outVertexIndex + 3 > outVerticesCapcity) {
                            ATTOERROR("Not enough space to triangulate polygon");
                            return 0;
                        }

                        outVertices[outVertexIndex] = poly.vertices[i];
                        outVertexIndex++;
                        outVertices[outVertexIndex] = poly.vertices[i + 1];
                        outVertexIndex++;
                        outVertices[outVertexIndex] = poly.vertices[i + 2];
                        outVertexIndex++;

                        poly.vertices.RemoveIndex(i + 1);
                    }
                }
            }
        }

        return outVertexIndex;
    }

}
