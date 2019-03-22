#include "indexed_view.hpp"
#include <assert.h>
#include <vector>
#include <type_traits>
#include <string>
#include <deque>
#include <algorithm>

void test_indexed_view_is_empty_for_empty_vector() {
    std::vector<int> v;
    auto view= jss::indexed_view(v);

    assert(view.begin() == view.end());
}

void test_indexed_view_iterator_has_index_and_value_of_source() {
    std::vector<int> v;
    auto view= jss::indexed_view(v);

    static_assert(
        std::is_same<
            typename decltype(view)::iterator, decltype(view.begin())>::value,
        "begin must return an iterator");
    static_assert(
        std::is_same<decltype((*view.begin()).index), size_t>::value,
        "index is size_t");
    static_assert(
        std::is_same<decltype((*view.begin()).value), int &>::value,
        "value is ref");
}

void test_dereferencing_begin_iterator_of_indexed_view_gives_0_and_element() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);

    assert((*view.begin()).index == 0);
    assert(&(*view.begin()).value == &v[0]);
}

void test_begin_and_end_of_non_empty_range_are_not_equal() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);
    assert(!(view.begin() == view.end()));
}

void test_can_use_arrow_operator_on_iterator() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);

    assert(view.begin()->index == 0);
    assert(&view.begin()->value == &v[0]);
    static_assert(
        std::is_same<decltype(view.begin()->index), size_t>::value,
        "index is size_t");
    static_assert(
        std::is_same<decltype(view.begin()->value), int &>::value,
        "value is ref");
}

void test_can_increment_view_iterator() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);

    assert(view.begin()->index == 0);
    assert(&view.begin()->value == &v[0]);

    auto it= view.begin();
    ++it;
    assert(it->index == 1);
    assert(&it->value == &v[1]);
}

void test_preincrement_view_iterator() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);

    assert(view.begin()->index == 0);
    assert(&view.begin()->value == &v[0]);

    auto it= view.begin();
    decltype(*it) val= *it++;
    assert(it->index == 1);
    assert(&it->value == &v[1]);
    assert(val.index == 0);
    assert(&val.value == &v[0]);
}

void test_view_iterator_has_iterator_properties() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);
    static_assert(
        std::is_same<
            typename decltype(view.begin())::value_type,
            typename decltype(view.begin())::reference>::value,
        "Value type and reference are the same");
    static_assert(
        std::is_same<
            typename decltype(view.begin())::value_type *,
            typename decltype(view.begin())::pointer>::value,
        "Pointer is pointer to value type");
    static_assert(
        std::is_same<
            typename decltype(view.begin())::iterator_category,
            std::input_iterator_tag>::value,
        "Input iterators");
    static_assert(
        std::is_same<
            typename decltype(view.begin())::difference_type, void>::value,
        "No difference type");
}

void test_view_iterator_equality_comparisons() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);

    auto it= view.begin();
    auto it2= view.end();

    assert(!(it == it2));
    assert(it != it2);
    ++it;
    assert(it != it2);
    assert(!(it == it2));
    ++it;
    ++it;
    assert(it == it2);
    assert(!(it != it2));
}

void test_view_iterator_with_range_for() {
    std::string const source[]= {"hello", "goodbye", "analysis", "dungeon"};

    std::vector<std::pair<size_t, std::string>> output;

    for(auto &x : jss::indexed_view(source)) {
        output.push_back({x.index, x.value});
    }

    assert(output.size() == 4);
    for(unsigned i= 0; i < output.size(); ++i) {
        assert(output[i].first == i);
        assert(output[i].second == source[i]);
    }
}

void test_can_write_through_value_in_range_for() {
    unsigned const count= 5;
    int values[count]= {0};

    for(auto &x : jss::indexed_view(values)) {
        x.value= x.index * 2;
    }
    for(unsigned i= 0; i < count; ++i) {
        assert(values[i] == i * 2);
    }
}

template <typename T> struct IncrementValue {
    void operator()(T &x) const {
        ++x;
    }
};

template <typename T> struct IncrementBy {

    T delta;

    IncrementBy(T delta_) : delta(std::move(delta_)) {}

    void operator()(T &x) const {
        x+= delta;
    }
};

template <typename T, typename Increment= IncrementValue<T>>
class numeric_range {
public:
    enum class direction { increasing, decreasing };

private:
    T current;
    T final;
    Increment inc;
    direction dir;

    bool at_end() {
        if(dir == direction::increasing) {
            return current >= final;
        } else {
            return current <= final;
        }
    }

public:
    class iterator {
        numeric_range *range;

        void check_done() {
            if(range->at_end()) {
                range= nullptr;
            }
        }

        class postinc_return {
            T value;

        public:
            postinc_return(T value_) : value(std::move(value_)) {}
            T operator*() {
                return std::move(value);
            }
        };

    public:
        using value_type= T;
        using reference= T;
        using iterator_category= std::input_iterator_tag;
        using pointer= T *;
        using difference_type= void;

        iterator(numeric_range *range_) : range(range_) {
            if(range)
                check_done();
        }

        T operator*() const {
            return range->current;
        }

        T *operator->() const {
            return &range->current;
        }

        iterator &operator++() {
            if(!range)
                throw std::runtime_error("Increment a past-the-end iterator");
            range->inc(range->current);
            check_done();
            return *this;
        }

        postinc_return operator++(int) {
            postinc_return temp(**this);
            ++*this;
            return temp;
        }

        friend bool operator==(iterator const &lhs, iterator const &rhs) {
            return lhs.range == rhs.range;
        }
        friend bool operator!=(iterator const &lhs, iterator const &rhs) {
            return !(lhs == rhs);
        }
    };

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator(nullptr);
    }

    numeric_range(T initial_, T final_) :
        current(std::move(initial_)), final(std::move(final_)),
        dir(direction::increasing) {}
    numeric_range(T initial_, T final_, Increment inc_) :
        current(std::move(initial_)), final(std::move(final_)),
        inc(std::move(inc_)), dir(direction::increasing) {}
    numeric_range(T initial_, T final_, Increment inc_, direction dir_) :
        current(std::move(initial_)), final(std::move(final_)),
        inc(std::move(inc_)), dir(dir_) {}
};

template <typename T> numeric_range<T> range(T from, T to) {
    if(to < from)
        throw std::runtime_error("Cannot count down ");
    return numeric_range<T>(std::move(from), std::move(to));
}

void test_can_index_input_ranges() {
    std::vector<std::pair<size_t, unsigned>> output;

    unsigned const base= 5;
    unsigned const count= 20;

    for(auto &x : jss::indexed_view(range(base, base + count))) {
        output.push_back({x.index, x.value});
    }

    assert(
        !std::is_reference<decltype(jss::indexed_view(range(base, base + count))
                                        .begin()
                                        ->value)>::value);

    assert(output.size() == count);
    for(unsigned i= 0; i < output.size(); ++i) {
        assert(output[i].first == i);
        assert(output[i].second == (base + i));
    }
}

struct my_range {
    static const unsigned max= 3;

    size_t values[max];

    my_range() {
        for(auto x : jss::indexed_view(values)) {
            x.value= x.index * 2;
        }
    }

    class sentinel {};

    struct iterator {
        my_range *range;
        unsigned index;

        iterator(my_range *range_) : range(range_), index(0) {}

        size_t operator*() {
            return range->values[index];
        }

        iterator &operator++() {
            ++index;
            return *this;
        }

        iterator operator++(int) {
            iterator temp(*this);
            ++*this;
            return temp;
        }

        friend bool operator!=(iterator const &lhs, sentinel const &rhs) {
            return lhs.index != max;
        }
    };

    iterator begin() {
        return iterator(this);
    }

    sentinel end() {
        return sentinel();
    }
};

void test_can_index_ranges_with_sentinels() {
    my_range r;

    unsigned i= 0;

    for(auto x : r) {
        assert(x == i * 2);
        ++i;
    }

    for(auto x : jss::indexed_view(r)) {
        assert(x.value == (2 * x.index));
    }
}

void test_can_index_iterator_pairs() {
    std::deque<int> const d= {1, 45, 67, 98, 123, -45};

    unsigned count= 0;
    for(auto &x : jss::indexed_view(d.begin(), d.end())) {
        assert(x.index == count);
        assert(&x.value == &d[count]);
        ++count;
    }
    assert(count == d.size());
}

void test_can_index_iterator_sentinel_pairs() {
    my_range r;

    unsigned i= 0;

    for(auto &x : jss::indexed_view(r.begin(), r.end())) {
        assert(x.value == (2 * x.index));
        assert(x.index == i);
        ++i;
    }
    assert(i == my_range::max);
}

void test_can_reuse_view_if_underlying_range_stable() {
    std::vector<int> v{42, 56, 99};
    auto view= jss::indexed_view(v);
    unsigned i= 0;

    for(auto &x : view) {
        assert(x.index == i);
        assert(&x.value == &v[i]);
        ++i;
    }
    assert(i == v.size());

    i= 0;
    for(auto &x : view) {
        assert(x.index == i);
        assert(&x.value == &v[i]);
        ++i;
    }
    assert(i == v.size());
}

void test_can_use_view_with_standard_algorithms() {
    std::vector<int> v;
    v.resize(100);
    auto view= jss::indexed_view(v);

    std::for_each(view.begin(), view.end(), [](auto &x) { x.value= x.index; });

    unsigned i= 0;
    for(auto &x : v) {
        assert(x == i);
        ++i;
    }
}

int main() {
    test_indexed_view_is_empty_for_empty_vector();
    test_indexed_view_iterator_has_index_and_value_of_source();
    test_dereferencing_begin_iterator_of_indexed_view_gives_0_and_element();
    test_begin_and_end_of_non_empty_range_are_not_equal();
    test_can_use_arrow_operator_on_iterator();
    test_can_increment_view_iterator();
    test_preincrement_view_iterator();
    test_view_iterator_has_iterator_properties();
    test_view_iterator_equality_comparisons();
    test_view_iterator_with_range_for();
    test_can_write_through_value_in_range_for();
    test_can_index_input_ranges();
    test_can_index_ranges_with_sentinels();
    test_can_index_iterator_pairs();
    test_can_index_iterator_sentinel_pairs();
    test_can_reuse_view_if_underlying_range_stable();
    test_can_use_view_with_standard_algorithms();
}
