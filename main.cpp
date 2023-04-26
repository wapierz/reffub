#include <iostream>

#include "gap_buffer.hpp"


constexpr bool equal(auto str1, std::string_view str2) {
    return std::ranges::equal(str1, str2);
}


consteval auto test() {
    using namespace std::string_view_literals;
    gap_buffer<char> gb;
    bool t1 = gb.empty() && gb.size() == 0;
    gb.push_back("gap buffer"sv);
    bool t2 = equal(gb.view(), "gap buffer"sv);
    gb.remove(0, 100);
    bool t3 = gb.empty();
    gb.push_back("gap buffer"sv);
    gb.clear();
    bool t4 = gb.empty();
    gb.push_front("gap buffer"sv);
    bool t5 = equal(gb.view(), "gap buffer"sv);
    gb.insert(" abc"sv);
    bool t6 = equal(gb.view(), "gap buffer abc"sv);
    gb.push_back(" efg"sv);
    bool t7 = equal(gb.view(), "gap buffer abc efg"sv);
    gb.push_front("--- "sv);
    bool t8 = equal(gb.view(), "--- gap buffer abc efg"sv);
    gb.insert("***"sv);
    bool t9 = equal(gb.view(), "--- ***gap buffer abc efg"sv);
    gb.insert('#');
    bool t10 = equal(gb.view(), "--- ***#gap buffer abc efg"sv);
    gb.remove_prefix(0);
    bool t11 = equal(gb.view(), "--- ***#gap buffer abc efg"sv);
    gb.remove_prefix(4);
    bool t12 = equal(gb.view(), "***#gap buffer abc efg"sv);
    gb.remove_suffix(4);
    bool t13 = equal(gb.view(), "***#gap buffer abc"sv);
    gb.remove(4, 4);
    bool t14 = equal(gb.view(), "***#buffer abc"sv);
    gb.insert("&&&"sv);
    bool t15 = equal(gb.view(), "***#&&&buffer abc"sv);
    bool t16 = gb.back() == 'c';
    bool t17 = gb.front() == '*';
    // clang-format off
    return std::array{t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,
        t11, t12, t13, t14, t15, t16, t17};
    // clang-format on
}

void test2() {
    gap_buffer<int> gb;
    std::array a{1, 2, 3, 4, 5, 6, 7, 8, 9};
    gb.insert(std::views::all(a));
    auto gbv = gb.view();
    *gbv.begin() = 100;
    gb.back() = 500;
    for (auto c : gbv) { std::cout << c << ", "; }
    std::cout << "\n";
    gb.remove(3, -1);
    std::cout << "after removing 4th element\n";
    for (auto c : gb.view()) { std::cout << c << ", "; }
    std::cout << "\n";
    gb.remove(1, -2);
    std::cout << "after removing 1th  and 2nd elements\n";
    for (auto c : gb.view()) { std::cout << c << ", "; }
    std::cout << "\n";
    gb.insert(1, 33);
    std::cout << "after inserting 33 at index 1\n";
    for (auto c : gb.view()) { std::cout << c << ", "; }
    std::cout << "\n";
}


int main(int argc, char const *argv[]) {
    constexpr auto results = test();
    for (auto [id, res] : std::views::enumerate(results)) {
        std::cout << "test " << id + 1
                  << (res ? std::string_view{" passed"}
                          : std::string_view{" failed"})
                  << "\n";
    }
    test2();
    return 0;
}