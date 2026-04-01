/**
 * @file Material.cpp
 * @author RT Team - Dev C (Renderer)
 * @date 2026-04-01
 * @brief Material implementation
 */

#include "core/Material.hpp"

Material::Material() : color(1, 1, 1), ambient(0.1), diffuse(0.7), specular(0.2), 
                       shininess(32.0), reflectivity(0.0), transparency(0.0), 
                       refractiveIndex(1.0), texture(nullptr) {}

Material::Material(const Vec3& color, double ambient, double diffuse, double specular, double shininess)
    : color(color), ambient(ambient), diffuse(diffuse), specular(specular), 
      shininess(shininess), reflectivity(0.0), transparency(0.0), 
      refractiveIndex(1.0), texture(nullptr) {}

Material::Material(const Material& other)
    : color(other.color), ambient(other.ambient), diffuse(other.diffuse), 
      specular(other.specular), shininess(other.shininess), 
      reflectivity(other.reflectivity), transparency(other.transparency),
      refractiveIndex(other.refractiveIndex), texture(other.texture) {}

Material& Material::operator=(const Material& other)
{
  if (this == &other) {
    return *this;
  }
  color = other.color;
  ambient = other.ambient;
  diffuse = other.diffuse;
  specular = other.specular;
  shininess = other.shininess;
  reflectivity = other.reflectivity;
  transparency = other.transparency;
  refractiveIndex = other.refractiveIndex;
  texture = other.texture;
    return *this;
}

void Material::setColor(const Vec3& color) { this->color = color; }
const Vec3& Material::getColor() const { return color; }
void Material::setAmbient(double ambient) { this->ambient = ambient; }
double Material::getAmbient() const { return ambient; }
void Material::setDiffuse(double diffuse) { this->diffuse = diffuse; }
double Material::getDiffuse() const { return diffuse; }
void Material::setSpecular(double specular) { this->specular = specular; }
double Material::getSpecular() const { return specular; }
void Material::setShininess(double shininess) { this->shininess = shininess; }
double Material::getShininess() const { return shininess; }
void Material::setReflectivity(double reflectivity) { this->reflectivity = reflectivity; }
double Material::getReflectivity() const { return reflectivity; }
void Material::setTransparency(double transparency) { this->transparency = transparency; }
double Material::getTransparency() const { return transparency; }
void Material::setRefractiveIndex(double refractiveIndex) { this->refractiveIndex = refractiveIndex; }
double Material::getRefractiveIndex() const { return refractiveIndex; }
void Material::setTexture(Texture* texture) { this->texture = texture; }
Texture* Material::getTexture() const { return texture; }
bool Material::hasTexture() const { return texture != nullptr; }
