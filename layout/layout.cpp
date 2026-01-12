// Force host-only compilation
#ifdef __clang__
#undef __clang__
#define __CUTE_CLANG_UNDEF__
#endif

#include <cute/layout.hpp>
#include <cute/tile.hpp>
#include <cute/swizzle.hpp>
#include <cute/swizzle_layout.hpp>

#ifdef __CUTE_CLANG_UNDEF__
#define __clang__ 1
#endif
#include <cstdio>

using namespace cute;

void example_1(){
    auto row_order = make_layout(make_shape(Int<10>{}, Int<5>{}),
                           make_stride(Int<5>{}, Int<1>{})); 
    cute::print_layout(row_order);
}

void example_2(){
    auto col_order = make_layout(make_shape(Int<10>{}, Int<5>{}),
                           make_stride(Int<1>{}, Int<5>{})); 
    cute::print_layout(col_order);
}

int main() {

    example_1();
    example_2();

    return 0;

}
