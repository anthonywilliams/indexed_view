#ifndef JSS_INDEXED_VIEW_HPP
#define JSS_INDEXED_VIEW_HPP
#include <iterator>

namespace jss {
    template <typename UnderlyingIterator, typename UnderlyingSentinel>
    class indexed_view_type {
    private:
        static constexpr bool nothrow_move_iterators=
            std::is_nothrow_move_constructible<UnderlyingIterator>::value;
        static constexpr bool nothrow_move_sentinels=
            std::is_nothrow_move_constructible<UnderlyingSentinel>::value;
        static constexpr bool nothrow_comparable_iterators= noexcept(
            std::declval<UnderlyingIterator &>() !=
            std::declval<UnderlyingSentinel &>());
        static constexpr bool nothrow_iterator_increment=
            noexcept(++std::declval<UnderlyingIterator &>());

        using underlying_value_type=
            decltype(*std::declval<UnderlyingIterator &>());
        static constexpr bool nothrow_deref=
            noexcept(*std::declval<UnderlyingIterator &>());

    public:
        indexed_view_type(
            UnderlyingIterator &&begin_,
            UnderlyingSentinel &&end_) noexcept(nothrow_move_iterators) :
            source_begin(std::move(begin_)),
            source_end(std::move(end_)) {}

        struct value_type {
            size_t index;
            underlying_value_type value;
        };

        class iterator;

        class sentinel {
            friend class iterator;
            friend class indexed_view_type;

            sentinel(UnderlyingSentinel &end_) noexcept(
                nothrow_move_sentinels) :
                end(std::move(end_)) {}

            mutable UnderlyingSentinel end;
        };

        class iterator {
            struct arrow_proxy {
                value_type *operator->() noexcept {
                    return &value;
                }

                value_type value;
            };

            struct postinc_return {
                value_type value;

                value_type
                operator*() noexcept(std::is_nothrow_move_constructible<
                                     UnderlyingIterator>::value) {
                    return std::move(value);
                }
            };

        public:
            using value_type= typename indexed_view_type::value_type;
            using reference= value_type;
            using iterator_category= std::input_iterator_tag;
            using pointer= value_type *;
            using difference_type= void;

            friend constexpr bool operator!=(
                iterator const &lhs,
                sentinel const &rhs) noexcept(nothrow_comparable_iterators) {
                return lhs.source_iter != get_end(rhs);
            }

            friend constexpr bool operator==(
                iterator const &lhs,
                sentinel const &rhs) noexcept(nothrow_comparable_iterators) {
                return !(lhs != rhs);
            }

            value_type operator*() const noexcept(
                nothrow_deref
                    &&std::is_nothrow_move_constructible<value_type>::value) {
                return value_type{index, *source_iter};
            }

            arrow_proxy operator->() const noexcept(
                nothrow_deref
                    &&std::is_nothrow_move_constructible<value_type>::value) {
                return arrow_proxy{value_type{index, *source_iter}};
            }

            iterator &operator++() noexcept(nothrow_iterator_increment) {
                ++source_iter;
                ++index;
                return *this;
            }

            postinc_return operator++(int) noexcept(
                nothrow_iterator_increment &&nothrow_deref &&std::
                    is_nothrow_move_constructible<UnderlyingIterator>::value) {
                postinc_return temp{**this};
                ++*this;
                return temp;
            }

        private:
            friend class indexed_view_type;

            static UnderlyingSentinel const &
            get_end(sentinel const &s) noexcept {
                return s.end;
            }

            iterator(size_t index_, UnderlyingIterator &source_iter_) noexcept(
                nothrow_move_iterators) :
                index(index_),
                source_iter(std::move(source_iter_)) {}

            size_t index;
            mutable UnderlyingIterator source_iter;
        };

        iterator begin() noexcept(nothrow_move_iterators) {
            return iterator(0, source_begin);
        }
        sentinel end() noexcept(nothrow_move_iterators) {
            return sentinel(source_end);
        }

    private:
        UnderlyingIterator source_begin;
        UnderlyingSentinel source_end;
    };

    template <typename Range> class range_holder {
    private:
        mutable Range source_range;

    protected:
        range_holder(Range &source_) noexcept(
            std::is_nothrow_move_constructible<Range>::value) :
            source_range(std::move(source_)) {}

        auto get_source_begin() const
            noexcept(noexcept(std::begin(std::declval<Range &>())))
                -> decltype(std::begin(this->source_range)) {
            return std::begin(source_range);
        }
        auto get_source_end() const
            noexcept(noexcept(std::end(std::declval<Range &>())))
                -> decltype(std::end(this->source_range)) {
            return std::end(source_range);
        }
    };

    template <
        typename Range, typename UnderlyingIterator,
        typename UnderlyingSentinel>
    class extended_indexed_view_type
        : range_holder<Range>,
          public indexed_view_type<UnderlyingIterator, UnderlyingSentinel> {
    public:
        extended_indexed_view_type(Range &source) noexcept(
            std::is_nothrow_move_constructible<Range>::value
                &&std::is_nothrow_move_constructible<UnderlyingIterator>::value
                    &&std::is_nothrow_move_constructible<UnderlyingSentinel>::
                        value &&noexcept(std::begin(std::declval<Range &>())) &&
            noexcept(std::end(std::declval<Range &>()))) :
            range_holder<Range>(source),
            indexed_view_type<UnderlyingIterator, UnderlyingSentinel>(
                this->get_source_begin(), this->get_source_end()) {}
    };

    template <typename Range>
    auto indexed_view(Range &&source) -> extended_indexed_view_type<
        Range, decltype(std::begin(source)), decltype(std::end(source))> {
        return extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))>(
            source);
    }
    template <typename Range>
    auto indexed_view(Range &source) -> indexed_view_type<
        decltype(std::begin(source)), decltype(std::end(source))> {
        return indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }
    template <typename Range>
    auto indexed_view(Range const &source) -> indexed_view_type<
        decltype(std::begin(source)), decltype(std::end(source))> {
        return indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }

} // namespace jss

#endif
