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

double AABB::surfaceArea() const
{
    Vec3 d = max - min;
    // Handle degenerate boxes (single point or flat)
    double dx = std::max(d.x, 0.0);
    double dy = std::max(d.y, 0.0);
    double dz = std::max(d.z, 0.0);
    // 2 * (dx*dy + dx*dz + dy*dz)
    return 2.0 * (dx * dy + dx * dz + dy * dz);
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

    // --- Surface Area Heuristic (SAH) ---
    // Use 12 bins per axis to find the split that minimizes expected traversal cost.
    constexpr size_t N_BINS = 12;
    constexpr double C_TRAV = 1.0;   // Cost of traversing a node (AABB test)
    constexpr double C_ISECT = 2.0;  // Cost of intersecting a primitive

    // Pre-compute world bounding boxes and centroids for all objects
    struct ObjData {
        Vec3 bMin, bMax;
        double centroid[3];
    };
    std::vector<ObjData> objData(count);
    for (size_t i = 0; i < count; ++i) {
        objects[start + i]->getWorldBoundingBox(objData[i].bMin, objData[i].bMax);
        objData[i].centroid[0] = (objData[i].bMin.x + objData[i].bMax.x) * 0.5;
        objData[i].centroid[1] = (objData[i].bMin.y + objData[i].bMax.y) * 0.5;
        objData[i].centroid[2] = (objData[i].bMin.z + objData[i].bMax.z) * 0.5;
    }

    int bestAxis = -1;
    size_t bestSplit = 0;
    double bestCost = C_ISECT * static_cast<double>(count); // cost of making a leaf
    double axisMinC[3] = {}, axisMaxC[3] = {}; // saved for partition step

    for (int axis = 0; axis < 3; ++axis) {
        // Find extent of centroids along this axis
        double minC = objData[0].centroid[axis];
        double maxC = minC;
        for (size_t i = 1; i < count; ++i) {
            double c = objData[i].centroid[axis];
            if (c < minC) minC = c;
            if (c > maxC) maxC = c;
        }
        axisMinC[axis] = minC;
        axisMaxC[axis] = maxC;

        double centroidRange = maxC - minC;
        if (centroidRange < Vec3::EPSILON)
            continue; // All centroids at same position along this axis → no useful split here

        double invRange = static_cast<double>(N_BINS) / centroidRange;

        // Bin objects along this axis
        struct Bin {
            AABB bounds;
            size_t count = 0;
        };
        Bin bins[N_BINS];

        for (size_t i = 0; i < count; ++i) {
            int idx = static_cast<int>((objData[i].centroid[axis] - minC) * invRange);
            idx = std::clamp(idx, 0, static_cast<int>(N_BINS) - 1);
            bins[idx].count++;
            bins[idx].bounds.expand(AABB(objData[i].bMin, objData[i].bMax));
        }

        // Compute prefix AABBs and counts (bins [0 .. b])
        AABB prefixAABB[N_BINS];
        size_t prefixCount[N_BINS] = {};
        for (size_t b = 0; b < N_BINS; ++b) {
            if (b > 0) {
                prefixAABB[b] = prefixAABB[b - 1];
                prefixCount[b] = prefixCount[b - 1];
            }
            if (bins[b].count > 0) {
                prefixAABB[b].expand(bins[b].bounds);
                prefixCount[b] += bins[b].count;
            }
        }

        // Compute suffix AABBs and counts (bins [b .. N_BINS-1])
        AABB suffixAABB[N_BINS];
        size_t suffixCount[N_BINS] = {};
        for (size_t b = N_BINS; b > 0; --b) {
            size_t i = b - 1;
            if (b < N_BINS) {
                suffixAABB[i] = suffixAABB[i + 1];
                suffixCount[i] = suffixCount[i + 1];
            }
            if (bins[i].count > 0) {
                suffixAABB[i].expand(bins[i].bounds);
                suffixCount[i] += bins[i].count;
            }
        }

        // Evaluate each bin boundary (split between bin b-1 and bin b)
        double parentSA = objBox.surfaceArea();
        if (parentSA <= 0.0) continue;

        for (size_t b = 1; b < N_BINS; ++b) {
            size_t leftN = prefixCount[b - 1];
            size_t rightN = suffixCount[b];
            if (leftN == 0 || rightN == 0) continue;

            double SA_left = prefixAABB[b - 1].surfaceArea();
            double SA_right = suffixAABB[b].surfaceArea();
            double cost = C_TRAV + C_ISECT * (SA_left * static_cast<double>(leftN)
                                            + SA_right * static_cast<double>(rightN)) / parentSA;

            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestSplit = b;
            }
        }
    }

    // If no split improves over a leaf node, store as leaf
    if (bestAxis < 0) {
        node->objects.reserve(count);
        for (size_t i = start; i < end; ++i) {
            node->objects.push_back(objects[i]);
        }
        return node;
    }

    // Partition objects using the chosen split plane
    // The split plane is at the midpoint of the chosen bin boundary
    double splitPlane = axisMinC[bestAxis] + (axisMaxC[bestAxis] - axisMinC[bestAxis])
                      * static_cast<double>(bestSplit) / static_cast<double>(N_BINS);

    auto mid = std::partition(
        objects.begin() + static_cast<long>(start),
        objects.begin() + static_cast<long>(end),
        [bestAxis, splitPlane](const std::shared_ptr<AObject>& obj) {
            Vec3 minB, maxB;
            obj->getWorldBoundingBox(minB, maxB);
            double c;
            switch (bestAxis) {
                case 0: c = (minB.x + maxB.x) * 0.5; break;
                case 1: c = (minB.y + maxB.y) * 0.5; break;
                default: c = (minB.z + maxB.z) * 0.5; break;
            }
            return c < splitPlane;
        }
    );

    size_t midIdx = static_cast<size_t>(mid - objects.begin());

    // Guard against degenerate splits (all objects on one side)
    if (midIdx <= start || midIdx >= end) {
        node->objects.reserve(count);
        for (size_t i = start; i < end; ++i) {
            node->objects.push_back(objects[i]);
        }
        return node;
    }

    node->left = build(objects, start, midIdx);
    node->right = build(objects, midIdx, end);

    return node;
}
