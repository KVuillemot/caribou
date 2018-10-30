#ifndef CARIBOU_GEOMETRY_HEXAHEDRON_H
#define CARIBOU_GEOMETRY_HEXAHEDRON_H

#include <Caribou/Geometry/Point.h>
#include <Caribou/Geometry/Segment.h>
#include <Caribou/Geometry/Quad.h>

namespace caribou
{
namespace geometry
{

/**
 * The base hexahedron class (of n nodes) in 3D space.
 *
 * This class will be overloaded with the number of nodes (8-nodes linear, 20-nodes quadratic, etc.) and the
 * type (regular or non-regular).
 */
template <size_t NNodes, typename TVector>
struct Hexahedron
{

    static constexpr size_t NumberOfNodes = NNodes;

    using VectorType = TVector;
    using PointType = Point<3, VectorType>;

    static_assert(NumberOfNodes >= 8, "A hexahedron must have at least eight nodes.");

    /** Delete the default constructor as it should be overloaded by base class on the number of nodes **/
    Hexahedron () = delete;

    /**
     * Constructor by a list initializer of PointType.
     *
     * Example:
     *
     * \code{.cpp}
     * Point3D p1, p2, p3, p4, p5, p6, p7, p8;
     * BaseHexahedron<8, false, Point3D::VectorType> hexahedron ({p1, p2, p3, p4, p5, p6, p7, p8});
     * \endcode
     *
     */
    Hexahedron (const std::initializer_list<PointType> & il)
    {
        std::copy(std::begin(il), std::end(il), std::begin(nodes));
    }

    Hexahedron (const std::initializer_list<VectorType> & il)
    {
        auto v = std::begin(il);
        size_t i = 0;
        for (; v != std::end(il); ++i, ++v) {
            nodes[i][0] = (*v)[0];
            nodes[i][1] = (*v)[1];
            nodes[i][2] = (*v)[2];
        }
    }

protected:
    std::array<PointType, NumberOfNodes> nodes;
};

/* Linear hexahedron
 *    7-------6
 *   /|      /|
 *  / |     / |
 * 3--|----2  |
 * |  4----|--5
 * | /     | /
 * 0-------1
 */


template <typename TVector>
struct LinearHexahedron : public Hexahedron<8, TVector>
{
    using Base = Hexahedron<8, TVector>;
    using Hexahedron<8, TVector>::Hexahedron; // Import the base constructors
    using VectorType = TVector;
    using PointType = Point<3, VectorType>;
    using SegmentType = Segment<VectorType>;
    using FaceType = Quad<VectorType>;

    LinearHexahedron() : Base ({
       //  {xi, eta, zeta}
       PointType({-1, -1, -1}), // 0
       PointType({ 1, -1, -1}), // 1
       PointType({ 1,  1, -1}), // 2
       PointType({-1,  1, -1}), // 3
       PointType({-1, -1,  1}), // 4
       PointType({ 1, -1,  1}), // 5
       PointType({ 1,  1,  1}), // 6
       PointType({-1,  1,  1})  // 7
    })
    {}

    /**
     * Get the node at index
     *
     *
     *          7----------------6
     *         /|               /|
     *        / |              / |
     *       /  |             /  |
     *      /   |            /   |
     *     3----------------2    |
     *     |    |           |    |
     *     |    |           |    |
     *     |    |           |    |
     *     |    4-----------|----5
     *     |   /            |   /
     *     |  /             |  /
     *     | /              | /
     *     |/               |/
     *     0----------------1
     *
     */
    PointType node(size_t index) const {
        return Base::nodes[index];
    };

    /**
     * Get the edge at index
     * 
     *                   6
     *                   .
     *          +----------------+
     *         /|               /|
     *   11 ../ |              / |
     *       /  |  2          /...... 10
     *      /   |  .         /   |
     *     +----------------+    |... 5
     *     | 7..|           |    |
     *     |    |           |    |
     *     |    |           |....|.... 1
     *  3..|    +-----------|----+
     *     |   /      4     |   /
     *     |  / 8           | 9/
     *     | /              | /
     *     |/        0      |/
     *     +----------------+
     *
     */
    SegmentType edge(size_t index) const {
        static constexpr std::array<std::array<unsigned char, 2>, 12> edges {{
            {0, 1}, // 0
            {1, 2}, // 1
            {2, 3}, // 2
            {3, 0}, // 3
            {4, 5}, // 4
            {5, 6}, // 5
            {6, 7}, // 6
            {7, 4}, // 7
            {0, 4}, // 8
            {1, 5}, // 9
            {2, 6}, // 10
            {3, 7}, // 11
        }};

        return SegmentType (
                Base::nodes[edges[index][0]],
                Base::nodes[edges[index][1]]
        );
    };

    /**
     * Get the face at index
     *                          2
     *                         .
     *          +----------------+
     *         /|     5         /|
     *        / |     .        / |
     *       /  |     .       /  |
     *      /   |            /   |
     *     +----------------+    |
     *     |    |           |    |
     *     |    |           |  .......... 3
     * 1...|.   |           |    |
     *     |    +-----------|----+
     *     |   /      ................... 0
     *     |  /             |  /
     *     | /              | /
     *     |/               |/
     *     +----------------+
     *               .
     *               .
     *               4
     */
    FaceType face(size_t index) const {
        static const std::array<std::array<NodeId, 4>, 6> nodes_of_face = {{
            {0, 1, 2, 3}, // 0 ; Edges = {0, 1, 2, 3,}
            {3, 7, 4, 0}, // 1 ; Edges = {11, 7, 8, 3}
            {7, 6, 5, 4}, // 2 ; Edges = {6, 5, 4, 7}
            {5, 6, 2, 1}, // 3 ; Edges = {5, 10, 1, 9}
            {5, 1, 0, 4}, // 4 ; Edges = {9, 0, 8, 4}
            {2, 6, 7, 3}, // 5 ; Edges = {10, 6, 11, 2}
        }};

        return FaceType (
                Base::nodes[nodes_of_face[index][0]],
                Base::nodes[nodes_of_face[index][1]],
                Base::nodes[nodes_of_face[index][2]],
                Base::nodes[nodes_of_face[index][3]]
        );
    };

};

/**
 * Regular linear hexahedron
 * @tparam TVector
 */
template <typename TVector>
struct RegularLinearHexahedron : public LinearHexahedron<TVector>
{
    using LinearHexahedron<TVector>::LinearHexahedron; // Import the base constructors
};

} // namespace geometry

} // namespace caribou

#endif //CARIBOU_GEOMETRY_HEXAHEDRON_H
