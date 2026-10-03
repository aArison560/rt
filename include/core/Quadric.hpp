#pragma once
#include "core/Object.hpp"
#include "math/Ray.hpp"

// Quadrique générique Phase 1 : F(x,y,z) = Ax²+By²+Cz²+Dxy+Exz+Fyz+Gx+Hy+Iz+J = 0
// exprimée en repère local (Y = dir/axe, X/Z = perpendiculaires, origine = pos).
// Intersection : injecte le rayon local -> a*t²+b*t+c = 0 ; normale = grad(F) normalisé.

class Quadric : public Object {
public:
    Quadric(const Vec3& p, const Vec3& d, const Vec3& col, double hMin_, double hMax_); // tronquée [hMin,hMax] sur Y local
    Quadric(const Vec3& p, const Vec3& d, const Vec3& col); // infinie

    virtual bool intersect(const Ray& ray, Hit& hit) const override; // résout le trinôme + clip + grad
    void getUV(const Vec3& point, double& u, double& v) const override; // cylindrique : angle × hauteur

    void setCoeffs(double A_, double B_, double C_, double D_, double E_, double F_,
                   double G_, double H_, double I_, double J_); // équation canonique locale

protected:
    Vec3    pos;
    Vec3    dir; // normalisée = Y local
    double  hMin, hMax;
    bool    hasClip; // true = tronquée

    double  A,B,C,D,E,Fc,G,Hc,I,J; // Fc = coef F (évite macro), Hc = coef H

    void buildBasis(Vec3& u, Vec3& v) const; // base locale (legacy per-ray)
    void updateBasis(); // recalcule le cache : 1 seul appel par rotation
    Vec3    cachedU{1,0,0}; // X local caché : évite 2 sqrt + cross par rayon/objet
    Vec3    cachedV{0,0,1}; // Z local caché

public:
    // Blender-like : translation O(1) sans rebuild, rotation = 1 rebuild.
    void translate(const Vec3& delta) override { pos = pos + delta; } // G : pas de rebuild basis
    Vec3 getPos() const override { return pos; }
    void setPos(const Vec3& p) override { pos = p; }
    Vec3 getDir() const override { return dir; }
    void setDir(const Vec3& d) override { dir = d.normalized(); updateBasis(); } // R : 1 seul rebuild
    inline const Vec3& getU() const noexcept { return cachedU; }
    inline const Vec3& getV() const noexcept { return cachedV; }
};

// Cylindre et Cone héritent Quadric (tronqués hMin/hMax) — AGENTS.md Phase 1
class Cylinder : public Quadric {
public:
    double radius;
    double height; // base pos -> sommet pos+dir*height

    Cylinder(const Vec3& p, const Vec3& d, double r, double h, const Vec3& col);
    bool intersect(const Ray& ray, Hit& hit) const override; // latéral Quadric + 2 caps disque
};

class Cone : public Quadric {
public:
    double radius; // à la base (apex = point, sans cap)
    double height;

    Cone(const Vec3& p, const Vec3& d, double r, double h, const Vec3& col);
    bool intersect(const Ray& ray, Hit& hit) const override; // latéral + cap base uniquement
};

class Paraboloid : public Quadric {
public:
    double a, b; // y = x²/a² + z²/b²
    double height; // troncature hMax (0 = infini via 2e constructeur)

    Paraboloid(const Vec3& p, const Vec3& d, double a_, double b_, double h, const Vec3& col);
    Paraboloid(const Vec3& p, const Vec3& d, double a_, double b_, const Vec3& col); // infini
    bool intersect(const Ray& ray, Hit& hit) const override; // latéral + cap base si tronqué
};

class Hyperboloid : public Quadric {
public:
    double a, b, c; // x²/a² + z²/b² − y²/c² = ±1
    bool oneSheet; // true = 1 nappe (+1), false = 2 nappes (−1)
    double height; // demi-hauteur si tronqué [-h,h]
    bool finite;

    Hyperboloid(const Vec3& p, const Vec3& d, double a_, double b_, double c_, bool oneSheet_, const Vec3& col);
    Hyperboloid(const Vec3& p, const Vec3& d, double a_, double b_, double c_, bool oneSheet_, double h, const Vec3& col);
    bool intersect(const Ray& ray, Hit& hit) const override; // direct Quadric (pas de caps)
};
