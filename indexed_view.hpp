#ifndef JSS_INDEXED_VIEW_HPP
#define JSS_INDEXED_VIEW_HPP
#include <iterator>

namespace jss {
    template <typename UnderlyingIterator> class indexed_view_type {
    public:
        indexed_view_type(
            UnderlyingIterator &&begin_, UnderlyingIterator &&end_) :
            source_begin(std::move(begin_)),
            source_end(std::move(end_)) {}

        struct value_type {
            size_t index;
            decltype(*std::declval<UnderlyingIterator>()) value;
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

                value_type operator*() {
                    return std::move(value);
                }
            };

        public:
            using value_type= typename indexed_view_type::value_type;
            using reference= value_type;
            using iterator_category= std::input_iterator_tag;
            using pointer= value_type *;
            using difference_type= void;

            friend constexpr bool
            operator==(iterator const &lhs, iterator const &rhs) noexcept {
                return lhs.source_iter == rhs.source_iter;
            }

            friend constexpr bool
            operator!=(iterator const &lhs, iterator const &rhs) noexcept {
                return !(lhs == rhs);
            }

            value_type operator*() const {
                return value_type{index, *source_iter};
            }

            arrow_proxy operator->() const {
                return arrow_proxy{value_type{index, *source_iter}};
            }

            iterator &operator++() {
                ++source_iter;
                ++index;
                return *this;
            }

            postinc_return operator++(int) {
                postinc_return temp{**this};
                ++*this;
                return temp;
            }

        private:
            friend class indexed_view_type;

            iterator(size_t index_, UnderlyingIterator source_iter_) :
                index(index_), source_iter(std::move(source_iter_)) {}

            size_t index;
            UnderlyingIterator source_iter;
        };

        iterator begin() {
            return iterator(0, source_begin);
        }
        iterator end() {
            return iterator(0, source_end);
        }

    private:
        UnderlyingIterator source_begin;
        UnderlyingIterator source_end;
    };

    template <typename Range> class range_holder {
    private:
        mutable Range source_range;

    protected:
        range_holder(Range &source_) : source_range(std::move(source_)) {}

        auto get_source_begin() const
            -> decltype(std::begin(this->source_range)) {
            return std::begin(source_range);
        }
        auto get_source_end() const -> decltype(std::end(this->source_range)) {
            return std::end(source_range);
        }
    };

    template <typename Range, typename UnderlyingIterator>
    class extended_indexed_view_type
        : range_holder<Range>,
          public indexed_view_type<UnderlyingIterator> {
    public:
        extended_indexed_view_type(Range &source) :
            range_holder<Range>(source), indexed_view_type<UnderlyingIterator>(
                                             this->get_source_begin(),
                                             this->get_source_end()) {}
    };

    template <typename Range>
    auto indexed_view(Range &&source)
        -> extended_indexed_view_type<Range, decltype(std::begin(source))> {
        return extended_indexed_view_type<Range, decltype(std::begin(source))>(
            source);
    }
    template <typename Range>
    auto indexed_view(Range &source)
        -> indexed_view_type<decltype(std::begin(source))> {
        return indexed_view_type<decltype(std::begin(source))>(
            std::begin(source), std::end(source));
    }
    template <typename Range>
    auto indexed_view(Range const &source)
        -> indexed_view_type<decltype(std::begin(source))> {
        return indexed_view_type<decltype(std::begin(source))>(
            std::begin(source), std::end(source));
    }

} // namespace jss

#endif
