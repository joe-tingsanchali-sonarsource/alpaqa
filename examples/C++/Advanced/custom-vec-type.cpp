#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <numeric>
#include <span>
#include <type_traits>

namespace custom {

template <class T, bool ElementWise = false>
class CustomVectorView;

template <class T>
class CustomVector {
  public:
    static_assert(!std::is_const_v<T>);
    using value_type           = T;
    using reference_type       = T &;
    using pointer_type         = T *;
    using const_pointer_type   = const T *;
    using const_reference_type = const T &;
    using index_type           = std::ptrdiff_t;
    using vec                  = CustomVector;

    CustomVector(index_type size = 0)
        : v{std::make_unique<T[]>(cast_index(size))}, size_{size} {}
    CustomVector(const CustomVector &o) : CustomVector(o.size()) {
        std::ranges::copy(o, begin());
    }
    CustomVector(CustomVector &&) = default;
    CustomVector &operator=(CustomVectorView<T>);
    CustomVector &operator=(CustomVectorView<const T>);
    CustomVector &operator=(const CustomVector<T> &o) {
        resize(o.size());
        std::ranges::copy(o, begin());
        return *this;
    }
    CustomVector &operator=(CustomVector<T> &&) = default;

    [[nodiscard]] reference_type operator()(index_type i) { return v[i]; }
    [[nodiscard]] const_reference_type operator()(index_type i) const {
        return v[i];
    }

    [[nodiscard]] index_type size() const { return size_; }
    [[nodiscard]] index_type rows() const { return size(); }
    [[nodiscard]] index_type cols() const { return 1; }
    [[nodiscard]] pointer_type data() { return v.get(); }
    [[nodiscard]] pointer_type begin() { return v.get(); }
    [[nodiscard]] pointer_type end() { return v.get() + size(); }
    [[nodiscard]] const_pointer_type data() const { return v.get(); }
    [[nodiscard]] const_pointer_type begin() const { return v.get(); }
    [[nodiscard]] const_pointer_type end() const { return v.get() + size(); }

    void setConstant(value_type value) { std::ranges::fill(*this, value); }
    void setZero() { setConstant(T{0}); }
    static CustomVector<T> Constant(index_type size, value_type value) {
        CustomVector<T> v(size);
        v.setConstant(value);
        return v;
    }
    static CustomVector<T> Zero(index_type size) {
        return Constant(size, T{0});
    }
    static CustomVector<T> Ones(index_type size) {
        return Constant(size, T{1});
    }

    void resize(index_type size) {
        if (size != this->size())
            v = std::make_unique<T[]>(cast_index(size));
    }

    value_type dot(CustomVectorView<const T> o);
    vec operator+(CustomVectorView<const T> o) const;
    vec &operator+=(CustomVectorView<const T> o) const;
    vec operator-(CustomVectorView<const T> o) const;
    vec &operator-=(CustomVectorView<const T> o) const;
    vec operator-() const;
    vec cwiseProduct(CustomVectorView<const T> o) const;
    vec cwiseQuotient(CustomVectorView<const T> o) const;
    bool operator==(CustomVectorView<const T> o) const;
    bool operator!=(CustomVectorView<const T> o) const;
    [[nodiscard]] bool allFinite() const;
    [[nodiscard]] value_type squaredNorm() const;
    [[nodiscard]] value_type norm() const;
    vec cwiseMax(CustomVectorView<const T> o) const;
    vec cwiseMax(const T &o) const;
    vec cwiseMin(CustomVectorView<const T> o) const;
    vec cwiseMin(const T &o) const;

    CustomVectorView<T, true> array();
    CustomVectorView<const T, true> array() const;

    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a,
                           CustomVectorView<const U> b) const;
    template <class U>
    CustomVector<U> select(CustomVector<U> a, CustomVector<U> b) const;
    template <class U>
    CustomVector<U> select(CustomVector<U> a, const U &b) const;

    CustomVectorView<T> topRows(index_type n);
    CustomVectorView<T> bottomRows(index_type n);
    CustomVectorView<const T> topRows(index_type n) const;
    CustomVectorView<const T> bottomRows(index_type n) const;

    void swap(CustomVector<T> &o) noexcept {
        using std::swap;
        static_assert(noexcept(swap(v, o.v)));
        swap(v, o.v);
    }

  private:
    static std::size_t cast_index(index_type i) {
        assert(i >= 0);
        return static_cast<size_t>(i);
    }
    std::unique_ptr<T[]> v;
    index_type size_ = 0;
};

template <class T, bool ElementWise>
class CustomVectorView {
  public:
    using value_type     = std::remove_cv_t<T>;
    using reference_type = T &;
    using pointer_type   = T *;
    using index_type     = std::ptrdiff_t;
    using vec            = CustomVector<value_type>;

    template <class U, bool E>
        requires(!std::is_same_v<T, U> || ElementWise != E)
    CustomVectorView(const CustomVectorView<U, E> &v) : v{v.begin(), v.end()} {}
    CustomVectorView(const CustomVectorView &v) : v{v.begin(), v.end()} {}
    template <class U>
    CustomVectorView(CustomVector<U> &v) : v{v.begin(), v.end()} {}
    template <class U>
    CustomVectorView(const CustomVector<U> &v)
        requires std::is_const_v<T>
        : v{v.begin(), v.end()} {}
    CustomVectorView(pointer_type ptr, index_type size)
        : v{ptr, cast_index(size)} {}

    template <class U, bool E>
        requires(!std::is_same_v<T, U>)
    CustomVectorView &operator=(const CustomVectorView<U, E> &o) {
        assert(o.size() == this->size());
        std::ranges::copy(o, begin());
        return *this;
    }
    CustomVectorView &operator=(const CustomVectorView &o) {
        assert(o.size() == this->size());
        std::ranges::copy(o, begin());
        return *this;
    }
    CustomVectorView &operator=(const CustomVector<value_type> &o) {
        assert(o.size() == this->size());
        std::ranges::copy(o, begin());
        return *this;
    }

    [[nodiscard]] reference_type operator()(index_type i) const {
        return v[cast_index(i)];
    }
    [[nodiscard]] index_type size() const {
        return static_cast<index_type>(v.size());
    }
    [[nodiscard]] index_type rows() const { return size(); }
    [[nodiscard]] index_type cols() const { return 1; }
    [[nodiscard]] pointer_type data() const { return v.data(); }
    [[nodiscard]] pointer_type begin() const { return v.data(); }
    [[nodiscard]] pointer_type end() const { return v.data() + v.size(); }

    value_type dot(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        return std::inner_product(begin(), end(), o.begin(), value_type{0});
    }
    vec operator+(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::plus{});
        return v;
    }
    CustomVectorView &operator+=(CustomVectorView<const T, ElementWise> o)
        requires(!std::is_const_v<T>)
    {
        assert(o.size() == this->size());
        std::ranges::transform(*this, o, begin(), std::plus{});
        return *this;
    }
    vec operator-(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::minus{});
        return v;
    }
    CustomVectorView &operator-=(CustomVectorView<const T, ElementWise> o)
        requires(!std::is_const_v<T>)
    {
        assert(o.size() == this->size());
        std::ranges::transform(*this, o, begin(), std::minus{});
        return *this;
    }
    vec operator-() const {
        vec v(size());
        std::ranges::transform(*this, v.begin(), std::negate{});
        return v;
    }
    vec cwiseProduct(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::multiplies{});
        return v;
    }
    vec operator*(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise
    {
        return cwiseProduct(o);
    }
    CustomVectorView &operator*=(CustomVectorView<const T, ElementWise> o)
        requires(ElementWise && !std::is_const_v<T>)
    {
        assert(o.size() == this->size());
        std::ranges::transform(*this, o, begin(), std::multiplies{});
        return *this;
    }
    template <class U>
    CustomVectorView &operator*=(const U &o)
        requires(!std::is_const_v<T>)
    {
        auto mul = [&o](const auto &a) { return a * o; };
        std::ranges::transform(*this, begin(), mul);
        return *this;
    }
    vec cwiseQuotient(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::divides{});
        return v;
    }
    CustomVectorView &operator/=(CustomVectorView<const T, ElementWise> o)
        requires(ElementWise && !std::is_const_v<T>)
    {
        assert(o.size() == this->size());
        std::ranges::transform(*this, o, begin(), std::divides{});
        return *this;
    }
    vec operator/(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise
    {
        return cwiseQuotient(o);
    }
    template <class U>
    CustomVectorView &operator/=(const U &o)
        requires(!std::is_const_v<T>)
    {
        auto div = [&o](const auto &a) { return a / o; };
        std::ranges::transform(*this, begin(), div);
        return *this;
    }
    bool operator==(CustomVectorView<const T, ElementWise> o) const
        requires(!ElementWise)
    {
        assert(o.size() == this->size());
        return std::ranges::equal(*this, o);
    }
    bool operator!=(CustomVectorView<const T, ElementWise> o) const
        requires(!ElementWise)
    {
        return !(*this == o);
    }
    CustomVector<bool>
    operator==(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise
    {
        assert(o.size() == this->size());
        CustomVector<bool> v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::equal_to{});
        return v;
    }
    CustomVector<bool>
    operator!=(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise
    {
        assert(o.size() == this->size());
        CustomVector<bool> v(o.size());
        std::ranges::transform(*this, o, v.begin(), std::not_equal_to{});
        return v;
    }
    template <class U>
    CustomVector<bool> operator==(const U &b) const
        requires(ElementWise && std::equality_comparable_with<T, U>)
    {
        CustomVector<bool> v(size());
        auto eq = [&b](const auto &a) { return a == b; };
        std::ranges::transform(*this, v.begin(), eq);
        return v;
    }
    template <class U>
    CustomVector<bool> operator!=(const U &b) const
        requires(ElementWise && std::equality_comparable_with<T, U>)
    {
        CustomVector<bool> v(size());
        auto eq = [&b](const auto &a) { return a != b; };
        std::ranges::transform(*this, v.begin(), eq);
        return v;
    }
    [[nodiscard]] bool allFinite() const {
        return std::ranges::all_of(
            *this, [](const auto &v) -> bool { return std::isfinite(v); });
    }
    [[nodiscard]] value_type squaredNorm() const { return dot(*this); }
    [[nodiscard]] value_type norm() const {
        using std::sqrt;
        return sqrt(squaredNorm());
    }
    void setConstant(value_type value) { std::ranges::fill(*this, value); }
    void setZero() { setConstant(T{0}); }
    vec cwiseMax(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        auto max = [](const T &a, const T &b) {
            return std::max(a, b); // TODO: fmax?
        };
        std::ranges::transform(*this, o, v.begin(), max);
        return v;
    }
    vec cwiseMin(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        vec v(o.size());
        auto min = [](const T &a, const T &b) {
            return std::min(a, b); // TODO: fmin?
        };
        std::ranges::transform(*this, o, v.begin(), min);
        return v;
    }
    vec cwiseMax(const T &o) const {
        vec v(size());
        auto max = [&o](const T &a) {
            return std::max(a, o); // TODO: fmax?
        };
        std::ranges::transform(*this, v.begin(), max);
        return v;
    }
    vec cwiseMin(const T &o) const {
        vec v(size());
        auto min = [&o](const T &a) {
            return std::min(a, o); // TODO: fmin?
        };
        std::ranges::transform(*this, v.begin(), min);
        return v;
    }
    CustomVectorView<T, true> array() const { return {*this}; }

    CustomVector<bool> operator<(const T &o) const {
        CustomVector<bool> v(size());
        std::ranges::transform(*this, v.begin(),
                               [&o](const T &a) { return std::less{}(a, o); });
        return v;
    }
    CustomVector<bool>
    operator<(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        CustomVector<bool> v(size());
        std::ranges::transform(*this, o, v.begin(), std::less{});
        return v;
    }
    CustomVector<bool>
    operator<=(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        CustomVector<bool> v(size());
        std::ranges::transform(*this, o, v.begin(), std::less_equal{});
        return v;
    }
    CustomVector<bool> operator>(const T &o) const {
        CustomVector<bool> v(size());
        std::ranges::transform(*this, v.begin(), [&o](const T &a) {
            return std::greater{}(a, o);
        });
        return v;
    }
    CustomVector<bool>
    operator>(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        CustomVector<bool> v(size());
        std::ranges::transform(*this, o, v.begin(), std::greater{});
        return v;
    }
    CustomVector<bool>
    operator>=(CustomVectorView<const T, ElementWise> o) const {
        assert(o.size() == this->size());
        CustomVector<bool> v(size());
        std::ranges::transform(*this, o, v.begin(), std::greater_equal{});
        return v;
    }

    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a,
                           CustomVectorView<const U> b) const {
        assert(a.size() == this->size());
        assert(b.size() == this->size());
        CustomVector<U> v(size());
        for (index_type i = 0; i < size(); ++i)
            v(i) = (*this)(i) ? a(i) : b(i);
        return v;
    }
    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a, const U &b) const {
        assert(a.size() == this->size());
        CustomVector<U> v(size());
        for (index_type i = 0; i < size(); ++i)
            v(i) = (*this)(i) ? a(i) : b;
        return v;
    }

    CustomVectorView<T> topRows(index_type n) const { return {begin(), n}; }
    CustomVectorView<T> bottomRows(index_type n) const {
        return {end() - n, n};
    }

  private:
    static std::size_t cast_index(index_type i) {
        assert(i >= 0);
        return static_cast<size_t>(i);
    }
    std::span<T> v;
};

template <class T>
CustomVector<T> &CustomVector<T>::operator=(CustomVectorView<const T> o) {
    resize(o.size());
    std::ranges::copy(o, begin());
    return *this;
}
template <class T>
CustomVector<T> &CustomVector<T>::operator=(CustomVectorView<T> o) {
    return *this = CustomVectorView<const T>{o};
}

template <class T>
auto CustomVector<T>::dot(CustomVectorView<const T> o) -> value_type {
    return CustomVectorView<const T>{*this}.dot(o);
}
template <class T>
auto CustomVector<T>::operator+(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this} + o;
}
template <class T>
auto CustomVector<T>::operator+=(CustomVectorView<const T> o) const -> vec & {
    return CustomVectorView<T>{*this} += o, *this;
}
template <class T>
auto CustomVector<T>::operator-(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this} - o;
}
template <class T>
auto CustomVector<T>::operator-=(CustomVectorView<const T> o) const -> vec & {
    return CustomVectorView<T>{*this} -= o, *this;
}
template <class T>
auto CustomVector<T>::operator-() const -> vec {
    return -CustomVectorView<const T>{*this};
}
template <class T>
auto CustomVector<T>::cwiseProduct(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseProduct(o);
}
template <class T>
auto CustomVector<T>::cwiseQuotient(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseQuotient(o);
}
template <class T>
auto CustomVector<T>::operator==(CustomVectorView<const T> o) const -> bool {
    return CustomVectorView<const T>{*this} == o;
}
template <class T>
auto CustomVector<T>::operator!=(CustomVectorView<const T> o) const -> bool {
    return CustomVectorView<const T>{*this} != o;
}
template <class T>
auto CustomVector<T>::allFinite() const -> bool {
    return CustomVectorView<const T>{*this}.allFinite();
}
template <class T>
auto CustomVector<T>::squaredNorm() const -> value_type {
    return CustomVectorView<const T>{*this}.squaredNorm();
}
template <class T>
auto CustomVector<T>::norm() const -> value_type {
    return CustomVectorView<const T>{*this}.norm();
}
template <class T>
auto CustomVector<T>::cwiseMax(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMax(o);
}
template <class T>
auto CustomVector<T>::cwiseMin(CustomVectorView<const T> o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMin(o);
}
template <class T>
auto CustomVector<T>::cwiseMax(const T &o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMax(o);
}
template <class T>
auto CustomVector<T>::cwiseMin(const T &o) const -> vec {
    return CustomVectorView<const T>{*this}.cwiseMin(o);
}

template <class T>
auto CustomVector<T>::array() -> CustomVectorView<T, true> {
    return {*this};
}
template <class T>
auto CustomVector<T>::array() const -> CustomVectorView<const T, true> {
    return {*this};
}

template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(CustomVectorView<const U> a,
                                        CustomVectorView<const U> b) const {
    return CustomVectorView<const T>{*this}.select(a, b);
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(CustomVector<U> a,
                                        CustomVector<U> b) const {
    return CustomVectorView<const T>{*this}.select(
        CustomVectorView<const U>{a}, CustomVectorView<const U>{b});
}
template <class T>
template <class U>
CustomVector<U> CustomVector<T>::select(CustomVector<U> a, const U &b) const {
    return CustomVectorView<const T>{*this}.select(CustomVectorView<const U>{a},
                                                   b);
}

template <class T>
auto CustomVector<T>::topRows(index_type n) const -> CustomVectorView<const T> {
    return CustomVectorView<const T>{*this}.topRows(n);
}
template <class T>
auto CustomVector<T>::bottomRows(index_type n) const
    -> CustomVectorView<const T> {
    return CustomVectorView<const T>{*this}.bottomRows(n);
}
template <class T>
auto CustomVector<T>::topRows(index_type n) -> CustomVectorView<T> {
    return CustomVectorView<T>{*this}.topRows(n);
}
template <class T>
auto CustomVector<T>::bottomRows(index_type n) -> CustomVectorView<T> {
    return CustomVectorView<T>{*this}.bottomRows(n);
}

// free-function operators

template <class A, class B>
auto operator*(const A &a, CustomVectorView<B> o) {
    CustomVector<decltype(a * std::declval<B>())> v(o.size());
    std::ranges::transform(o, v.begin(), [&a](const auto &b) { return a * b; });
    return v;
}
template <class A, class B>
auto operator*(CustomVectorView<A> a, const B &b) {
    CustomVector<decltype(std::declval<A>() * b)> v(a.size());
    std::ranges::transform(a, v.begin(), [&b](const auto &a) { return a * b; });
    return v;
}
template <class A, class B>
auto operator*(const A &a, CustomVector<B> o) {
    return a * CustomVectorView<const B>{o};
}
template <class A, class B>
auto operator*(CustomVector<A> a, const B &b) {
    return CustomVectorView<const B>{a} * b;
}

template <class T>
T norm_inf(CustomVectorView<T> v) {
    auto abs_ = [](const T &a) {
        using std::abs;
        return abs(a);
    };
    auto max_ = [](const T &a, const T &b) {
        using std::max;
        return max(a, b);
    };
    return std::transform_reduce(v.begin(), v.end(), T{0}, max_, abs_);
}
template <class T>
T norm_inf(const CustomVector<T> &v) {
    return norm_inf(CustomVectorView<const T>{v});
}

template <class T>
T norm_1(CustomVectorView<T> v) {
    auto abs_ = [](const T &a) {
        using std::abs;
        return abs(a);
    };
    return std::transform_reduce(v.begin(), v.end(), T{0}, std::plus{}, abs_);
}
template <class T>
T norm_1(const CustomVector<T> &v) {
    return norm_1(CustomVectorView<const T>{v});
}

} // namespace custom

#include <alpaqa/implementation/inner/panoc.tpp>
#include <alpaqa/implementation/outer/alm.tpp>
#include <alpaqa/implementation/problem/type-erased-problem.tpp>
#include <alpaqa/inner/directions/panoc/noop.hpp>
#include <alpaqa/problem/box-constr-problem.hpp>

struct CustomConfig {
    /// Real scalar element type.
    using real_t = double;
    /// Complex scalar element type.
    using cplx_t = std::complex<real_t>;
    /// Dynamic vector type.
    using vec = custom::CustomVector<real_t>;
    /// Map of vector type.
    using mvec = custom::CustomVectorView<real_t>;
    /// Immutable map of vector type.
    using cmvec = custom::CustomVectorView<const real_t>;
    /// Reference to mutable vector.
    using rvec = custom::CustomVectorView<real_t>;
    /// Reference to immutable vector.
    using crvec = custom::CustomVectorView<const real_t>;

    /// Type for lengths and sizes.
    using length_t = typename vec::index_type;
    /// Type for vector and matrix indices.
    using index_t = typename vec::index_type;

#if 0
    /// Dynamic vector of indices.
    using indexvec = Eigen::VectorX<index_t>;
    /// Reference to mutable index vector.
    using rindexvec = Eigen::Ref<indexvec>;
    /// Reference to immutable index vector.
    using crindexvec = Eigen::Ref<const indexvec>;
    /// Map of index vector type.
    using mindexvec = Eigen::Map<indexvec>;
    /// Immutable map of index vector type.
    using cmindexvec = Eigen::Map<const indexvec>;

    /// Dynamic matrix type.
    using mat = Eigen::MatrixX<real_t>;
    /// Map of matrix type.
    using mmat = Eigen::Map<mat>;
    /// Immutable map of matrix type.
    using cmmat = Eigen::Map<const mat>;
    /// Reference to mutable matrix.
    using rmat = Eigen::Ref<mat>;
    /// Reference to immutable matrix.
    using crmat = Eigen::Ref<const mat>;
    /// Dynamic complex matrix type.
    using cmat = Eigen::MatrixX<cplx_t>;
    /// Map of complex matrix type.
    using mcmat = Eigen::Map<cmat>;
    /// Immutable map of complex matrix type.
    using cmcmat = Eigen::Map<const cmat>;
    /// Reference to mutable complex matrix.
    using rcmat = Eigen::Ref<cmat>;
    /// Reference to immutable complex matrix.
    using crcmat = Eigen::Ref<const cmat>;
#else
    struct unsupported {};
    /// Dynamic vector of indices.
    using indexvec = unsupported;
    /// Reference to mutable index vector.
    using rindexvec = unsupported;
    /// Reference to immutable index vector.
    using crindexvec = unsupported;
    /// Map of index vector type.
    using mindexvec = unsupported;
    /// Immutable map of index vector type.
    using cmindexvec = unsupported;

    /// Dynamic matrix type.
    using mat = unsupported;
    /// Map of matrix type.
    using mmat = unsupported;
    /// Immutable map of matrix type.
    using cmmat = unsupported;
    /// Reference to mutable matrix.
    using rmat = unsupported;
    /// Reference to immutable matrix.
    using crmat = unsupported;
    /// Dynamic complex matrix type.
    using cmat = unsupported;
    /// Map of complex matrix type.
    using mcmat = unsupported;
    /// Immutable map of complex matrix type.
    using cmcmat = unsupported;
    /// Reference to mutable complex matrix.
    using rcmat = unsupported;
    /// Reference to immutable complex matrix.
    using crcmat = unsupported;
    /// Whether indexing by vectors of indices is supported.
    static constexpr bool supports_indexvec = false;
#endif
};

template <>
struct alpaqa::is_config<CustomConfig> : std::true_type {};

USING_ALPAQA_CONFIG(CustomConfig);

// Problem specification
// minimize  ½ xᵀQx
//  s.t.     Ax ≤ b
struct Problem : alpaqa::BoxConstrProblem<config_t> {
    alpaqa::mat<alpaqa::EigenConfigd> Q{n, n};
    alpaqa::mat<alpaqa::EigenConfigd> A{m, n};
    alpaqa::vec<alpaqa::EigenConfigd> b{m};
    mutable alpaqa::vec<alpaqa::EigenConfigd> Qx{n}, Ax{m};

    Problem() : alpaqa::BoxConstrProblem<config_t>{2, 1} {
        // Initialize problem matrices
        Q << 3, -1, -1, 3;
        A << 2, 1;
        b << -1;

        // Specify the bounds
        C.lowerbound = vec::Constant(n, -alpaqa::inf<config_t>);
        C.upperbound = vec::Constant(n, +alpaqa::inf<config_t>);
        D.lowerbound = vec::Constant(m, -alpaqa::inf<config_t>);
        D.upperbound = cmvec{b.data(), b.size()};
    }

    // Evaluate the cost
    real_t eval_f(crvec x) const {
        Qx = Q * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        return 0.5 * x.dot(cmvec{Qx.data(), Qx.size()});
    }
    // Evaluat the gradient of the cost
    void eval_grad_f(crvec x, rvec gr) const {
        Qx = Q * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        gr = cmvec{Qx.data(), Qx.size()};
    }
    // Evaluate the constraints
    void eval_g(crvec x, rvec g) const {
        Ax = A * alpaqa::cmvec<alpaqa::EigenConfigd>{x.data(), x.size()};
        g  = cmvec{Ax.data(), Ax.size()};
    }
    // Evaluate a matrix-vector product with the gradient of the constraints
    void eval_grad_g_prod(crvec x, crvec y, rvec gr) const {
        (void)x;
        Qx = A.transpose() *
             alpaqa::cmvec<alpaqa::EigenConfigd>{y.data(), y.size()};
        gr = cmvec{Qx.data(), Qx.size()};
    }
};

int main() {
    using Direction   = alpaqa::NoopDirection<config_t>;
    using InnerSolver = alpaqa::PANOCSolver<Direction>;
    using OuterSolver = alpaqa::ALMSolver<InnerSolver>;

    OuterSolver solver{
        {.print_interval = 1},
        {{.max_iter = 500, .print_interval = 50}, {}},
    };
    vec x = vec::Zero(2), y = vec::Zero(1);

    Problem problem;
    auto stats = solver(problem, x, y);

    return stats.status == alpaqa::SolverStatus::Converged ? 0 : 1;
}
