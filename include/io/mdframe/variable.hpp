#ifndef NGEN_IO_MDFRAME_VARIABLE_HPP
#define NGEN_IO_MDFRAME_VARIABLE_HPP

#include "mdarray.hpp"
#include "traits.hpp"
#include "dimension.hpp"
#include <initializer_list>
#include <boost/bind.hpp>


#define MDARRAY_VISITOR(name, return_type) struct name : public boost::static_visitor<return_type>
#define MDARRAY_VISITOR_IMPL(var_name) \
    template<typename T> \
    auto operator()(T& var_name) const

#define MDARRAY_VISITOR_TEMPLATE_IMPL(prototype, ...) \
    template<typename T> \
    auto operator()(prototype, ##__VA_ARGS__) const \

namespace io {

namespace detail {

namespace visitors { // -------------------------------------------------------

/**
 * mdarray visitor for retrieving the size of the mdarray
 */
MDARRAY_VISITOR(mdarray_size, std::size_t)
{
    MDARRAY_VISITOR_IMPL(v) -> std::size_t { return v.size(); }
};

/**
 * mdarray visitor for retrieving the rank of the mdarray
 */
MDARRAY_VISITOR(mdarray_rank, std::size_t)
{
    MDARRAY_VISITOR_IMPL(v) -> std::size_t { return v.rank(); }
};

MDARRAY_VISITOR(mdarray_shape, boost::span<const size_t>)
{
    MDARRAY_VISITOR_IMPL(v) -> boost::span<const size_t> { return v.shape(); }
};

/**
 * mdarray visitor for inserting a value
 */
MDARRAY_VISITOR(mdarray_insert, void)
{
    MDARRAY_VISITOR_TEMPLATE_IMPL(T& v, std::initializer_list<std::size_t> index, typename T::value_type val) -> void
    {
        v.insert(index, val);
    };
};

/**
 * mdarray visitor for indexed access
 */
template<typename... SupportedTypes>
MDARRAY_VISITOR(mdarray_at, typename traits::type_list<SupportedTypes...>::variant_scalar)
{
    MDARRAY_VISITOR_TEMPLATE_IMPL(T& v, boost::span<const size_t> index) -> typename T::value_type
    {
        auto val = v.at(index);

        return val;
    }
};

} // namespace visitors -------------------------------------------------------

/**
 * Variable Key
 *
 * Provides a tagged variable structure.
 * 
 * @tparam SupportedTypes types that this variable is able to hold.
 */
template<typename... SupportedTypes>
struct variable {
    using size_type = std::size_t;

    /**
     * The variable value types this frame can support.
     * These are stored as a compile-time type list to
     * derive further type aliases.
     */
    using types = traits::type_list<SupportedTypes...>;

    /**
     * A boost::variant type consisting of mdarrays of the
     * support types, i.e. for types int and double, this is
     * equivalent to:
     *     boost::variant<io::mdarray<int>, io::mdarray<double>>
     */
    using mdarray_variant = typename types::template variant_container<io::mdarray>;

    using element_type = typename types::variant_scalar;

    // Hasher for variables
    struct hash
    {
        std::size_t operator()(const variable& v) const noexcept
        {
            return std::hash<std::string>{}(v.m_name);
        }

        static std::size_t apply(const variable& d) noexcept
        {
            return variable::hash{}(d);
        }
    };

    /**
     * Constructs an empty variable
     */
    variable() noexcept
        : m_name()
        , m_data(mdarray<int>{0})
        , m_dimensions() {};

    /**
     * Constructs a named variable spanned over the given dimensions
     *
     * @param name Name of the dimension
     * @param dimensions List of dimensions
     */
    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    static variable make(const std::string& name, const std::vector<dimension>& dimensions)
    {
        variable var;
        var.m_name = name;
        var.m_dimensions = dimensions;

        std::vector<size_type> dsizes;
        dsizes.reserve(dimensions.size());
        for (const auto& d : dimensions) {
            dsizes.push_back(d.size());
        }
    
        var.m_data = mdarray<T>{dsizes};
        return var;
    }

    /**
     * Assign an mdarray to this variable.
     * 
     * @tparam T Supported mdframe types.
     * @param data The mdarray to assign.
     * @return variable& 
     */
    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    variable& set_data(const mdarray<T>& data)
    {
        this->m_data = data;
    }

    /**
     * Assign an mdarray to this variable.
     * 
     * @tparam T Supported mdframe types.
     * @param data The mdarray to assign.
     * @return variable& 
     */
    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    variable& set_data(mdarray<T>&& data)
    {
        this->m_data = std::move(data);
    }

    /**
     * Get the values of this variable
     * 
     * @return const mdarray_variant& 
     */
    const mdarray_variant& values() const noexcept
    {
        return this->m_data;
    }

    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    const mdarray<T>& values() const noexcept
    {
        return boost::get<mdarray<T>>(this->m_data);
    }

    /**
     * Equality operator to check equality between two variables.
     *
     * @note This operator compares the hashes of the variable **names**,
     *       not the backing mdarray.
     * 
     * @param rhs
     * @return true if the variables have the same name
     * @return false if the variables have different names
     */
    bool operator==(const variable& rhs) const
    {
        return hash{}(*this) == hash{}(rhs);
    }

    /**
     * Get the name of this variable
     * 
     * @return const std::string& 
     */
    const std::string& name() const noexcept {
        return this->m_name;
    }

    /**
     * Get the names of all dimensions associated with this variable.
     * 
     * @return std::vector<std::string> 
     */
    std::vector<std::string> dimensions() const noexcept {
        std::vector<std::string> names;
        names.reserve(this->m_dimensions.size());
        for (const dimension& dim : this->m_dimensions) {
            names.push_back(dim.name());
        }

        return names;
    }

    /**
     * Get the size of this variable
     *
     * @see mdarray::size
     * 
     * @return size_type 
     */
    size_type size() const noexcept {
        return boost::apply_visitor(visitors::mdarray_size{}, this->m_data);
    }

    /**
     * Get the rank of this variable
     *
     * @see mdarray::rank
     * 
     * @return size_type 
     */
    size_type rank() const noexcept {
        return boost::apply_visitor(visitors::mdarray_rank{}, this->m_data);
    }

    /**
     * Construct and insert a mdvalue into the backing mdarray.
     *
     * @see mdarray::emplace
     * 
     * @tparam T Must be the type stored within the mdarray
     * @param index Multi-dimensional index to insert to
     * @param value Value to insert into mdarray
     */
    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    void insert(std::initializer_list<size_type> index, T value)
    {
        // bind arguments to operator()
        auto visitor = boost::bind(
            visitors::mdarray_insert{},
            boost::placeholders::_1,
            index,
            value
        );

        boost::apply_visitor(visitor, this->m_data);
    }

    /**
     * Access value at a given index
     * 
     * @param index Multi-dimensional index.
     *        Size of this index list must match
     *        the dimensions of this variable.
     *
     * @return A variant corresponding to the
     *         value within this variable at the given index.
     */
    element_type at(const boost::span<const size_type> index)
    {
        auto visitor = std::bind(
            visitors::mdarray_at<SupportedTypes...>{},
            std::placeholders::_1,
            index
        );
    
        return boost::apply_visitor(visitor, this->m_data);
    }

    template<typename T, typename types::template enable_if_supports<T, bool> = true>
    T at(std::initializer_list<size_type> index)
    {
        auto visitor = std::bind(
            visitors::mdarray_at<SupportedTypes...>{},
            std::placeholders::_1,
            index
        );

        element_type result = boost::apply_visitor(visitor, this->m_data);
        T value = boost::get<T>(result);
        return value;
    }

    boost::span<const size_type> shape() const noexcept
    {
        return boost::apply_visitor(visitors::mdarray_shape{}, this->m_data);
    }

  private:
    // Name of this variable
    mutable std::string m_name;

    // multi-dimensional vector associated with this variable
    mdarray_variant m_data;

    // References to dimensions that this variable spans
    std::vector<dimension> m_dimensions;
};

} // namespace detail

} // namespace io

#endif // NGEN_IO_MDFRAME_VARIABLE_HPP
