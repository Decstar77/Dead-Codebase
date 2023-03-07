#pragma once

#include "AttoLib.h"

namespace atto
{
    inline f32 Lerp(const f32 a, const f32 b, const f32 t) {
        return a + (b - a) * t;
    }

    inline glm::vec2 ClampLength(glm::vec2 v, f32 maxLength) {
        f32 magSqrd = glm::dot(v, v);
        if (magSqrd > maxLength * maxLength) {
            return glm::normalize(v) * maxLength;
        }

        return v;
    }

    inline bool ApproxEqual(const f32 a, const f32 b, const f32 epsilon = 0.0001f) {
        return glm::abs(a - b) < epsilon;
    }

    struct Ray2D {
        glm::vec2 origin;
        glm::vec2 direction;
    };

    struct Manifold {
        f32 penetration;
        glm::vec2 pointA;
        glm::vec2 pointB;
        glm::vec2 normal;
    };


    struct Circle {
        glm::vec2   pos;
        f32         rad;

        bool                    Intersects(const Circle& circle);
        bool                    Collision(const Circle& other, Manifold& manifold) const;
    };

    struct Box {
        glm::vec2  pos;
        glm::vec2  dim;
        f32        rot;

        bool                    Intersects(const Box& other) const;
        bool                    Collision(const Circle& other, Manifold& manifold) const;
    };

    struct PolygonCollider {
        FixedList<glm::vec2, 8> vertices;
        void                   Translate(const glm::vec2& translation);
    };

    struct BoxBounds {
        glm::vec2 min;
        glm::vec2 max;

        void                    Translate(const glm::vec2& translation);
        bool                    Intersects(const BoxBounds& other) const;
        bool                    Collision(const BoxBounds& other, Manifold &manifold) const;
        bool                    Contains(const glm::vec2& point) const;

        void                    CreateFromCenterSize(const glm::vec2& center, const glm::vec2& size);
    };

    struct RayInfo {
        f32 t;
        glm::vec2 normal;
        glm::vec2 point;
    };

    class CollisionTests {
    public:
        static bool CirclePoly(const Circle &circle, const PolygonCollider &poly, Manifold& manifold);
    };

    class Geometry {
    public:
        static void SortPointsIntoClockWiseOrder(glm::vec2* vertices, i32 verticesCount);
        static i32  Triangulate(PolygonCollider poly, glm::vec2* outVertices, i32 outVerticesCapcity);
    };
}
