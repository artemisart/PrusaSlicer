#ifndef slic3r_ShapeDiameterFunction_hpp_
#define slic3r_ShapeDiameterFunction_hpp_

#include "Point.hpp"
#include "Model.hpp"
#include "AABBTreeIndirect.hpp"
#include "NormalUtils.hpp"
#include "PointGrid3D.hpp"

#include <random> // sampling

namespace Slic3r {

class ShapeDiameterFunction
{
public:
    /// <summary>
    /// Rays into z direction.
    /// For calculation SDF function with multi rays.
    /// Weighted directions.
    /// </summary>
    struct Direction{
        Vec3f dir;
        float weight;
    };
    using Directions = std::vector<Direction>;

    /// <summary>
    /// DTO extends indexed_triangle_set with normals of triangles
    /// </summary>
    struct IndexTriangleNormals : public indexed_triangle_set{
        std::vector<Vec3f> triangle_normals; // same count as indexed_triangle_set.indices
        std::vector<Vec3f> vertex_normals;  // same count as indexed_triangle_set.vertuces     
    };

    struct AABBTree{
        AABBTreeIndirect::Tree3f tree;

        // for measure angle between ray and hit surface normal
        // condition of 90 deg from oposit direction of normal
        std::vector<Vec3f> triangle_normals; 

        // needed by function AABBTreeIndirect::intersect_ray_first_hit
        indexed_triangle_set vertices_indices;
    };

    struct PointRadius {
        Vec3f point;
        float radius;
        PointRadius(Vec3f point, float radius) : point(point), radius(radius){}
    };
    using PointRadiuses = std::vector<PointRadius>;

    struct Config;
    struct RaysConfig;
    struct SampleConfig;

    /// <summary>
    /// Only static functions
    /// </summary>
    ShapeDiameterFunction() = delete;

    /// <summary>
    /// Sample surface of tiny parts of model.
    /// </summary>
    /// <param name="its">Define model surface.</param>
    /// <param name="grid">Contain support points.</param>
    /// <param name="config">Define configuration for sampling</param> 
    /// <param name="random_generator">For create samples on surface</param> 
    /// <returns>Points from simplified model surface.</returns>
    static std::vector<Vec3f> sample_tiny_parts(
        const indexed_triangle_set &its,
        const PointGrid3D &         grid,
        const Config &              config,
        std::mt19937 &              random_generator);

    /// <summary>
    /// Calculate width in point given by weighted average width
    /// </summary>
    /// <param name="point">Surface point of model</param>
    /// <param name="normal">Normal in surface point</param>
    /// <param name="tree">AABB tree to fast detect first intersection</param>
    /// <param name="config">Define parameters and filtration for calculate SDF width</param>
    /// <returns>Width of model for point on surface</returns>
    static float calc_width(const Vec3f &     point,
                            const Vec3f &     normal,
                            const AABBTree &  tree,
                            const RaysConfig &config);

    /// <summary>
    /// Concurrent calculation of width for each vertex
    /// </summary>
    /// <param name="points">Point on surface of model</param>
    /// <param name="normals">Oposit Direction to cast rays, same count as points</param>
    /// <param name="tree">AABB tree to fast detect first intersection</param>
    /// <param name="config">Define parameters and filtration for calculate SDF width</param>
    /// <returns>Width(guess by SDF) for each vertex</returns>
    static std::vector<float> calc_widths(const std::vector<Vec3f> &points,
                                          const std::vector<Vec3f> &normals,
                                          const AABBTree &          tree,
                                          const RaysConfig &        config);

    /// <summary>
    /// Generate surface points on tiny part of model
    /// Traingle should have normalized size of side
    /// </summary>
    /// <param name="its">Define vertices and indices of triangle mesh</param>
    /// <param name="widths">Width for each vertex(same size as its::vetices)</param>
    /// <param name="cfg">configuration where to generate support</param>
    /// <param name="random_generator">Generate random points on surface</param>
    /// <returns>Vector of surface points</returns>
    static PointRadiuses generate_support_points(
        const indexed_triangle_set &its, const std::vector<float> &widths,
        const SampleConfig& cfg, std::mt19937& random_generator);

    /// <summary>
    /// Reduce samples by its alone raiuses
    /// </summary>
    /// <param name="samples">IN/OUT: Position with alone radius</param>
    /// <param name="grid">Already selected support points</param>
    static void poisson_sphere_from_samples(PointRadiuses &samples, const PointGrid3D &grid);

    /// <summary>
    /// Create points on unit sphere surface. with weight by z value
    /// </summary>
    /// <param name="angle">Cone angle in DEG, filtrate uniform sample of half sphere</param>
    /// <param name="count_samples">Count samples for half sphere</param>
    /// <returns>Unit vectors lay inside cone with direction to Z axis</returns>
    static Directions create_fibonacci_sphere_samples(double angle, size_t count_samples);

    /// <summary>
    /// divide each triangle with biger side length than max_length
    /// </summary>
    /// <param name="its">Input vertices and faces</param>
    /// <param name="max_length">Maximal length</param>
    /// <returns>new triangle_set</returns>
    static indexed_triangle_set subdivide(const indexed_triangle_set &its, float max_length);

    /// <summary>
    /// Minimal length of triangle side, smaller side will be removed
    /// </summary>
    /// <param name="its">Input vertices and faces</param>
    /// <param name="min_length">Minimal length of triangle side</param>
    /// <param name="max_error">Maximal error during reduce of triangle side</param>
    /// <returns>Re-meshed</returns>
    static void connect_small_triangles(indexed_triangle_set &its, float min_length, float max_error); 

    /// <summary>
    /// Find shortest edge in index triangle set
    /// TODO: move to its utils
    /// </summary>
    /// <param name="its">input definition of triangle --> every triangle has 3 edges to explore</param>
    /// <returns>Length of shortest edge</returns>
    static float min_triangle_side_length(const indexed_triangle_set &its);

    /// <summary>
    /// Find shortest edge in index triangle set
    /// TODO: move to triangle utils
    /// </summary>
    /// <param name="v0">Triangle vertex</param>
    /// <param name="v1">Triangle vertex</param>
    /// <param name="v2">Triangle vertex</param> 
    /// <returns>Area of triangle</returns>
    static float triangle_area(const Vec3f &v0,
                               const Vec3f &v1,
                               const Vec3f &v2);

    /// <summary>
    /// Calculate area
    /// TODO: move to its utils
    /// </summary>
    /// <param name="inices">Triangle indices - index to vertices</param>
    /// <param name="vertices">Mesh vertices</param>
    /// <returns>Area of triangles</returns>
    static float triangle_area(const Vec3crd &inices, const std::vector<Vec3f> &vertices);

    /// <summary>
    /// Calculate surface area of all triangles defined by indices.
    /// TODO: move to its utils
    /// </summary>
    /// <param name="its">Indices and Vertices</param>
    /// <returns>Surface area as sum of triangle areas</returns>
    static float area(const indexed_triangle_set &its);

private:
    // for debug purpose
    // store direction to STL file
    static bool store(const Directions &unit_z_rays);

};// class ShapeDiameterFunction

/// <summary>
/// DTO with valus for calculate width by SDF
/// </summary>
struct ShapeDiameterFunction::RaysConfig
{
    // Multiplicator of standart deviation to be result of ray counted,
    // negative number means no filtration by deviation
    // be carefull with value close 1. Standart deviation and means are
    // calculated with float precision only. when use only 2 values non of
    // them needs to be in deviation
    float allowed_deviation = 1.5f; // [in std dev multiplication]

    // Maximal angle between normal and hitted triangle normal[in Radians]
    // negative number means no filtration by angle
    float allowed_angle = -1.f;
    // for no oposit direction use value: static_cast<float>(M_PI_2) +
    // std::numeric_limits<float>::epsilon();

    // Direction to cast rays with unit size
    // Before use z direction is rotated to negative normal of point
    Directions dirs = create_fibonacci_sphere_samples(120., 60);

    // safe against ray intersection with origin trinagle made by source vertex
    float safe_move = 1e-3f;

    // Filtration parameter for speed up
    // Do not calculate Width from top of model
    // Must be less or equal to SampleConfig::normal_z_max
    float normal_z_max = 0.3f;

    // create config with default values
    RaysConfig() = default;
    void set_no_deviation_filtering() { allowed_deviation = -1.f; }
    bool is_deviation_filtering() const { return allowed_deviation > 0; }
    void set_no_angle_filtering() { allowed_angle = -1.f; }
    bool is_angle_filtering() const { return allowed_angle > 0; }
};

/// <summary>
/// DTO with valus for samplig tiny parts
/// </summary>
struct ShapeDiameterFunction::SampleConfig
{
    // range of width to support for linear distributiion of count supports
    float min_width = 0.1f;
    float max_width = 10.f;

    // range of alone support radius - for filtration of generated support points
    float min_radius = 1.5f;
    float max_radius = 10.f;

    // at min_width is min_radius
    // at max_width is max_radius

    // filter top side triangles,
    // minimal angle to z Axis = acos(0.3) = 72.5 DEG
    // recommend value greater than zero to support vertical walls
    float normal_z_max = 0.3f;

    // multiply count of generated samples(before Poisson filtration) for
    // radius cover error of random generator recommend value in range(2 - 12)
    float multiplicator = 6;

    SampleConfig() = default;
};

struct ShapeDiameterFunction::Config
{
    RaysConfig   rays;
    SampleConfig sample;

    //// Quadric edge collapse - Reduce amount of triangles
    // Only edge with smaller error will be collapsed
    float max_error = .5f;  // quadric
    // Only smaller edge than this value will be collapsed
    float min_length = .5f; // [in mm]

    // maximal length of edge - longer edge will be divided
    float max_length = 1.f; // [in mm]

    // Way of deduce normal for vertex from surrounding triangles
    NormalUtils::VertexNormalType normal_type =
        NormalUtils::VertexNormalType::NelsonMaxWeighted;

    Config() = default;
};

} // namespace Slic3r

#endif // slic3r_ShapeDiameterFunction_hpp_
