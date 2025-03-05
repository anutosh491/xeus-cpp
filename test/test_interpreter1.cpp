/***************************************************************************
 * Copyright (c) 2023, xeus-cpp contributors
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
 ****************************************************************************/

#include <future>

#include "doctest/doctest.h"
#include "xeus-cpp/xinterpreter.hpp"

TEST_SUITE("kernel_info_request")
{
    TEST_CASE("good_status")
    {
        std::vector<const char*> Args = {/*"-v", "resource-dir", "....."*/};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());

        nl::json result = interpreter.kernel_info_request();

        REQUIRE(result["implementation"] == "xeus-cpp");
        REQUIRE(result["language_info"]["name"] == "C++");
        REQUIRE(result["language_info"]["mimetype"] == "text/x-c++src");
        REQUIRE(result["language_info"]["codemirror_mode"] == "text/x-c++src");
        REQUIRE(result["language_info"]["file_extension"] == ".cpp");
        REQUIRE(result["status"] == "ok");
    }

}