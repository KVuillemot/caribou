#ifndef CARIBOU_GEOMETRY_QUAD_H
#define CARIBOU_GEOMETRY_QUAD_H

#include <Caribou/config.h>
#include <Caribou/Geometry/Interpolation/Quad.h>

namespace caribou::geometry {

template <size_t Dim, typename CanonicalElementType = interpolation::Quad4>
struct Quad : public CanonicalElementType
{
    static constexpr INTEGER_TYPE NumberOfNodes = CanonicalElementType::NumberOfNodes;

    using LocalCoordinates = typename CanonicalElementType::LocalCoordinates;
    using WorldCoordinates = Eigen::Matrix<FLOATING_POINT_TYPE, Dim, 1>;

    static_assert(Dim == 2 or Dim == 3, "Only 2D and 3D quads are supported.");

    template <
        typename ...Nodes,
        REQUIRES(NumberOfNodes == sizeof...(Nodes)+1)
    >
    Quad(const WorldCoordinates & first_node, Nodes&&...remaining_nodes)
    {
        construct_from_nodes<0>(first_node, std::forward<Nodes>(remaining_nodes)...);
    }

    inline
    auto
    node(UNSIGNED_INTEGER_TYPE index) const
    {
        return p_nodes.row(index);
    }

    inline
    auto
    node(UNSIGNED_INTEGER_TYPE index)
    {
        return p_nodes.row(index);
    }

    /** Get a reference to the set of nodes */
    inline
    const Eigen::Matrix<FLOATING_POINT_TYPE, NumberOfNodes, Dim> &
    nodes() const
    {
        return p_nodes;
    }

    /** Compute the center position **/
    auto
    center() const noexcept
    {
        return T(LocalCoordinates({0., 0.}));
    }

    /**
     * Compute the transformation of a local position {u} to its world position {x,y,z}
     */
    inline
    WorldCoordinates
    T(const LocalCoordinates & coordinates) const
    {
        return CanonicalElementType::interpolate_at_local_position(coordinates, nodes());
    }

    /** Compute the jacobian matrix evaluated at local position {u,v} */
    Eigen::Matrix<FLOATING_POINT_TYPE, Dim, 2>
    jacobian (const LocalCoordinates & coordinates) const
    {
        return CanonicalElementType::Jacobian(coordinates, p_nodes);
    }

private:
    template <size_t index, typename ...Nodes, REQUIRES(sizeof...(Nodes) >= 1)>
    inline
    void construct_from_nodes(const WorldCoordinates & first_node, Nodes&&...remaining_nodes) {
        p_nodes.row(index) = first_node;
        construct_from_nodes<index+1>(std::forward<Nodes>(remaining_nodes)...);
    }

    template <size_t index>
    inline
    void construct_from_nodes(const WorldCoordinates & last_node) {
        p_nodes.row(index) = last_node;
    }


private:
    Eigen::Matrix<FLOATING_POINT_TYPE, NumberOfNodes, Dim> p_nodes;
};

} // namespace caribou::geometry
#endif //CARIBOU_GEOMETRY_QUAD_H
