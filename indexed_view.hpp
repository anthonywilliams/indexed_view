#ifndef JSS_INDEXED_VIEW_HPP
#define JSS_INDEXED_VIEW_HPP
#include <iterator>
#include <variant>
#include <stddef.h>

namespace jss {
    template <typename UnderlyingIterator, typename UnderlyingSentinel>
    class indexed_view_type {
    private:
        static constexpr bool nothrow_move_iterators=
            std::is_nothrow_move_constructible<UnderlyingIterator>::value;
        static constexpr bool nothrow_move_sentinels=
            std::is_nothrow_move_constructible<UnderlyingSentinel>::value;
        static constexpr bool nothrow_copy_iterators=
            std::is_nothrow_copy_constructible<UnderlyingIterator>::value;
        static constexpr bool nothrow_copy_sentinels=
            std::is_nothrow_copy_constructible<UnderlyingSentinel>::value;
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
            UnderlyingSentinel &&end_) noexcept(nothrow_move_iterators
                                                    &&nothrow_move_sentinels) :
            source_begin(std::move(begin_)),
            source_end(std::move(end_)) {}

        struct value_type {
            size_t index;
            underlying_value_type value;
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

                const value_type operator*() noexcept(
                    std::is_nothrow_move_constructible<value_type>::value) {
                    return std::move(value);
                }
            };

        public:
            using value_type= typename indexed_view_type::value_type;
            using reference= value_type;
            using iterator_category= std::input_iterator_tag;
            using pointer= value_type *;
            using difference_type= void;

            friend bool operator!=(
                iterator const &lhs,
                iterator const &rhs) noexcept(nothrow_comparable_iterators) {
                if(lhs.is_iterator()) {
                    if(rhs.is_iterator()) {
                        return lhs.get_iterator_state().index !=
                               rhs.get_iterator_state().index;
                    } else {
                        return lhs.get_iterator_state().source_iter !=
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

            friend bool operator==(
                iterator const &lhs,
                iterator const &rhs) noexcept(nothrow_comparable_iterators) {
                return !(lhs != rhs);
            }

            const value_type operator*() const noexcept(
                nothrow_deref
                    &&std::is_nothrow_move_constructible<value_type>::value) {
                auto &iter= get_iterator_state();
                return value_type{iter.index, *iter.source_iter};
            }

            arrow_proxy operator->() const noexcept(
                nothrow_deref
                    &&std::is_nothrow_move_constructible<value_type>::value) {
                auto &iter= get_iterator_state();
                return arrow_proxy{value_type{iter.index, *iter.source_iter}};
            }

            iterator &operator++() noexcept(nothrow_iterator_increment) {
                auto &iter= get_iterator_state();
                ++iter.source_iter;
                ++iter.index;
                return *this;
            }

            postinc_return operator++(int) noexcept(
                nothrow_iterator_increment &&nothrow_deref
                    &&std::is_nothrow_move_constructible<value_type>::value) {
                postinc_return temp{**this};
                ++*this;
                return temp;
            }

        private:
            friend class indexed_view_type;
            struct iterator_state {
                size_t index;
                UnderlyingIterator source_iter;
            };

            bool is_iterator() const noexcept {
                return std::holds_alternative<iterator_state>(state);
            }

            UnderlyingSentinel &get_sentinel() const noexcept {
                return std::get<UnderlyingSentinel>(state);
            }

            iterator_state &get_iterator_state() const noexcept {
                return std::get<iterator_state>(state);
            }

            iterator(size_t index_, UnderlyingIterator &source_iter_) noexcept(
                nothrow_copy_iterators) :
                state(iterator_state{index_, source_iter_}) {}

            iterator(UnderlyingSentinel &sentinel_) noexcept(
                nothrow_copy_sentinels) :
                state(sentinel_) {}

            mutable std::variant<iterator_state, UnderlyingSentinel> state;
        };

        iterator begin() noexcept(nothrow_copy_iterators) {
            return iterator(0, source_begin);
        }
        iterator end() noexcept(nothrow_copy_sentinels) {
            return iterator(source_end);
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
    auto indexed_view(Range &&source) noexcept(noexcept(
        extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))>(
            source)))
        -> extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))> {
        return extended_indexed_view_type<
            Range, decltype(std::begin(source)), decltype(std::end(source))>(
            source);
    }
    template <typename Range>
    auto indexed_view(Range &source) noexcept(
        noexcept(indexed_view_type<
                 decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source))))
        -> indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))> {
        return indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }
    template <typename Range>
    auto indexed_view(Range const &source) noexcept(
        noexcept(indexed_view_type<
                 decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source))))
        -> indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))> {
        return indexed_view_type<
            decltype(std::begin(source)), decltype(std::end(source))>(
            std::begin(source), std::end(source));
    }
    template <typename UnderlyingIterator, typename UnderlyingSentinel>
    auto
    indexed_view(UnderlyingIterator source_begin, UnderlyingSentinel source_end) noexcept(
        noexcept(indexed_view_type<UnderlyingIterator, UnderlyingSentinel>(
            std::move(source_begin), std::move(source_end))))
        -> indexed_view_type<UnderlyingIterator, UnderlyingSentinel> {
        return indexed_view_type<UnderlyingIterator, UnderlyingSentinel>(
            std::move(source_begin), std::move(source_end));
    }

} // namespace jss

#endif
