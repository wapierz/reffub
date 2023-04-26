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
requires(sizeof...(Vs) >= 1) &&
        (std::same_as<first_t<Vs...>, Vs> && ...)
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


template <typename Ch>
class gap_buffer {
  private:
    using buf_t = std::vector<Ch>;
    static_assert(std::ranges::common_range<buf_t>);
    using buf_i = typename buf_t::iterator;
    using gap_t = std::ranges::subrange<buf_i>;

  private:
    buf_t _buf{};
    gap_t _gap{_buf.begin(), _buf.end()};

  protected:
    static constexpr Ch NL{'\n'};
    static constexpr std::basic_string_view<Ch> NLV{"\n"};

  private:
    constexpr int64_t buf_size() const { return _buf.size(); }


    constexpr auto gap_id() const {
        auto [gb, ge] = _gap;
        return std::make_pair(gb - _buf.begin(), ge - _buf.begin());
    }


    constexpr int64_t gap_size() const { return _gap.size(); }


  private:
    constexpr bool enlarge_by_at_least(int64_t i) {
        if (i <= 0) { return false; }
        auto old_buf_size = buf_size();
        auto new_buf_size =
            2 * (std::max(i, old_buf_size));  // strategy to double old space
        auto [gb, ge] = gap_id();
        _buf.resize(new_buf_size);
        _gap = gap_t{_buf.begin() + gb, _buf.end() - (old_buf_size - ge)};
        std::basic_string_view<Ch> old_right_data{_buf.begin() + ge,
                                                  _buf.begin() + old_buf_size};
        std::ranges::copy_backward(old_right_data, _buf.end());
        return true;
    }


    constexpr void move_cursor_right(int64_t count) {
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(ge + count - buf_size());
        gap_t new_gap{_buf.begin() + gb + count, _buf.begin() + ge + count};
        std::ranges::copy(
            _gap.end(), new_gap.end(), _buf.begin() + gap_id().first);
        _gap = new_gap;
    }


    constexpr void move_cursor_left(int64_t count) {
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(count - gb);
        gap_t new_gap{_buf.begin() + gb - count, _buf.begin() + ge - count};
        std::ranges::copy_backward(
            new_gap.begin(), _gap.begin(), _buf.begin() + gap_id().second);
        _gap = new_gap;
    }


    constexpr void move_cursor_to(int64_t index) {
        auto [gb, ge] = gap_id();
        if (index == gb) return;
        if (index > gb) {  // move right
            move_cursor_right(index - gb);
            return;
        }
        move_cursor_left(gb - index);
    }


  public:
    constexpr gap_buffer() {}


  public:
    constexpr auto view() const {
        auto [gb, ge] = gap_id();
        return concat(
            std::basic_string_view<Ch>{std::views::take(_buf, gb)},
            std::basic_string_view<Ch>{std::views::drop(_buf, ge)});
    }

   
    constexpr int64_t size() const { return _buf.size() - _gap.size(); }
   

    constexpr int64_t empty() const { return size() == 0; }
   

    constexpr Ch back() const { return view().back(); }
   

    constexpr Ch front() const { return view().front(); }


  public:
    // It is a procedure used to insert character into the text at a given
    // position. It first checks whether the gap is empty or not, if it
    // finds that the gap is empty it calls procedure grow() and resizes the
    // gap and now the element can be inserted.
    template <std::ranges::view V>
    requires(std::same_as<std::ranges::range_value_t<V>, Ch>) &&
            (std::ranges::sized_range<V>)
    constexpr auto& insert(int64_t index, V data) {
        if !consteval { assert(0 <= index && index <= size()); }
        enlarge_by_at_least(data.size());
        move_cursor_to(index);  /// watch out if index == size() then push back!
        auto [gb, ge] = gap_id();
        std::ranges::copy(data, _buf.begin() + gb);
        _gap = gap_t(_buf.begin() + gb + data.size(), _buf.begin() + ge);
        return *this;
    }


    constexpr auto& insert(int64_t index, const Ch* data) {
        return insert(index,
                      std::span{data, std::char_traits<Ch>::length(data)});
    }


    constexpr auto& insert(int64_t index, Ch c) {
        return insert(index, std::views::single(c));
    }


    constexpr auto& insert(auto data) { return insert(gap_id().first, data); }


    constexpr auto& push_front(auto data) { return insert(0, data); }


    constexpr auto& push_back(auto data) { return insert(size(), data); }


    template <bool to_the_left>
    constexpr auto& remove(int64_t count) {
        if (count == 0) { *this; }
        auto [gb, ge] = gap_id();
        if !consteval { assert(count >= 0); }
        if constexpr (to_the_left) {
            if !consteval { assert(gb >= count); }
            _gap = gap_t{_buf.begin() + (gb - count), _buf.begin() + ge};
        } else {
            if !consteval { assert(ge + count <= size()); }
            _gap = gap_t{_buf.begin() + gb, _buf.begin() + (ge + count)};
        }
        return *this;
    }


    constexpr auto& remove(int64_t index, int64_t count) {
        auto [gb, ge] = gap_id();
        if (count >= 0) {
            count = std::min(count, size() - index);
            move_cursor_to(index + count);
            return remove<true>(count);
        }
        count = std::min(-count, index);
        move_cursor_to(index);
        return remove<true>(count);
    }


    constexpr auto& remove_prefix(int64_t count) { return remove(0, count); }


    constexpr auto& remove_suffix(int64_t count) {
        return remove(size() - count, count);
    }


    constexpr auto& clear() {
        _buf.clear();
        _gap = gap_t{_buf.begin(), _buf.end()};
        return *this;
    }
};
