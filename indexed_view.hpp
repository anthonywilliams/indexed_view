#ifndef JSS_INDEXED_VIEW_HPP
#define JSS_INDEXED_VIEW_HPP
#include <iterator>
#include <type_traits>
#include <stddef.h>
#include <stdlib.h>

namespace jss {
    namespace detail {
        /// A type that encapsulates an indexed view over an underlying range
        /// So the value_type is a struct holding an index and the value of the
        /// underlying range
        template <typename UnderlyingIterator, typename UnderlyingSentinel>
        class indexed_view_type {
        private:
            /// Special index marker for the sentinel
            static constexpr size_t sentinel_marker= ~static_cast<size_t>(0);
            /// Is the underlying iterator nothrow move constructible?
            static constexpr bool nothrow_move_iterators=
                std::is_nothrow_move_constructible<UnderlyingIterator>::value;
            /// Is the sentinel nothrow move constructible?
            static constexpr bool nothrow_move_sentinels=
                std::is_nothrow_move_constructible<UnderlyingSentinel>::value;
            /// Is the iterator nothrow copy constructible?
            static constexpr bool nothrow_copy_iterators=
                std::is_nothrow_copy_constructible<UnderlyingIterator>::value;
            /// Is the sentinel nothrow copy constructible?
            static constexpr bool nothrow_copy_sentinels=
                std::is_nothrow_copy_constructible<UnderlyingSentinel>::value;
            /// Is the iterator/sentinel comparison nothrow?
            static constexpr bool nothrow_comparable_iterators= noexcept(
                std::declval<UnderlyingIterator &>() !=
                std::declval<UnderlyingSentinel &>());
            /// Is incrementing an iterator nothrow?
            static constexpr bool nothrow_iterator_increment=
                noexcept(++std::declval<UnderlyingIterator &>());

            /// The type of dereferencing an underlying iterator
            using underlying_value_type=
                decltype(*std::declval<UnderlyingIterator &>());
            /// Is dereferencing an iterator nothrow?
            static constexpr bool nothrow_deref=
                noexcept(*std::declval<UnderlyingIterator &>());

        public:
            /// Construct a range from an iterator/sentinel pair
            indexed_view_type(
                UnderlyingIterator &&begin_,
                UnderlyingSentinel
                    &&end_) noexcept(nothrow_move_iterators
                                         &&nothrow_move_sentinels) :
                source_begin(std::move(begin_)),
                source_end(std::move(end_)) {}

            /// The value_type of our range is an index/value pair
            struct value_type {
                size_t index;
                underlying_value_type value;
            };

            /// The iterator for our range
            class iterator {
                /// It's an input iterator, so we need a proxy for ->
                struct arrow_proxy {
                    /// Our proxy operator->
                    value_type *operator->() noexcept {
                        return &value;
                    }

                    /// The pointed-to value
                    value_type value;
                };

                /// Proxy for handling *x++
                struct postinc_return {
                    /// The pointed-to value
                    value_type value;

                    /// Our proxy operator*
                    const value_type operator*() noexcept(
                        std::is_nothrow_move_constructible<value_type>::value) {
                        return std::move(value);
                    }
                };

            public:
                /// Required iterator typedefs
                using value_type= typename indexed_view_type::value_type;
                /// Required iterator typedefs
                using reference= value_type;
                /// Required iterator typedefs
                using iterator_category= std::input_iterator_tag;
                /// Required iterator typedefs
                using pointer= value_type *;
                /// Required iterator typedefs: cannot do std::distance on input
                /// iterators
                using difference_type= void;

                /// Compare iterators for inequality.
                /// Only requires underlying_iterator!=underlying_sentinel
                friend bool
                operator!=(iterator const &lhs, iterator const &rhs) noexcept(
                    nothrow_comparable_iterators) {
                    if(lhs.is_iterator()) {
                        if(rhs.is_iterator()) {
                            return lhs.index != rhs.index;
                        } else {
                            return lhs.get_source_iterator() !=
                                   rhs.get_sentinel();
                        }
                    } else {
                        if(rhs.is_iterator()) {
                            return rhs != lhs;
                        } else {
                            return false;
                        }
                    }
                }

                /// Equality in terms of iterators: if it's not not-equal then
                /// it must be equal
                friend bool
                operator==(iterator const &lhs, iterator const &rhs) noexcept(
                    nothrow_comparable_iterators) {
                    return !(lhs != rhs);
                }

                /// Dereference the iterator
                const value_type operator*() const noexcept(
                    nothrow_deref &&
                        std::is_nothrow_move_constructible<value_type>::value) {
                    return value_type{index, *get_source_iterator()};
                }

                /// Dereference for iter->m
                arrow_proxy operator->() const noexcept(
                    nothrow_deref &&
                        std::is_nothrow_move_constructible<value_type>::value) {
                    return arrow_proxy{
                        value_type{index, *get_source_iterator()}};
                }

                /// Pre-increment
                iterator &operator++() noexcept(nothrow_iterator_increment) {
                    ++get_source_iterator();
                    ++index;
                    return *this;
                }

                /// Post-increment
                postinc_return operator++(int) noexcept(
                    nothrow_iterator_increment &&nothrow_deref &&
                        std::is_nothrow_move_constructible<value_type>::value) {
                    postinc_return temp{**this};
                    ++*this;
                    return temp;
                }

                /// Copy constructor
                iterator(iterator const &other) noexcept(
                    nothrow_copy_iterators &&nothrow_copy_sentinels) {
                    construct_from(other);
                }
                /// Move constructor
                iterator(iterator &&other) noexcept(
                    nothrow_move_iterators &&nothrow_move_sentinels) {
                    construct_from(std::move(other));
                }

                /// Copy assignment
                iterator &operator=(iterator const &other) noexcept {
                    if(&other != this) {
                        destroy();
                        construct_from(other);
                    }
                    return *this;
                }

                /// Move assignment
                iterator &operator=(iterator &&other) noexcept {
                    if(&other != this) {
                        destroy();
                        construct_from(std::move(other));
                    }
                    return *this;
                }

                /// Destructor
                ~iterator() {
                    destroy();
                }

            private:
                friend class indexed_view_type;

                /// Either copy-construct an underling iterator or sentinel as
                /// appropriate
                void construct_from(iterator const &other) noexcept(
                    nothrow_copy_iterators &&nothrow_copy_sentinels) {
                    index= other.index;
                    if(other.is_iterator()) {
                        new(get_storage_ptr())
                            UnderlyingIterator(other.get_source_iterator());
                    } else {
                        new(get_storage_ptr())
                            UnderlyingSentinel(other.get_sentinel());
                    }
                }
                /// Either move-construct an underling iterator or sentinel as
                /// appropriate
                void construct_from(iterator &&other) noexcept(
                    nothrow_move_iterators &&nothrow_move_sentinels) {
                    index= other.index;
                    if(other.is_iterator()) {
                        new(get_storage_ptr()) UnderlyingIterator(
                            std::move(other.get_source_iterator()));
                    } else {
                        new(get_storage_ptr())
                            UnderlyingSentinel(std::move(other.get_sentinel()));
                    }
                }

                /// Either destroy an underling iterator or sentinel as
                /// appropriate
                void destroy() {
                    if(is_iterator()) {
                        get_source_iterator().~UnderlyingIterator();
                    } else {
                        get_sentinel().~UnderlyingSentinel();
                    }
                }

                /// Is this an iterator or sentinel?
                bool is_iterator() const noexcept {
                    return index != sentinel_marker;
                }

                /// Get a pointer to the storage
                void *get_storage_ptr() const noexcept {
                    return static_cast<void *>(&storage);
                }
                /// Get the stored sentinel
                UnderlyingSentinel &get_sentinel() const noexcept {
                    return *static_cast<UnderlyingSentinel *>(
                        get_storage_ptr());
                }

                /// Get the stored iterator
                UnderlyingIterator &get_source_iterator() const noexcept {
                    return *static_cast<UnderlyingIterator *>(
                        get_storage_ptr());
                }

                /// Construct from an underlying iterator and an index
                iterator(
                    size_t index_,
                    UnderlyingIterator
                        &source_iter_) noexcept(nothrow_copy_iterators) :
                    index(index_) {
                    new(get_storage_ptr()) UnderlyingIterator(source_iter_);
                }

                /// Construct a sentinel
                iterator(UnderlyingSentinel &sentinel_) noexcept(
                    nothrow_copy_sentinels) :
                    index(sentinel_marker) {
                    new(get_storage_ptr()) UnderlyingSentinel(sentinel_);
                }

                /// The stored index
                size_t index;

                /// Storage for an iterator or a sentinel
                /// Should be std::variant if available
                mutable typename std::aligned_storage<
                    (sizeof(UnderlyingIterator) > sizeof(UnderlyingSentinel)) ?
                        sizeof(UnderlyingIterator) :
                        sizeof(UnderlyingSentinel),
                    (alignof(UnderlyingIterator) >
                     alignof(UnderlyingSentinel)) ?
                        alignof(UnderlyingIterator) :
                        alignof(UnderlyingSentinel)>::type storage;
            };

            /// Get an iterator for the start of the range
            iterator begin() noexcept(nothrow_copy_iterators) {
                return iterator(0, source_begin);
            }
            /// Get an iterator for the sentinel at the end of the range
            iterator end() noexcept(nothrow_copy_sentinels) {
                return iterator(source_end);
            }

        private:
            /// The start of the underlying range
            UnderlyingIterator source_begin;
            /// The end of the underlying range
            UnderlyingSentinel source_end;
        };

        /// A class to hold a copy of a source range, in order to keep it alive
        template <typename Range> class range_holder {
        private:
            /// The stored range
            mutable Range source_range;

        protected:
            /// Move a source range into internal storage
            range_holder(Range &source_) noexcept(
                std::is_nothrow_move_constructible<Range>::value) :
                source_range(std::move(source_)) {}

            /// Get the begin iterator for the stored range
            auto get_source_begin() const
                noexcept(noexcept(std::begin(std::declval<Range &>())))
                    -> decltype(std::begin(this->source_range)) {
                return std::begin(source_range);
            }
            /// Get the end iterator for the stored range
            auto get_source_end() const
                noexcept(noexcept(std::end(std::declval<Range &>())))
                    -> decltype(std::end(this->source_range)) {
                return std::end(source_range);
            }
        };

        /// A class derived from indexed_view_type that also holds a copy of the
        /// range
        template <
            typename Range, typename UnderlyingIterator,
            typename UnderlyingSentinel>
        class extended_indexed_view_type
            : range_holder<Range>,
              public indexed_view_type<UnderlyingIterator, UnderlyingSentinel> {
        public:
            /// Construct from a source range: move the range into storage and
            /// then construct the indexed view over that storaged range
            extended_indexed_view_type(Range &source) noexcept(
                std::is_nothrow_move_constructible<Range>::value &&std::
                    is_nothrow_move_constructible<UnderlyingIterator>::value
                        &&std::is_nothrow_move_constructible<
                            UnderlyingSentinel>::value
                            &&noexcept(std::begin(std::declval<Range &>())) &&
                noexcept(std::end(std::declval<Range &>()))) :
                range_holder<Range>(source),
                indexed_view_type<UnderlyingIterator, UnderlyingSentinel>(
                    this->get_source_begin(), this->get_source_end()) {}
        };

    }

    /// Construct an indexed view over the supplied range
    /// This handles rvalue ranges by capturing the range into an
    /// ExtendedIndexedViewType
    template <typename Range>
    auto indexed_view(Range &&source) noexcept(noexcept(
        detail::extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))>(
            source)))
        -> detail::extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))> {
        return detail::extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))>(
            source);
    }

    /// Construct an indexed view over an lvalue range
    /// The source range must be valid until the view is no longer used
    template <typename Range>
    auto indexed_view(Range &source) noexcept(
        noexcept(detail::indexed_view_type<
                 decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source))))
        -> detail::indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))> {
        return detail::indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }

    /// Construct an indexed view over a const lvalue range
    /// The source range must be valid until the view is no longer used
    template <typename Range>
    auto indexed_view(Range const &source) noexcept(
        noexcept(detail::indexed_view_type<
                 decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source))))
        -> detail::indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))> {
        return detail::indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }

    /// Construct an indexed view over a range specified by an iterator/sentinal
    /// pair The source range must be valid until the view is no longer used
    template <typename UnderlyingIterator, typename UnderlyingSentinel>
    auto indexed_view(
        UnderlyingIterator source_begin,
        UnderlyingSentinel
            source_end) noexcept(noexcept(detail::
                                              indexed_view_type<
                                                  UnderlyingIterator,
                                                  UnderlyingSentinel>(
                                                  std::move(source_begin),
                                                  std::move(source_end))))
        -> detail::indexed_view_type<UnderlyingIterator, UnderlyingSentinel> {
        return detail::indexed_view_type<
            UnderlyingIterator, UnderlyingSentinel>(
            std::move(source_begin), std::move(source_end));
    }

}

#endif
