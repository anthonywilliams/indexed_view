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
