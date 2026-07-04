#include "rendering/BVH.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

AABB::AABB()
    : min(Vec3(1e30, 1e30, 1e30)), max(Vec3(-1e30, -1e30, -1e30)) {}

AABB::AABB(const Vec3& min, const Vec3& max)
    : min(min), max(max) {}

AABB::AABB(const AABB& a, const AABB& b)
    : min(Vec3(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z))),
      max(Vec3(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z))) {}

void AABB::expand(const AABB& other)
{
    min.x = std::min(min.x, other.min.x);
    min.y = std::min(min.y, other.min.y);
    min.z = std::min(min.z, other.min.z);
    max.x = std::max(max.x, other.max.x);
    max.y = std::max(max.y, other.max.y);
    max.z = std::max(max.z, other.max.z);
}

int AABB::longestAxis() const
{
    double dx = max.x - min.x;
    double dy = max.y - min.y;
    double dz = max.z - min.z;
    if (dx > dy && dx > dz) return 0;
    if (dy > dz) return 1;
    return 2;
}

Vec3 AABB::centroid() const
{
    return Vec3((min.x + max.x) * 0.5, (min.y + max.y) * 0.5, (min.z + max.z) * 0.5);
}

bool AABB::hit(const Ray& ray, double tMin, double tMax) const
{
    double t0, t1, invD;

    invD = 1.0 / ray.getDirection().x;
    t0 = (min.x - ray.getOrigin().x) * invD;
    t1 = (max.x - ray.getOrigin().x) * invD;
    if (invD < 0.0) std::swap(t0, t1);
    tMin = std::max(t0, tMin);
    tMax = std::min(t1, tMax);
    if (tMax <= tMin) return false;

    invD = 1.0 / ray.getDirection().y;
    t0 = (min.y - ray.getOrigin().y) * invD;
    t1 = (max.y - ray.getOrigin().y) * invD;
    if (invD < 0.0) std::swap(t0, t1);
    tMin = std::max(t0, tMin);
    tMax = std::min(t1, tMax);
    if (tMax <= tMin) return false;

    invD = 1.0 / ray.getDirection().z;
    t0 = (min.z - ray.getOrigin().z) * invD;
    t1 = (max.z - ray.getOrigin().z) * invD;
    if (invD < 0.0) std::swap(t0, t1);
    tMin = std::max(t0, tMin);
    tMax = std::min(t1, tMax);
    if (tMax <= tMin) return false;

    return true;
}

std::optional<HitRecord> BVHNode::hit(const Ray& ray, double tMin, double tMax) const
{
    if (!box.hit(ray, tMin, tMax)) {
        return std::nullopt;
    }

    if (left || right) {
        auto hitLeft = left ? left->hit(ray, tMin, tMax) : std::nullopt;
        auto hitRight = right ? right->hit(ray, tMin, (hitLeft ? hitLeft->getT() : tMax)) : std::nullopt;

        if (hitLeft && hitRight) {
            return (hitLeft->getT() < hitRight->getT()) ? hitLeft : hitRight;
        }
        return hitLeft ? hitLeft : hitRight;
    }

    std::optional<HitRecord> bestHit;
    for (const auto& obj : objects) {
        auto hr = obj->hit(ray, tMin, tMax);
        if (hr && (!bestHit || hr->getT() < bestHit->getT())) {
            bestHit = hr;
            tMax = hr->getT();
        }
    }
    return bestHit;
}

std::unique_ptr<BVHNode> BVHNode::build(
    std::vector<std::shared_ptr<AObject>>& objects,
    size_t start, size_t end)
{
    auto node = std::make_unique<BVHNode>();

    AABB objBox;
    for (size_t i = start; i < end; ++i) {
        Vec3 minB, maxB;
        objects[i]->getWorldBoundingBox(minB, maxB);
        objBox.expand(AABB(minB, maxB));
    }
    node->box = objBox;

    size_t count = end - start;
    if (count <= 4) {
        node->objects.reserve(count);
        for (size_t i = start; i < end; ++i) {
            node->objects.push_back(objects[i]);
        }
        return node;
    }

    int axis = objBox.longestAxis();
    auto getCentroid = [axis](const std::shared_ptr<AObject>& obj) -> double {
        Vec3 minB, maxB;
        obj->getWorldBoundingBox(minB, maxB);
        switch (axis) {
            case 0: return (minB.x + maxB.x) * 0.5;
            case 1: return (minB.y + maxB.y) * 0.5;
            default: return (minB.z + maxB.z) * 0.5;
        }
    };
    auto comparator = [&](const std::shared_ptr<AObject>& a, const std::shared_ptr<AObject>& b) {
        return getCentroid(a) < getCentroid(b);
    };

    size_t mid = start + count / 2;
    std::nth_element(objects.begin() + static_cast<long>(start),
                     objects.begin() + static_cast<long>(mid),
                     objects.begin() + static_cast<long>(end),
                     comparator);

    node->left = build(objects, start, mid);
    node->right = build(objects, mid, end);

    return node;
}
