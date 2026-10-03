#include "core/Quadric.hpp"
#include <cmath>
#include <limits>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Tronquée [hMin,hMax] sur Y local (cylindre, cône, paraboloïde fini).
Quadric::Quadric(const Vec3& p, const Vec3& d, const Vec3& col, double hMin_, double hMax_)
    : pos(p), dir(d.normalized()), hMin(hMin_), hMax(hMax_), hasClip(true),
      A(0),B(0),C(0),D(0),E(0),Fc(0),G(0),Hc(0),I(0),J(0) {
    color = col;
    material = Material(col); // albedo = couleur uni par défaut
    updateBasis(); // 1 calcul base au lieu de per-ray
}

// Infinie (hyperboloïde infini) : pas de clip.
Quadric::Quadric(const Vec3& p, const Vec3& d, const Vec3& col)
    : pos(p), dir(d.normalized()), hMin(0), hMax(0), hasClip(false),
      A(0),B(0),C(0),D(0),E(0),Fc(0),G(0),Hc(0),I(0),J(0) {
    color = col;
    material = Material(col);
    updateBasis();
}

// Équation canonique locale (posée par chaque sous-classe).
void Quadric::setCoeffs(double A_, double B_, double C_, double D_, double E_, double F_,
                        double G_, double H_, double I_, double J_) {
    A=A_; B=B_; C=C_; D=D_; E=E_; Fc=F_; G=G_; Hc=H_; I=I_; J=J_;
}

// Base locale (legacy per-ray, gardée pour compat).
void Quadric::buildBasis(Vec3& u, Vec3& v) const {
    Vec3 w = dir;
    Vec3 arb{0,1,0};
    if (std::fabs(w.dot(arb)) > 0.999)
        arb = Vec3{0,0,1}; // axe quasi vertical : change de référence
    u = w.cross(arb).normalized(); // X local
    v = w.cross(u).normalized(); // Z local (base orthonormée directe)
}

// Recalcule le cache : appelé 1 fois par rotation seulement (pas par rayon).
void Quadric::updateBasis() {
    Vec3 w = dir;
    Vec3 arb{0,1,0};
    if (std::fabs(w.dot(arb)) > 0.999)
        arb = Vec3{0,0,1};
    cachedU = w.cross(arb).normalized();
    cachedV = w.cross(cachedU).normalized();
}

// Cylindrique : u = angle [0,1), v = hauteur normalisée (tronqué) ou fract (infini).
void Quadric::getUV(const Vec3& point, double& u, double& v) const {
    Vec3 rel = point - pos;
    double xL = rel.dot(cachedU);
    double yL = rel.dot(dir);
    double zL = rel.dot(cachedV);
    double theta = std::atan2(zL, xL); // -pi..pi
    u = theta / (2*M_PI) + 0.5;
    if (u < 0) u += 1.0;
    if (u >= 1.0) u -= 1.0;
    if (hasClip && hMax > hMin)
        v = (yL - hMin) / (hMax - hMin);
    else
        v = yL - std::floor(yL); // infini : répète verticalement
}

// Intersection générique : rayon -> local -> trinôme a*t²+b*t+c -> clip -> grad(F).
bool Quadric::intersect(const Ray& ray, Hit& hit) const {
    // Cache précalculé (1 rebuild/rotation) au lieu de buildBasis per-ray.
    const Vec3& u = cachedU;
    const Vec3& v = cachedV;
    const Vec3& w = dir;

    // Rayon exprimé en local (origine relative + projections).
    Vec3 oc = ray.origin - pos;
    double ox = oc.dot(u);
    double oy = oc.dot(w);
    double oz = oc.dot(v);

    double dx = ray.dir.dot(u);
    double dy = ray.dir.dot(w);
    double dz = ray.dir.dot(v);

    // Injection dans F : coefficients du trinôme.
    double a = A*dx*dx + B*dy*dy + C*dz*dz + D*dx*dy + E*dx*dz + Fc*dy*dz;
    double b = 2*A*ox*dx + 2*B*oy*dy + 2*C*oz*dz
             + D*(ox*dy + oy*dx) + E*(ox*dz + oz*dx) + Fc*(oy*dz + oz*dy)
             + G*dx + Hc*dy + I*dz;
    double c = A*ox*ox + B*oy*oy + C*oz*oz + D*ox*oy + E*ox*oz + Fc*oy*oz
             + G*ox + Hc*oy + I*oz + J;

    // Résolution : linéaire si a≈0, sinon discriminant.
    const double EPS = 1e-9;
    double tCandidates[2];
    int nCand = 0;

    if (std::fabs(a) < EPS) {
        if (std::fabs(b) < EPS) return false;
        double t = -c / b;
        tCandidates[0] = t;
        nCand = 1;
    } else {
        double disc = b*b - 4*a*c;
        if (disc < 0) return false;
        double sq = std::sqrt(disc);
        double t1 = (-b - sq) / (2*a);
        double t2 = (-b + sq) / (2*a);
        tCandidates[0] = t1;
        tCandidates[1] = t2;
        nCand = 2;
        if (t1 > t2) { double tmp=tCandidates[0]; tCandidates[0]=tCandidates[1]; tCandidates[1]=tmp; } // tri croissant
    }

    // Retient le plus proche valide : devant caméra + dans [hMin,hMax] + gradient non nul.
    double bestT = std::numeric_limits<double>::infinity();
    Vec3 bestNormalWorld{0,0,0};
    bool found = false;

    for (int i=0;i<nCand;i++) {
        double t = tCandidates[i];
        if (t < 1e-4 || t >= bestT) continue;
        double yLocal = oy + t*dy;
        if (hasClip && (yLocal < hMin - 1e-6 || yLocal > hMax + 1e-6)) continue; // hors troncature

        double xLocal = ox + t*dx;
        double zLocal = oz + t*dz;

        // Normale = gradient de F, reconverti en monde.
        double gx = 2*A*xLocal + D*yLocal + E*zLocal + G;
        double gy = D*xLocal + 2*B*yLocal + Fc*zLocal + Hc;
        double gz = E*xLocal + 2*C*zLocal + Fc*yLocal + I;

        Vec3 nWorld = u * gx + w * gy + v * gz;
        double len = nWorld.length();
        if (len < 1e-9) continue; // apex/singularité : ignore
        nWorld = nWorld / len;
        if (nWorld.dot(ray.dir) > 0) nWorld = nWorld * -1; // face au rayon

        bestT = t;
        bestNormalWorld = nWorld;
        found = true;
        if (nCand==2 && i==0) { // t triés : le 1er valide est le plus proche
            break;
        }
    }

    if (!found) return false;
    hit.t = bestT;
    hit.point = ray.origin + ray.dir * bestT;
    hit.normal = bestNormalWorld;
    hit.color = color;
    hit.material = material;
    getUV(hit.point, hit.u, hit.v);
    return true;
}

// ===== Cylinder : x² + z² − R² = 0, tronqué 0..h =====
Cylinder::Cylinder(const Vec3& p, const Vec3& d, double r, double h, const Vec3& col)
    : Quadric(p, d, col, 0, h), radius(r), height(h) {
    setCoeffs(1, 0, 1, 0, 0, 0, 0, 0, 0, -r*r); // A=1, C=1, J=−R²
    color = col;
    material = Material(col);
}

// Latéral via Quadric + 2 caps disque (base + sommet).
bool Cylinder::intersect(const Ray& ray, Hit& hit) const {
    Hit best;
    best.t = std::numeric_limits<double>::infinity();
    bool has = false;

    Hit lat;
    if (Quadric::intersect(ray, lat)) {
        best = lat;
        has = true;
    }

    // Caps : intersection plan + test radial.
    Vec3 topCenter = pos + dir * height;
    for (int cap=0; cap<2; cap++) {
        Vec3 capPos = (cap==0) ? pos : topCenter;
        Vec3 capN = (cap==0) ? dir * -1 : dir;
        double denom = capN.dot(ray.dir);
        if (std::fabs(denom) < 1e-9) continue; // rayon parallèle au cap
        double t = (capPos - ray.origin).dot(capN) / denom;
        if (t < 1e-4 || (has && t >= best.t)) continue;
        Vec3 p = ray.origin + ray.dir * t;
        if ((p - capPos).dot(p - capPos) > radius*radius + 1e-6) continue; // hors disque
        best.t = t;
        best.point = p;
        best.normal = capN;
        if (best.normal.dot(ray.dir) > 0) best.normal = best.normal * -1;
        best.color = color;
        best.material = material;
        // UV planaire du disque : coords locales / diamètre.
        Vec3 rel = p - capPos;
        double capU = rel.dot(cachedU) / (2*radius) + 0.5;
        double capV = rel.dot(cachedV) / (2*radius) + 0.5;
        best.u = capU - std::floor(capU);
        best.v = capV - std::floor(capV);
        has = true;
    }

    if (!has) return false;
    hit = best;
    return true;
}

// ===== Cone : base (rayon R en y=0), apex (rayon 0 en y=h) =====
// x²+z² − (R²/h²)(h−y)² = 0 -> A=1, C=1, B=−R²/h², Hc=2R²/h, J=−R²
Cone::Cone(const Vec3& p, const Vec3& d, double r, double h, const Vec3& col)
    : Quadric(p, d, col, 0, h), radius(r), height(h) {
    double r2 = r*r;
    double h2 = h*h;
    double Bcoeff = -r2 / h2;
    double Hcoeff = 2*r2 / h;
    double Jcoeff = -r2;
    setCoeffs(1, Bcoeff, 1, 0, 0, 0, 0, Hcoeff, 0, Jcoeff);
    color = col;
    material = Material(col);
}

// Latéral + cap base uniquement (apex = point, pas de cap).
bool Cone::intersect(const Ray& ray, Hit& hit) const {
    Hit best;
    best.t = std::numeric_limits<double>::infinity();
    bool has = false;

    Hit lat;
    if (Quadric::intersect(ray, lat)) {
        best = lat;
        has = true;
    }

    Vec3 capPos = pos;
    Vec3 capN = dir * -1;
    double denom = capN.dot(ray.dir);
    if (std::fabs(denom) >= 1e-9) {
        double t = (capPos - ray.origin).dot(capN) / denom;
        if (t >= 1e-4 && (!has || t < best.t)) {
            Vec3 p = ray.origin + ray.dir * t;
            if ((p - capPos).dot(p - capPos) <= radius*radius + 1e-6) {
                best.t = t;
                best.point = p;
                best.normal = capN;
                if (best.normal.dot(ray.dir) > 0) best.normal = best.normal * -1;
                best.color = color;
                best.material = material;
                Vec3 rel = p - capPos;
                double capU = rel.dot(cachedU) / (2*radius) + 0.5;
                double capV = rel.dot(cachedV) / (2*radius) + 0.5;
                best.u = capU - std::floor(capU);
                best.v = capV - std::floor(capV);
                has = true;
            }
        }
    }

    if (!has) return false;
    hit = best;
    return true;
}

// ===== Paraboloid : y = x²/a² + z²/b² -> A=1/a², C=1/b², Hc=−1, tronqué 0..h =====
Paraboloid::Paraboloid(const Vec3& p, const Vec3& d, double a_, double b_, double h, const Vec3& col)
    : Quadric(p, d, col, 0, h), a(a_), b(b_), height(h) {
    double invA2 = 1.0/(a*a);
    double invB2 = 1.0/(b*b);
    setCoeffs(invA2, 0, invB2, 0, 0, 0, 0, -1, 0, 0);
    color = col;
    material = Material(col);
}
Paraboloid::Paraboloid(const Vec3& p, const Vec3& d, double a_, double b_, const Vec3& col)
    : Quadric(p, d, col), a(a_), b(b_), height(0) { // infini
    double invA2 = 1.0/(a*a);
    double invB2 = 1.0/(b*b);
    setCoeffs(invA2, 0, invB2, 0, 0, 0, 0, -1, 0, 0);
    color = col;
    material = Material(col);
}
bool Paraboloid::intersect(const Ray& ray, Hit& hit) const {
    Hit best;
    best.t = std::numeric_limits<double>::infinity();
    bool has = false;
    Hit lat;
    if (Quadric::intersect(ray, lat)) {
        best = lat;
        has = true;
    }
    if (hasClip) { // cap base : disque de rayon a√h × b√h
        Vec3 capPos = pos;
        Vec3 capN = dir * -1;
        double denom = capN.dot(ray.dir);
        if (std::fabs(denom) >= 1e-9) {
            double t = (capPos - ray.origin).dot(capN) / denom;
            if (t >= 1e-4 && (!has || t < best.t)) {
                Vec3 p = ray.origin + ray.dir * t;
                Vec3 local = p - capPos;
                double xl = local.dot(cachedU);
                double zl = local.dot(cachedV);
                if ((xl*xl)/(a*a) + (zl*zl)/(b*b) <= height + 1e-6) {
                    best.t = t;
                    best.point = p;
                    best.normal = capN;
                    if (best.normal.dot(ray.dir) > 0) best.normal = best.normal * -1;
                    best.color = color;
                    best.material = material;
                    double capU = xl / (2*a*std::sqrt(height > 0 ? height : 1.0)) + 0.5;
                    double capV = zl / (2*b*std::sqrt(height > 0 ? height : 1.0)) + 0.5;
                    best.u = capU - std::floor(capU);
                    best.v = capV - std::floor(capV);
                    has = true;
                }
            }
        }
    }
    if (!has) return false;
    hit = best;
    return true;
}

// ===== Hyperboloid : x²/a² + z²/b² − y²/c² = ±1 (1 nappe / 2 nappes) =====
Hyperboloid::Hyperboloid(const Vec3& p, const Vec3& d, double a_, double b_, double c_, bool oneSheet_, const Vec3& col)
    : Quadric(p, d, col), a(a_), b(b_), c(c_), oneSheet(oneSheet_), height(0), finite(false) {
    double invA2 = 1.0/(a*a);
    double invB2 = 1.0/(b*b);
    double invC2 = 1.0/(c*c);
    double Bcoeff = -invC2; // terme −y²/c²
    double Jcoeff = oneSheet ? -1 : 1; // +1 déplacé : 1 nappe −1, 2 nappes +1
    setCoeffs(invA2, Bcoeff, invB2, 0, 0, 0, 0, 0, 0, Jcoeff);
    color = col;
    material = Material(col);
}
Hyperboloid::Hyperboloid(const Vec3& p, const Vec3& d, double a_, double b_, double c_, bool oneSheet_, double h, const Vec3& col)
    : Quadric(p, d, col, -h, h), a(a_), b(b_), c(c_), oneSheet(oneSheet_), height(h), finite(true) {
    double invA2 = 1.0/(a*a);
    double invB2 = 1.0/(b*b);
    double invC2 = 1.0/(c*c);
    double Bcoeff = -invC2;
    double Jcoeff = oneSheet ? -1 : 1;
    setCoeffs(invA2, Bcoeff, invB2, 0, 0, 0, 0, 0, 0, Jcoeff);
    color = col;
    material = Material(col);
}
bool Hyperboloid::intersect(const Ray& ray, Hit& hit) const {
    return Quadric::intersect(ray, hit); // pas de caps : surface ouverte
}
