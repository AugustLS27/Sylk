//
// Created by August Silva on 7-3-23.
//

#ifndef SYLK_CORE_UTILS_CAST_HPP
#define SYLK_CORE_UTILS_CAST_HPP

namespace sylk {
    template<typename T>
    concept NotTriviallyCopyable = !std::is_trivially_copyable_v<T>;

    template<typename T1, typename T2>
    T1 cast(T2 val) {
        return static_cast<T1>(val);
    }

    template<typename T1, NotTriviallyCopyable T2>
    T1 cast(T2& val) {
        return static_cast<T1>(val);
    }
}

#endif // SYLK_CORE_UTILS_CAST_HPP