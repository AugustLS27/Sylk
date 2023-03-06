//
// Created by August Silva on 5-3-23.
//

#pragma once

namespace vk {
    enum class Result;
}

namespace sylk {
    void handle_result(vk::Result result, const char* error_message, bool terminate = false);
}
