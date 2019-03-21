#include "indexed_view.hpp"
#include <assert.h>
#include <vector>
#include <type_traits>
#include <string>

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
    auto it2= it;

    assert(it == it2);
    assert(!(it != it2));
    ++it;
    assert(it == it);
    assert(it != it2);
    assert(!(it == it2));
    ++it;
    ++it;
    assert(it == view.end());
}

void test_view_iterator_with_range_for() {
    std::string const source[]= {"hello", "goodbye", "analysis", "dungeon"};

    std::vector<std::pair<size_t, std::string>> output;

    for(auto const &x : jss::indexed_view(source)) {
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

    for(auto const &x : jss::indexed_view(values)) {
        x.value= x.index * 2;
    }
    for(unsigned i= 0; i < count; ++i) {
        assert(values[i] == i * 2);
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
}
