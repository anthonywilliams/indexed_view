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
            decltype(*std::declval<UnderlyingIterator>()) &value;
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

    template <typename Range>
    auto indexed_view(Range &&source)
        -> indexed_view_type<decltype(std::begin(source))> {
        return indexed_view_type<decltype(std::begin(source))>(
            std::begin(source), std::end(source));
    }

} // namespace jss

#endif
