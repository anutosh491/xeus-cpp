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
#include "xeus-cpp/xholder.hpp"
#include "xeus-cpp/xmanager.hpp"
#include "xeus-cpp/xutils.hpp"
#include "xeus-cpp/xoptions.hpp"
#include "xeus-cpp/xeus_cpp_config.hpp"
#include "xcpp/xmime.hpp"

#include "../src/xparser.hpp"
#include "../src/xsystem.hpp"
#include "../src/xmagics/os.hpp"
#include "../src/xmagics/xassist.hpp"
#include "../src/xinspect.hpp"


#include <iostream>
#include <pugixml.hpp>
#include <fstream>
#if defined(__GNUC__) && !defined(XEUS_CPP_EMSCRIPTEN_WASM_BUILD)
    #include <sys/wait.h>
    #include <unistd.h>
#endif


/// A RAII class to redirect a stream to a stringstream.
///
/// This class redirects the output of a given std::ostream to a std::stringstream.
/// The original stream is restored when the object is destroyed.
class StreamRedirectRAII {
    public:

        /// Constructor that starts redirecting the given stream.
        StreamRedirectRAII(std::ostream& stream) : old_stream_buff(stream.rdbuf()), stream_to_redirect(stream) {
            stream_to_redirect.rdbuf(ss.rdbuf());
        }

        /// Destructor that restores the original stream.
        ~StreamRedirectRAII() {
            stream_to_redirect.rdbuf(old_stream_buff);
        }

        /// Get the output that was written to the stream.
        std::string getCaptured() {
            return ss.str();
        }

    private:
        /// The original buffer of the stream.
        std::streambuf* old_stream_buff;

        /// The stream that is being redirected.
        std::ostream& stream_to_redirect;

        /// The stringstream that the stream is redirected to.
        std::stringstream ss;
};

TEST_SUITE("execute_request")
{
    TEST_CASE("stl")
    {
        std::vector<const char*> Args = {"stl-test-case", "-v"};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());
        std::string code = "#include <vector>";
        nl::json user_expressions = nl::json::object();
        xeus::execute_request_config config;
        config.silent = false;
        config.store_history = false;
        config.allow_stdin = false;
        nl::json header = nl::json::object();
        xeus::xrequest_context::guid_list id = {};
        xeus::xrequest_context context(header, id);

        std::promise<nl::json> promise;
        std::future<nl::json> future = promise.get_future();
        auto callback = [&promise](nl::json result) {
            promise.set_value(result);
        };

        interpreter.execute_request(
            std::move(context),
            std::move(callback),
            code,
            std::move(config),
            user_expressions
        );
        nl::json result = future.get();
        REQUIRE(result["status"] == "ok");
    }

    TEST_CASE("fetch_documentation")
    {
        std::vector<const char*> Args = {/*"-v", "resource-dir", "....."*/};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());

        std::string code = "?std::vector";
        std::string inspect_result = "https://en.cppreference.com/w/cpp/container/vector";
        nl::json user_expressions = nl::json::object();
        xeus::execute_request_config config;
        config.silent = false;
        config.store_history = false;
        config.allow_stdin = false;
        nl::json header = nl::json::object();
        xeus::xrequest_context::guid_list id = {};
        xeus::xrequest_context context(header, id);

        std::promise<nl::json> promise;
        std::future<nl::json> future = promise.get_future();
        auto callback = [&promise](nl::json result) {
            promise.set_value(result);
        };

        interpreter.execute_request(
            std::move(context),
            std::move(callback),
            code,
            std::move(config),
            user_expressions
        );
        nl::json result = future.get();
        REQUIRE(result["payload"][0]["data"]["text/plain"] == inspect_result);
        REQUIRE(result["user_expressions"] == nl::json::object());
        REQUIRE(result["found"] == true);
        REQUIRE(result["status"] == "ok");
    }

    TEST_CASE("fetch_documentation_of_member_or_parameter")
    {
        std::vector<const char*> Args = {/*"-v", "resource-dir", "....."*/};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());

        std::string code = "?std::vector.push_back";
        std::string inspect_result = "https://en.cppreference.com/w/cpp/container/vector/push_back";
        nl::json user_expressions = nl::json::object();
        xeus::execute_request_config config;
        config.silent = false;
        config.store_history = false;
        config.allow_stdin = false;
        nl::json header = nl::json::object();
        xeus::xrequest_context::guid_list id = {};
        xeus::xrequest_context context(header, id);

        std::promise<nl::json> promise;
        std::future<nl::json> future = promise.get_future();
        auto callback = [&promise](nl::json result) {
            promise.set_value(result);
        };

        interpreter.execute_request(
            std::move(context),
            std::move(callback),
            code,
            std::move(config),
            user_expressions
        );
        nl::json result = future.get();
        REQUIRE(result["payload"][0]["data"]["text/plain"] == inspect_result);
        REQUIRE(result["user_expressions"] == nl::json::object());
        REQUIRE(result["found"] == true);
        REQUIRE(result["status"] == "ok");
    }


    TEST_CASE("bad_status")
    {
        std::vector<const char*> Args = {"resource-dir"};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());

        std::string code = "int x = ;";
        nl::json user_expressions = nl::json::object();
        xeus::execute_request_config config;
        config.silent = false;
        config.store_history = false;
        config.allow_stdin = false;
        nl::json header = nl::json::object();
        xeus::xrequest_context::guid_list id = {};
        xeus::xrequest_context context(header, id);

        std::promise<nl::json> promise;
        std::future<nl::json> future = promise.get_future();
        auto callback = [&promise](nl::json result) {
            promise.set_value(result);
        };

        interpreter.execute_request(
            std::move(context),
            std::move(callback),
            code,
            std::move(config),
            user_expressions
        );
        nl::json result = future.get();
        REQUIRE(result["status"] == "error");
    }

    TEST_CASE("C++ stdout/stderr capture (exact match)")
    {
        std::vector<const char*> Args = {};
        xcpp::interpreter interpreter((int)Args.size(), Args.data());

        xeus::execute_request_config config;
        config.silent = false;
        config.store_history = false;
        config.allow_stdin = false;

        nl::json header = nl::json::object();
        xeus::xrequest_context::guid_list id = {};
        xeus::xrequest_context context(header, id);

        std::promise<nl::json> promise;
        auto callback = [&promise](nl::json result) { promise.set_value(result); };

        // Redirect **stderr first**, stdout second
        StreamRedirectRAII cerr_redirect(std::cerr);
        StreamRedirectRAII cout_redirect(std::cout);

        std::string code = R"(
            #include <iostream>
            std::cerr << "CPP_ERR\n";
            std::cout << "CPP_OUT\n";
        )";

        interpreter.execute_request(context, callback, code, config, nl::json::object());
        nl::json result = promise.get_future().get();  // wait for kernel reply

        std::string err = cerr_redirect.getCaptured();
        std::string out = cout_redirect.getCaptured();

        // Interpreter status must be OK
        REQUIRE(result["status"] == "ok");

        // Exact matches
        REQUIRE(err == "CPP_ERR\n");
        REQUIRE(out == "CPP_OUT\n");
    }

}

TEST_SUITE("mime_bundle_repr")
{
    TEST_CASE("int")
    {
        int value = 42;
        nl::json res = xcpp::mime_bundle_repr(value);
        nl::json expected = {
            {"text/plain", "42"}
        };

        REQUIRE(res == expected);
    }
}

// TEST_SUITE("Redirect")
// {

//     TEST_CASE("C and C++ stdout/stderr capture") 
//     {
//         std::vector<const char*> Args = {};
//         xcpp::interpreter interpreter((int)Args.size(), Args.data());
    
//         xeus::execute_request_config config;
//         config.silent = false;
//         config.store_history = false;
//         config.allow_stdin = false;
    
//         nl::json header = nl::json::object();
//         xeus::xrequest_context::guid_list id = {};
//         xeus::xrequest_context context(header, id);
    
//         std::promise<nl::json> promise;
//         auto callback = [&promise](nl::json result) { promise.set_value(result); };
    
//         // Redirect std::cout and std::cerr
//         StreamRedirectRAII cout_redirect(std::cout);
//         StreamRedirectRAII cerr_redirect(std::cerr);
    
//         std::string code = R"(
//             #include <stdio.h>
//             #include <iostream>
//             printf("C stdout\n");
//             fprintf(stderr, "C stderr\n");
//             std::cout << "C++ stdout\n";
//             std::cerr << "C++ stderr\n";
//         )";
    
//         interpreter.execute_request(context, callback, code, config, nl::json::object());
//         (void)promise.get_future().get(); // wait for result
    
//         std::string captured_out = cout_redirect.getCaptured();
//         std::string captured_err = cerr_redirect.getCaptured();
    
//         REQUIRE(captured_out.find("C stdout") != std::string::npos);
//         REQUIRE(captured_out.find("C++ stdout") != std::string::npos);
//         // REQUIRE(captured_err.find("C stderr") != std::string::npos);
//         // REQUIRE(captured_err.find("C++ stderr") != std::string::npos);
//     }

// }
