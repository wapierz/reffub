#pragma once


#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <ranges>
#include <vector>


/**
 * @brief      Gets the first type out of variadic templates.
 *
 * @tparam     T     The first type.
 * @tparam     Ts    The rest of the types.
 */
template <class T, class... Ts>
struct first {
    using type = T;
};


/**
 * @brief      Gets the first type out of variadic templates.
 *
 * @tparam     Ts          Types
 */
template <class... Ts>
requires(sizeof...(Ts) >= 1)
using first_t = typename first<Ts...>::type;


/**
 * @brief      Checks if all types from Ts... are the same.
 *
 * @tparam     Ts          Input types.
 */
template <typename... Ts>
concept all_same =
    (sizeof...(Ts) >= 1) || (std::same_as<first_t<Ts...>, Ts> && ...);


/**
 * @brief      Concatanation view.
 *
 * @tparam     Vs         Types of views to be concatenated.
 */
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
    /**
     * @brief      Constructs a new instance of concatenation view.
     *
     * @param[in]  vs    Views to be concatenated into one.
     */
    constexpr concat_view(Vs... vs)
        : _vs{vs...},
          _joined_vs{std::views::join(std::ranges::ref_view(_vs))} {}


    /**
     * @brief      Gets iterator to the beginning of this range.
     *
     * @return     The iterator to the beginning of this range.
     */
    constexpr auto begin() const { return _joined_vs.begin(); }


    /**
     * @brief      Gets sentinel of this range.
     *
     * @return     The sentinel of this range.
     */
    constexpr auto end() const { return _joined_vs.end(); }
};


/**
 * @brief      Concatenates provided views into one view.
 *
 * @tparam     Vs    Types of views to be concatenated.
 *
 * @param[in]  vs    Views to be concatenated.
 *
 * @return     Concatenation of \p vs.
 */
template <std::ranges::view... Vs>
requires(sizeof...(Vs) >= 1) && (all_same<Vs...>)
inline constexpr auto concat(Vs&&... vs) {
    return concat_view<Vs...>(std::forward<Vs>(vs)...);
}


/**
 * @brief      This class describes a gap buffer. Recall that the content of a
 *             gap buffer consists of everything inside the buffer
 *             which is outside the gap. There is one important
 *             notion related to a gap buffer, namely a cursor. Intuitively,
 *             a cursor is just the left end of the gap. More
 *             precisely, if gap buffer is of the form
 *             _*_*_<gap>_*_*_ where * form the content of the buffer and _ are
 *             possible positions of the cursor (so think about these as about
 *             places between elements of the buffer!)
 *             then the current position of the cursor is _ occuring just
 *             before <gap>. In particular, the cursor position
 *             can be any of the range [0, content.size()]. E.g. if the
 *             current position of the cursor is 0 (content.size() resp.)
 *             and we insert element to the gap buffer then this element is
 *             pushed front (pushed back resp.).
 *
 * @tparam     T     The type held by the buffer.
 */
template <typename T>
class gap_buffer {
  private:
    using buf_t = std::vector<T>;
    static_assert(std::ranges::common_range<buf_t>);
    using buf_i = typename buf_t::iterator;
    using gap_t = std::ranges::subrange<buf_i>;

  private:
    buf_t _buf{};
    gap_t _gap{_buf};


  private:
    /**
     * @brief      Gets the current buffer size.
     *
     * @return     The current buffer size.
     */
    constexpr int64_t buf_size() const { return _buf.size(); }


    /**
     * @brief      Provides the indexes of the beginning and end of the gap.
     *
     * @return     std::pair containing the beginning and the end of the gap.
     */
    constexpr auto gap_id() const {
        auto [gb, ge] = _gap;
        return std::make_pair(gb - _buf.begin(), ge - _buf.begin());
    }


    /**
     * @brief      Provides the current gap size.
     *
     * @return     The gap size.
     */
    constexpr int64_t gap_size() const { return _gap.size(); }


  private:
    /**
     * @brief      Resizes the internal buffer. Doubling size strategy is
     *             applied.
     *
     * @param[in]  i     The size by which the buffer is to be extended. If
     *                   negative, nothing happens.
     */
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


    /**
     * @brief      Moves the cursor (the left end of the gap) to the right.
     *             Note that some enlarging might happen.
     *
     * @param[in]  count  We assume that \p count >= 0. The number of positions
     *                    by which the cursor is shifted right.
     */
    constexpr void move_cursor_right(int64_t count) {
        [[assume(count >= 0)]];
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(ge + count - buf_size());
        gap_t new_gap{_buf.begin() + gb + count, _buf.begin() + ge + count};
        std::ranges::copy(_gap.end(), new_gap.end(), _buf.begin() + gb);
        _gap = new_gap;
    }


    /**
     * @brief      Moves the cursor (the left end of the gap) left.
     *             Note that some enlarging might happen.
     *
     * @param[in]  count  We assume that \p count >= 0. The number of positions
     *                    by which the cursor is shifted left.
     */
    constexpr void move_cursor_left(int64_t count) {
        [[assume(count >= 0)]];
        auto [gb, ge] = gap_id();
        enlarge_by_at_least(count - gb);
        gap_t new_gap{_buf.begin() + gb - count, _buf.begin() + ge - count};
        std::ranges::copy_backward(
            new_gap.begin(), _gap.begin(), _buf.begin() + ge);
        _gap = new_gap;
    }


    /**
     * @brief      Moves the cursor (the left end of the gap) to a given index.
     *
     * @param[in]  index  The index to which cursor is moved.
     */
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
    /**
     * @brief      Constructs a new instance of gap buffer.
     */
    constexpr gap_buffer() {}


  public:
    /**
     * @brief      Provides a view over the content.
     *
     * @return     The view over the content.
     */
    constexpr auto view() {
        auto [gb, ge] = gap_id();
        return concat(std::ranges::subrange{_buf.begin(), _buf.begin() + gb},
                      std::ranges::subrange{_buf.begin() + ge, _buf.end()});
    }


    /**
     * @brief      Provides the size of the content.
     *
     * @return     The size of the content.
     */
    constexpr int64_t size() const { return _buf.size() - _gap.size(); }


    /**
     * @brief      Checks if the content is empty.
     *
     * @return     True iff there is no content.
     */
    constexpr int64_t empty() const { return (size() == 0); }


    /**
     * @brief      Gets the last element of the content.
     *
     * @return     A reference to the last element of the content.
     */
    constexpr T& back() {
        if (_gap.empty() || _gap.end() != _buf.end()) { return _buf.back(); }
        return *(_gap.begin() - 1);
    }


    /**
     * @brief       Gets the first element of the content.
     *
     * @return      A reference to the first element of the content.
     */
    constexpr T& front() {
        if (_gap.empty() || _gap.begin() != _buf.begin()) {
            return _buf.front();
        }
        return *(_gap.end());
    }


  public:
    /**
     * @brief      It is a procedure used to insert a view into the content at
     *             the given position belonging to the range [0, size()]. E.g.
     *             if \p index == 0 then the \p data is pushed front and if
     *             \p index == size() then the \p data is pushed back.
     *
     * @tparam     V      A view contaning elements of type T.
     *
     * @param[in]  index  A position into which the \p data is inserted.
     * @param[in]  data   Data to be inserted.
     *
     */
    template <std::ranges::view V>
    requires(std::same_as<std::ranges::range_value_t<V>, T>) &&
            (std::ranges::sized_range<V>)
    constexpr void insert(int64_t index, V data) {
        if !consteval { assert(0 <= index && index <= size()); }
        enlarge_by_at_least(data.size());
        move_cursor_to(index);
        auto [gb, ge] = gap_id();
        std::ranges::copy(data, _buf.begin() + gb);
        _gap = gap_t(_buf.begin() + gb + data.size(), _buf.begin() + ge);
    }


    /**
     * @brief      Inserts element at the given position.
     *
     * @param[in]  index  A position into which the \p t is inserted.
     * @param[in]  t      An element to be inserted.
     */
    constexpr void insert(int64_t index, T t) {
        insert(index, std::views::single(t));
    }


    /**
     * @brief      Inserts the data of view type at the cursor position.
     *
     * @param[in]  data  Data to be inserted.
     */
    constexpr void insert(std::ranges::view auto data) {
        insert(gap_id().first, data);
    }


    /**
     * @brief      Inserts an element of type T at the cursor position.
     *
     * @param[in]  t     An element to be inserted.
     */
    constexpr void insert(T t) { insert(gap_id().first, t); }


    /**
     * @brief      Pushes a view of data at the front of the content.
     *
     * @param[in]  data     Data to be inserted.
     */
    constexpr void push_front(std::ranges::view auto data) { insert(0, data); }


    /**
     * @brief      Pushes an element at the front of the content.
     *
     * @param[in]  t     Element to be inserted.
     */
    constexpr void push_front(T t) { insert(0, t); }


    /**
     * @brief      Pushes a view of data at the end of the content.
     *
     * @param[in]  data  Data to be inserted.
     */
    constexpr void push_back(std::ranges::view auto data) {
        insert(size(), data);
    }


    /**
     * @brief      Pushes an element at the end of the content.
     *
     * @param[in]  t     Element to be pushed.
     */
    constexpr void push_back(T t) { return insert(size(), t); }


    /**
     * @brief      Removes a range of elements from the content.
     *
     * @param[in]  index  The starting index of the range.
     * @param[in]  count  Might be either nonnegative or negative. If
     *                    it is negative then \p count number of elements to
     *                    the left of the \p index is removed, that is
     *                    (\p index + \p count, \p index] is removed.
     *                    Otherwise, \p  count number of elements to the right
     *                    of the \p index is removed from the content
     *                    (i.e. [\p index, \p index + \p count) is removed).
     */
    constexpr void remove(int64_t index, int64_t count) {
        [[assume(index >= 0)]];
        if (count >= 0) {
            count = std::min(count, size() - index);
            move_cursor_to(index + count);
        } else {
            count = std::min(-count, index + 1);
            move_cursor_to(index + 1);
        }
        _gap.advance(-count);
    }


    /**
     * @brief      Removes a prefix.
     *
     * @param[in]  count  The number of elements to be removed from the
     *                    beginning of the content.
     */
    constexpr void remove_prefix(int64_t count) { remove(0, count); }


    /**
     * @brief      Removes a suffix.
     *
     * @param[in]  count  The number of elements to be removed from the
     *                    end of the content.
     */
    constexpr void remove_suffix(int64_t count) { remove(size() - 1, -count); }


    /**
     * @brief      Clears the content. After this operation the size of content
     *             is zero.
     */
    constexpr void clear() {
        _buf.clear();
        _gap = gap_t{_buf};
    }
};
