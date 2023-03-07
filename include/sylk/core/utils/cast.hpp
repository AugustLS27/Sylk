//
// Created by August Silva on 7-3-23.
//

#include <type_traits>

namespace sylk {

    template<typename T1, typename T2>
    T1 cast(T2 val) requires std::is_trivial_v<T2> {
        return static_cast<T1>(val);
    }

    template<typename T1, typename T2>
    T1 cast(T2& val) {
        return static_cast<T1>(val);
    }
}
