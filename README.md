# Indexed views

[![Build Status](https://travis-ci.com/anthonywilliams/indexed_view.svg?branch=master)](https://travis-ci.com/anthonywilliams/indexed_view)

One crucial difference between using an index-based `for` loop and a range-based `for` loop is that
the former allows you to use the index for something other than just identifying the element,
whereas the latter does not provide you with access to the index at all.

`jss::indexed_view` provides a means of obtaining that index with a range-based `for` loop: it
creates a new view range which wraps the original range, where each element holds the loop index, as
well as a reference to the element of the original range.

~~~cplusplus
#include <vector>
#include <string>
#include <iostream>
#include "indexed_view.hpp"

int main(){
    std::vector<std::string> v={"hello","world","goodbye"};

    for(auto x: jss::indexed_view(v)){
        std::cout<<x.index<<": "<<x.value<<"\n";
    }
}
~~~

This program will output

~~~
0: hello
1: world
2: goodbye
~~~

## Background

The difference between index-based `for` loops and range-based `for` loops means that some people
are unable to use simple range-based `for` loops in some cases, because they need the index.

For example, you might be initializing a set of worker threads in a thread pool, and each thread
needs to know it's own index:

~~~cplusplus
std::vector<std::thread> workers;

void setup_workers(unsigned num_threads){
    workers.resize(num_threads);
    for(unsigned i=0;i<num_threads;++i){
        workers[i]=std::thread(&my_worker_thread_func,i);
    }
}
~~~

Even though `workers` has a fixed size in the loop, we need the loop index to pass to the thread
function, so we cannot use range-based `for`. This requires that we duplicate `num_threads`,
adding the potential for error as we must ensure that it is correctly updated in both places if we
ever change it.

With `jss::indexed_view`, we can avoid this duplication and use the range-based `for`:

~~~cplusplus
std::vector<std::thread> workers;

void setup_workers(unsigned num_threads){
    workers.resize(num_threads);
    for(auto entry: jss::indexed_view(workers)){
        entry.value=std::thread(&my_worker_thread_func,entry.index);
    }
}
~~~

## Details

`jss::indexed_view(range)` returns a range object `r` such that `r.begin()` and `r.end()` return
`InputIterator`s with a `value_type` that holds two elements: one (`index`) is the 0-based index
into the range and the other (`value`) is the object or reference returned by dereferencing the
underlying iterator.

### `Range` concept

An object `x` of type `X` implements the `Range` concept used here if:

- `std::begin(x)` returns a type that implements at least the `InputIterator` concept
- `std::end(x)` returns an object such that `std::begin(x)!=std::end(x)` is well-formed and returns
  a `bool`.
- Incrementing the iterator returned from `std::begin(x)` is well-defined provided
  `std::begin(x)!=std::end(x)` returns `true`.

### `jss::indexed_view` function template

~~~cplusplus
template<typename Range>
see-below indexed_view(Range& r);

template<typename Range>
see-below indexed_view(Range const& r);
~~~

**Requires:** The supplied argument `r` implements the `Range` concept.

**Effects:** Constructs an instance `v` of a class that implements the `Range` concept, as described
below. Invokes `std::begin(r)` and `std::end(r)` and stores the results in internal storage owned by
`v`. Returns `v`.

~~~cplusplus
template<typename Range>
see-below indexed_view(Range&& r);
~~~

**Requires:** The supplied argument `r` implements the `Range` concept, and is `MoveConstructible`.

**Effects:** Constructs an instance `v` of a class that implements the `Range` concept, as described
below. Move-constructs `r` into internal storage `r2` owned by `v`. Invokes `std::begin(r2)` and
`std::end(r2)` and stores the results in internal storage owned by `v`. Returns `v`.

~~~cplusplus
template<typename Iterator,typename Sentinel>
see-below indexed_view(Iterator iter,Sentinel sentinel);
~~~

**Requires:** `Iterator` is a type that implements at least the `InputIterator`
concept. `iter!=sentinel` is well-formed and returns a `bool`. Incrementing `iter` is well-defined
if `iter!=sentinel` returns `true`. `Iterator` and `Sentinel` are `MoveConstructible`.

**Effects:** Constructs an instance `v` of a class that implements the `Range` concept, as described
below. Move-constructs `it` and `sentinel` into internal storage owned by `v`. Returns `v`.

### The indexed-view-range 

Given a range `r` of type `R`, `jss::indexed_view(r)` returns a range type with the behaviour
described below for `jss::indexed_view(std::begin(r),std::end(r))` in addition to any described in
the specification of `jss::indexed_view(r)` above.

Given an iterator/sentinel pair `iter` of type `Iter` and `sent` of type `Sentinel`, the range type
returned from `jss::indexed_view(iter,sent)` is as follows:

~~~cplusplus
class internal-indexed-view-range-type
{
public:
    internal-indexed-view-range-type(internal-indexed-view-range-type&&);
    
    class value_type{
        size_t index;
        decltype(*std::declval<Iter&>()) value;
    };
    class iterator;
    class sentinel;
    
    iterator begin();
    sentinel end();
};
~~~

The `value_type` of the iterator is the nested `value_type` member of the range type. The
`iterator_category` of the iterator is `std::input_iterator_tag`. Comparing an instance of
`iterator` against an instance of `sentinel` returns the same as the equivalent comparison on the
internal `Iter` and `Sentinel` values.

## License

This code is released under the [Boost Software License](https://www.boost.org/LICENSE_1_0.txt):

> Boost Software License - Version 1.0 - August 17th, 2003
>
> Permission is hereby granted, free of charge, to any person or organization
> obtaining a copy of the software and accompanying documentation covered by
> this license (the "Software") to use, reproduce, display, distribute,
> execute, and transmit the Software, and to prepare derivative works of the
> Software, and to permit third-parties to whom the Software is furnished to
> do so, all subject to the following:
>
> The copyright notices in the Software and this entire statement, including
> the above license grant, this restriction and the following disclaimer,
> must be included in all copies of the Software, in whole or in part, and
> all derivative works of the Software, unless such copies or derivative
> works are solely in the form of machine-executable object code generated by
> a source language processor.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
> SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
> FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
> ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
> DEALINGS IN THE SOFTWARE.

