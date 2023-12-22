#ifndef GEOMIPMAP_H
#define GEOMIPMAP_H

/**
 * @brief The GeoMipMap class
 */
class GeoMipMap {
public:
    GeoMipMap(unsigned int lod);
    unsigned lod();
    void lod(unsigned lod);

private:
    unsigned int _lod;
};

#endif // GEOMIPMAP_H
