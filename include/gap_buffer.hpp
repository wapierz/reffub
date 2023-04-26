#pragma once


#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <ranges>
#include <vector>


template <class T1, class... T>
struct first {
    using type = T1;
};


template <class... T>
requires(sizeof...(T) >= 1)
using first_t = typename first<T...>::type;


template <typename... Ts>
concept all_same =
    (sizeof...(Ts) >= 1) || (std::same_as<first_t<Ts...>, Ts> && ...);


template <std::ranges::view... Vs>
requires(sizeof...(Vs) >= 1) && (std::same_as<first_t<Vs...>, Vs> && ...)
class concat_view : public std::ranges::view_interface<concat_view<Vs...>> {
  private:
    using container_t = std::array<first_t<Vs...>, sizeof...(Vs)>;
    using join_view_t =
        std::ranges::join_view<std::ranges::ref_view<container_t>>;

  private:
    container_t _vs;
    join_view_t _joined_vs;


  public:
    constexpr concat_view(Vs... vs)
        : _vs{vs...},
          _joined_vs{std::views::join(std::ranges::ref_view(_vs))} {}


    constexpr auto begin() const { return _joined_vs.begin(); }


    constexpr auto end() const { return _joined_vs.end(); }


    constexpr auto front() { return _joined_vs.front(); }


    constexpr auto back() { return _joined_vs.back(); }
};


template <std::ranges::view... Vs>
requires(sizeof...(Vs) >= 1) && (all_same<Vs...>)
inline constexpr auto concat(Vs&&... vs) {
    return concat_view<Vs...>(std::forward<Vs>(vs)...);
}


template <typename T>
class gap_buffer {
  private:
    using buf_t = std::vector<T>;
    static_assert(std::ranges::common_range<buf_t>);
    using buf_i = typename buf_t::iterator;
    using gap_t = std::ranges::subrange<buf_i>;

  private:
    buf_t _buf{};
    gap_t _gap{_buf.begin(), _buf.end()};

  protected:
    static constexpr T NL{'\n'};
    static constexpr std::basic_string_view<T> NLV{"\n"};

  private:
    constexpr int64_t buf_size() const { return _buf.size(); }


    constexpr auto gap_id() const {
        auto [gb, ge] = _gap;
        return std::make_pair(gb - _buf.begin(), ge - _buf.begin());
    }


    constexpr int64_t gap_size() const { return _gap.size(); }


  private:
    constexpr void enlarge_by_at_least(int64_t i) {
        if (i <= 0) { return; }
        int64_t old_buf_size = buf_size();
        int64_t new_buf_size = 2 * std::max(i, old_buf_size);
        auto [gb, ge] = gap_id();
        _buf.resize(new_buf_size);
        _gap = gap_t{_buf.begin() + gb, _buf.end() - (old_buf_size - ge)};
        std::ranges::subrange old_right_data{_buf.begin() + ge,
                                             _buf.begin() + old_buf_size};
        std::ranges::copy_backward(old_right_data, _buf.end());
    }


    constexpr void move_cursor_right(int64_t count) {
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(ge + count - buf_size());
        gap_t new_gap{_buf.begin() + gb + count, _buf.begin() + ge + count};
        std::ranges::copy(_gap.end(), new_gap.end(), _buf.begin() + gb);
        _gap = new_gap;
    }


    constexpr void move_cursor_left(int64_t count) {
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(count - gb);
        gap_t new_gap{_buf.begin() + gb - count, _buf.begin() + ge - count};
        std::ranges::copy_backward(
            new_gap.begin(), _gap.begin(), _buf.begin() + ge);
        _gap = new_gap;
    }


    constexpr void move_cursor_to(int64_t index) {
        auto [gb, ge] = gap_id();
        if (index == gb) return;
        if (index > gb) {
            move_cursor_right(index - gb);
            return;
        }
        move_cursor_left(gb - index);
    }


  public:
    constexpr gap_buffer() {}


  public:
    constexpr auto view() {
        auto [gb, ge] = gap_id();
        return concat(std::ranges::subrange{_buf.begin(), _buf.begin() + gb},
                      std::ranges::subrange{_buf.begin() + ge, _buf.end()});
    }


    constexpr int64_t size() const { return _buf.size() - _gap.size(); }


    constexpr int64_t empty() const { return (size() == 0); }


    constexpr T& back() {
        if (_gap.empty() || _gap.end() != _buf.end()) { return _buf.back(); }
        return *(_gap.begin() - 1);
    }


    constexpr T& front() {
        if (_gap.empty() || _gap.begin() != _buf.begin()) {
            return _buf.front();
        }
        return *(_gap.end() + 1);
    }


  public:
    // It is a procedure used to insert character into the text at a given
    // position in the range [0, size()].
    template <std::ranges::view V>
    requires(std::same_as<std::ranges::range_value_t<V>, T>) &&
            (std::ranges::sized_range<V>)
    constexpr auto& insert(int64_t index, V data) {
        if !consteval { assert(0 <= index && index <= size()); }
        enlarge_by_at_least(data.size());
        move_cursor_to(index);
        auto [gb, ge] = gap_id();
        std::ranges::copy(data, _buf.begin() + gb);
        _gap = gap_t(_buf.begin() + gb + data.size(), _buf.begin() + ge);
        return *this;
    }


    constexpr auto& insert(int64_t index, T t) {
        return insert(index, std::views::single(t));
    }


    constexpr auto& insert(std::ranges::view auto data) {
        return insert(gap_id().first, data);
    }


    constexpr auto& insert(T t) { return insert(gap_id().first, t); }


    constexpr auto& push_front(std::ranges::view auto data) {
        return insert(0, data);
    }


    constexpr auto& push_front(T t) { return insert(0, t); }


    constexpr auto& push_back(std::ranges::view auto data) {
        return insert(size(), data);
    }


    constexpr auto& push_back(T t) { return insert(size(), t); }


    constexpr auto& remove(int64_t index, int64_t count) {
        if (count >= 0) {
            count = std::min(count, size() - index);
            move_cursor_to(index + count);
        } else {
            count = std::min(-count, index);
            move_cursor_to(index);
        }
        _gap.advance(-count);
        return *this;
    }


    constexpr auto& remove_prefix(int64_t count) { return remove(0, count); }


    constexpr auto& remove_suffix(int64_t count) {
        return remove(size(), -count);
    }


    constexpr auto& clear() {
        _buf.clear();
        _gap = gap_t{_buf.begin(), _buf.end()};
        return *this;
    }
};
