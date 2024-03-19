#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>

namespace custom {

template <class T, bool ElementWise = false>
class CustomVectorView;

template <class T>
class CustomVector {
    template <class, bool>
    friend class CustomVectorView;

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
    CustomVector &operator=(const CustomVector<T> &o);
    CustomVector &operator=(CustomVector<T> &&) = default;

  private:
    [[nodiscard]] reference_type operator()(index_type i) { return v[i]; }
    [[nodiscard]] const_reference_type operator()(index_type i) const {
        return v[i];
    }

  public:
    [[nodiscard]] index_type size() const { return size_; }
    [[nodiscard]] pointer_type data() { return v.get(); }
    [[nodiscard]] pointer_type begin() { return v.get(); }
    [[nodiscard]] pointer_type end() { return v.get() + size(); }
    [[nodiscard]] const_pointer_type data() const { return v.get(); }
    [[nodiscard]] const_pointer_type begin() const { return v.get(); }
    [[nodiscard]] const_pointer_type end() const { return v.get() + size(); }

    void setConstant(value_type value) { std::ranges::fill(*this, value); }
    void setZero() { setConstant(T{0}); }
    static CustomVector<T> Constant(index_type size, value_type value);
    static CustomVector<T> Zero(index_type size);
    static CustomVector<T> Ones(index_type size);

    void resize(index_type size);

    value_type dot(CustomVectorView<const T> o);
    vec operator+(CustomVectorView<const T> o) const;
    vec &operator+=(CustomVectorView<const T> o) const;
    vec operator-(CustomVectorView<const T> o) const;
    vec &operator-=(CustomVectorView<const T> o) const;
    vec operator-() const;
    [[nodiscard]] vec cwiseProduct(CustomVectorView<const T> o) const;
    [[nodiscard]] vec cwiseQuotient(CustomVectorView<const T> o) const;
    [[nodiscard]] vec cwiseMax(CustomVectorView<const T> o) const;
    [[nodiscard]] vec cwiseMax(const T &o) const;
    [[nodiscard]] vec cwiseMin(CustomVectorView<const T> o) const;
    [[nodiscard]] vec cwiseMin(const T &o) const;
    [[nodiscard]] vec cwiseAbs() const;
    [[nodiscard]] bool allFinite() const;
    [[nodiscard]] value_type squaredNorm() const;
    [[nodiscard]] value_type norm() const;
    bool operator==(CustomVectorView<const T> o) const;
    bool operator!=(CustomVectorView<const T> o) const;

    CustomVectorView<T, true> array();
    CustomVectorView<const T, true> array() const;

    template <class U>
    CustomVector<U> select(const CustomVector<U> &a,
                           CustomVectorView<const U> b) const;
    template <class U>
    CustomVector<U> select(const CustomVector<U> &a,
                           CustomVectorView<U> b) const;
    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a,
                           CustomVectorView<const U> b) const;
    template <class U>
    CustomVector<U> select(const CustomVector<U> &a,
                           const CustomVector<U> &b) const;
    template <class U>
    CustomVector<U> select(const CustomVector<U> &a, const U &b) const;

    CustomVectorView<T> topRows(index_type n);
    CustomVectorView<T> bottomRows(index_type n);
    CustomVectorView<const T> topRows(index_type n) const;
    CustomVectorView<const T> bottomRows(index_type n) const;

    void swap(CustomVector<T> &o) noexcept;

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
    template <class, bool>
    friend class CustomVectorView;

  public:
    using value_type     = std::remove_cv_t<T>;
    using reference_type = T &;
    using pointer_type   = T *;
    using index_type     = std::ptrdiff_t;
    using vec            = CustomVector<value_type>;

    template <class U>
    CustomVectorView(CustomVector<U> &v) : v{v.begin(), v.end()} {}
    template <class U, bool E>
    CustomVectorView(const CustomVectorView<U, E> &v)
        requires(!std::is_same_v<T, U> || ElementWise != E)
        : v{v.begin(), v.end()} {}
    CustomVectorView(const CustomVectorView &v) : v{v.begin(), v.end()} {}
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

  private:
    [[nodiscard]] reference_type operator()(index_type i) const {
        return v[cast_index(i)];
    }

  public:
    [[nodiscard]] index_type size() const { return std::ranges::ssize(v); }
    [[nodiscard]] pointer_type data() const { return v.data(); }
    [[nodiscard]] pointer_type begin() const { return v.data(); }
    [[nodiscard]] pointer_type end() const { return v.data() + v.size(); }

    void setConstant(value_type value) { std::ranges::fill(*this, value); }
    void setZero() { setConstant(T{0}); }

    value_type dot(CustomVectorView<const T, ElementWise> o) const;
    vec operator+(CustomVectorView<const T, ElementWise> o) const;
    CustomVectorView &operator+=(CustomVectorView<const T, ElementWise> o)
        requires(!std::is_const_v<T>);
    vec operator-(CustomVectorView<const T, ElementWise> o) const;
    CustomVectorView &operator-=(CustomVectorView<const T, ElementWise> o)
        requires(!std::is_const_v<T>);
    vec operator-() const;
    vec cwiseProduct(CustomVectorView<const T, ElementWise> o) const;

  private:
    vec operator*(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise;
    CustomVectorView &operator*=(CustomVectorView<const T, ElementWise> o)
        requires(ElementWise && !std::is_const_v<T>);

  public:
    template <class U>
    CustomVectorView &operator*=(const U &o)
        requires(!std::is_const_v<T>);
    vec cwiseQuotient(CustomVectorView<const T, ElementWise> o) const;

  private:
    CustomVectorView &operator/=(CustomVectorView<const T, ElementWise> o)
        requires(ElementWise && !std::is_const_v<T>);
    vec operator/(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise;

  public:
    template <class U>
    CustomVectorView &operator/=(const U &o)
        requires(!std::is_const_v<T>);
    bool operator==(CustomVectorView<const T, ElementWise> o) const
        requires(!ElementWise);
    bool operator!=(CustomVectorView<const T, ElementWise> o) const
        requires(!ElementWise);
    CustomVector<bool>
    operator==(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise;
    CustomVector<bool>
    operator!=(CustomVectorView<const T, ElementWise> o) const
        requires ElementWise;
    template <class U>
    CustomVector<bool> operator==(const U &b) const
        requires(ElementWise && std::equality_comparable_with<T, U>);
    template <class U>
    CustomVector<bool> operator!=(const U &b) const
        requires(ElementWise && std::equality_comparable_with<T, U>);
    [[nodiscard]] bool allFinite() const;
    [[nodiscard]] value_type squaredNorm() const;
    [[nodiscard]] value_type norm() const;
    [[nodiscard]] vec cwiseMax(CustomVectorView<const T, ElementWise> o) const;
    [[nodiscard]] vec cwiseMin(CustomVectorView<const T, ElementWise> o) const;
    [[nodiscard]] vec cwiseMax(const T &o) const;
    [[nodiscard]] vec cwiseMin(const T &o) const;
    [[nodiscard]] vec cwiseAbs() const;

    [[nodiscard]] CustomVectorView<T, true> array() const { return {*this}; }

    CustomVector<bool> operator<(const T &o) const;
    CustomVector<bool>
    operator<(CustomVectorView<const T, ElementWise> o) const;
    CustomVector<bool>
    operator<=(CustomVectorView<const T, ElementWise> o) const;
    CustomVector<bool> operator>(const T &o) const;
    CustomVector<bool>
    operator>(CustomVectorView<const T, ElementWise> o) const;
    CustomVector<bool>
    operator>=(CustomVectorView<const T, ElementWise> o) const;

    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a,
                           CustomVectorView<const U> b) const;
    template <class U>
    CustomVector<U> select(const CustomVector<U> &a,
                           CustomVectorView<U> b) const;
    template <class U>
    CustomVector<U> select(const CustomVector<U> &a,
                           CustomVectorView<const U> b) const;
    template <class U>
    CustomVector<U> select(CustomVectorView<const U> a, const U &b) const;

    CustomVectorView<T> topRows(index_type n) const;
    CustomVectorView<T> bottomRows(index_type n) const;

  private:
    static std::size_t cast_index(index_type i) {
        assert(i >= 0);
        return static_cast<size_t>(i);
    }
    std::span<T> v;
};

} // namespace custom
