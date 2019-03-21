#include "indexed_view.hpp"
#include <assert.h>
#include <vector>

void test_indexed_view_is_empty_for_empty_vector() {
    std::vector<int> v;

    auto view= jss::indexed_view(v);

    assert(view.begin() == view.end());
}
int main() {
    test_indexed_view_is_empty_for_empty_vector();
}
