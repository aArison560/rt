#pragma once

#include "core/Vec3.hpp"
#include "core/Ray.hpp"
#include "core/HitRecord.hpp"
#include "geometry/AObject.hpp"
#include <memory>
#include <optional>
#include <vector>
#include <algorithm>

struct AABB
{
    Vec3 min;
    Vec3 max;

    AABB();
    AABB(const Vec3& min, const Vec3& max);
    AABB(const AABB& a, const AABB& b);

    void expand(const AABB& other);
    [[nodiscard]] double surfaceArea() const;
    int longestAxis() const;
    Vec3 centroid() const;
    bool hit(const Ray& ray, double tMin, double tMax) const;
};

class BVHNode
{
public:
    [[nodiscard]] std::optional<HitRecord> hit(const Ray& ray, double tMin, double tMax) const;

    static std::unique_ptr<BVHNode> build(
        std::vector<std::shared_ptr<AObject>>& objects,
        size_t start, size_t end);

private:
    AABB box;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    std::vector<std::shared_ptr<AObject>> objects;
};
