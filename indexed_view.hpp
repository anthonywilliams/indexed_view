#ifndef JSS_INDEXED_VIEW_HPP
#define JSS_INDEXED_VIEW_HPP
#include <iterator>

namespace jss {
    template <typename UnderlyingIterator> class indexed_view_type {
    public:
        indexed_view_type(UnderlyingIterator begin_, UnderlyingIterator end_) {}

        class iterator {
        public:
            friend constexpr bool
            operator==(iterator const &lhs, iterator const &rhs) noexcept {
                return true;
            }
        };

        iterator begin() {
            return iterator();
        }
        iterator end() {
            return iterator();
        }
    };

    template <typename Range>
    auto indexed_view(Range &&source)
        -> indexed_view_type<decltype(std::begin(source))> {
        return indexed_view_type<decltype(std::begin(source))>(
            std::begin(source), std::end(source));
    }

} // namespace jss

#endif
