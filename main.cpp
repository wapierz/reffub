#include <iostream>

#include "gap_buffer.hpp"


constexpr bool equal(auto str1, std::string_view str2) {
    return std::ranges::equal(str1, str2);
}


consteval auto test() {
    gap_buffer<char> gb;
    bool t1 = gb.empty() && gb.size() == 0;
    gb.push_back("gap buffer");
    bool t2 = equal(gb.view(), "gap buffer");
    gb.remove(0, 100);
    bool t3 = gb.empty();
    gb.push_back("gap buffer").clear();
    bool t4 = gb.empty();
    gb.push_front("gap buffer");
    bool t5 = equal(gb.view(), "gap buffer");
    gb.insert(" abc");
    bool t6 = equal(gb.view(), "gap buffer abc");
    gb.push_back(" efg");
    bool t7 = equal(gb.view(), "gap buffer abc efg");
    gb.push_front("--- ");
    bool t8 = equal(gb.view(), "--- gap buffer abc efg");
    gb.insert("***");
    bool t9 = equal(gb.view(), "--- ***gap buffer abc efg");
    gb.insert('#');
    bool t10 = equal(gb.view(), "--- ***#gap buffer abc efg");
    gb.remove_prefix(0);
    bool t11 = equal(gb.view(), "--- ***#gap buffer abc efg");
    gb.remove_prefix(4);
    bool t12 = equal(gb.view(), "***#gap buffer abc efg");
    gb.remove_suffix(4);
    bool t13 = equal(gb.view(), "***#gap buffer abc");
    gb.remove(4, 4);
    bool t14 = equal(gb.view(), "***#buffer abc");
    gb.insert("&&&");
    bool t15 = equal(gb.view(), "***#&&&buffer abc");
    bool t16 = gb.back() == 'c';
    bool t17 = gb.front() == '*';
    // clang-format off
    return std::array{t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,
        t11, t12, t13, t14, t15, t16, t17};
    // clang-format on
}

int main(int argc, char const *argv[]) {
    auto results = test();
    for (auto [id, res] : std::views::enumerate(results)) {
        std::cout << "test " << id
                  << ((res) ? std::string_view{" passed"}
                            : std::string_view{" failed"})
                  << "\n";
    }

    return 0;
}