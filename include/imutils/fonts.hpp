//
// Created by wanjiangzhi on 2025/11/17.
//

#ifndef IM_UTILS_FONTS_HPP
#define IM_UTILS_FONTS_HPP

#include <ranges>
#include <imgui/imgui.h>
#include <utility>
#include <vector>

#ifdef _MSVC_LANG
#define _cpp_ver _MSVC_LANG
#else
#define _cpp_ver __cplusplus
#endif

#if _cpp_ver < 201103L
#error "Must Support C++ 11"
#endif

namespace ImUtils {
    class Glyph {
        void VecInit() { m_Range.clear(); }
        void VecAddItem(const ImWchar item) { m_Range.emplace_back(item); }
        void VecFinish() { if (!m_Range.empty() && m_Range.back() != 0) VecAddItem(0); }
    public:
        using base_t = std::vector<ImWchar>;
        using pair_t = std::pair<ImWchar, ImWchar>;
        using list_t = std::initializer_list<pair_t>;
        using chars_t = std::initializer_list<ImWchar>;

        template <typename... Args>
        static Glyph Build(Args&&... args) {
            return Glyph(std::forward<Args>(args)...);
        }
        template <typename... Args>
        static auto Add(Glyph& obj, Args&&... args) {
            return obj.add(std::forward<Args>(args)...);
        }
        template <typename... Args>
        static auto Add(Glyph* obj, Args&&... args) {
            return obj->add(std::forward<Args>(args)...);
        }

        Glyph() {
            VecInit();
            VecFinish();
        }
        Glyph(const list_t& ranges) {
            VecInit();
            for (pair_t range : ranges) {
                #if _cpp_ver >= 201703L // 结构化绑定实践
                auto& [fst, snd] = range;
                if (fst > snd)
                    std::swap(fst, snd);
                VecAddItem(fst);
                VecAddItem(snd);
                #else
                if (range.first > range.second)
                    std::swap(range.first, range.second);
                VecAddItem(range.first);
                VecAddItem(range.second);
                #endif
            }
            VecFinish();
        }

        template <typename arr_t, std::size_t arr_c>
        explicit Glyph(arr_t (&arr)[arr_c]) {
            static_assert(
                std::
                #if _cpp_ver >= 201703L
                is_same_v<arr_t, ImWchar>
                #else
                is_same<arr_t, ImWchar>::value
                #endif
               ,
                "Glyph C-array constructor only accepts ImWchar arrays!"
            );

            VecInit();
            for (std::size_t i = 0; i < arr_c; i++) VecAddItem(arr[i]);
            VecFinish();
        }

        Glyph(const chars_t& ranges) {
            VecInit();
            for (const auto& range : ranges)
                VecAddItem(range);
            VecFinish();
        }

        explicit Glyph(base_t vec) : m_Range(std::move(vec)) {
            VecFinish();
        }

        void add(ImWchar from, ImWchar to) {
            if (!m_Range.empty() && m_Range.back() == 0)
                m_Range.pop_back();
            if (from > to)
                std::swap(from, to);
            m_Range.emplace_back(from);
            m_Range.emplace_back(to);
            VecFinish();
        }

        void add(const pair_t range) {
            add(range.first, range.second);
        }

        void add(const std::initializer_list<pair_t>& ranges) {
            for (const auto& range : ranges)
                add(range);
        }

        template <typename arr_t, std::size_t arr_c>
        void add(arr_t (&arr)[arr_c]) {
            static_assert(
                std::
                #if _cpp_ver >= 201703L
                is_same_v<arr_t, ImWchar>
                #else
                is_same<arr_t, ImWchar>::value
                #endif
               ,
                "Glyph.add C-array function only accepts ImWchar arrays!"
            );
            if (!m_Range.empty() && m_Range.back() == 0)
                m_Range.pop_back();
            for (std::size_t i = 0; i < arr_c; i++) m_Range.emplace_back(arr[i]);
            VecFinish();
        }

        void remove(const ImWchar from, const ImWchar to) {
            std::erase(m_Range, from);
            std::erase(m_Range, to);
        }
        void removeAt(const size_t index) {
            if (index >= m_Range.size()) return;
            // NOLINTNEXTLINE(*-narrowing-conversions)
            m_Range.erase(m_Range.begin() + index);
        }

        [[nodiscard]] const ImWchar* data() const {
            if (m_Range.empty()) return nullptr;
            return m_Range.data();
        }

        [[nodiscard]] size_t size() const {
            if (m_Range.empty()) return 0;
            return m_Range.size() - 1;
        }

        base_t* ptr() {
            return std::addressof(m_Range);
        }

        explicit operator base_t() const {
            return m_Range;
        }

        // NOLINTNEXTLINE(*-explicit-constructor)
        operator const ImWchar*() const {
            return data();
        }

        base_t& operator*() {
            return m_Range;
        }

        base_t* operator->() {

            return ptr();
        }

    protected:
        base_t m_Range;
    };


}

#endif //IM_UTILS_FONTS_HPP
