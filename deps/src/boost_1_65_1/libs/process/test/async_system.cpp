// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MAIN
#define BOOST_TEST_IGNORE_SIGCHLD
#include <boost/test/included/unit_test.hpp>

#include <boost/process/error.hpp>
#include <boost/process/io.hpp>
#include <boost/process/async.hpp>
#include <boost/process/child.hpp>
#include <boost/process/async_system.hpp>

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/yield.hpp>

#include <vector>
#include <array>

namespace bp = boost::process;
BOOST_AUTO_TEST_CASE(stackful, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    bool did_something_else = false;

    boost::asio::io_service ios;
    auto stackful =
            [&](boost::asio::yield_context yield_)
            {
                int ret =
                    bp::async_system(
                        ios, yield_,
                        master_test_suite().argv[1],
                        "test", "--exit-code", "123");

                BOOST_CHECK_EQUAL(ret, 123);
                BOOST_CHECK(did_something_else);
            };

    boost::asio::spawn(ios, stackful);
    ios.post([&]{did_something_else = true;});

    ios.run();
    BOOST_CHECK(did_something_else);
}

BOOST_AUTO_TEST_CASE(stackful_except, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    bool did_something_else = false;

    boost::asio::io_service ios;
    auto stackful =
            [&](boost::asio::yield_context yield_)
            {

                BOOST_CHECK_THROW(
                    bp::async_system(
                        ios, yield_,
                        "none-existing-exe"), boost::system::system_error);

                BOOST_CHECK(did_something_else);
            };

    boost::asio::spawn(ios, stackful);
    ios.post([&]{did_something_else = true;});
    ios.run();

    BOOST_CHECK(did_something_else);
}


BOOST_AUTO_TEST_CASE(stackful_error, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    bool did_something_else = false;

    boost::asio::io_service ios;
    auto stackful =
            [&](boost::asio::yield_context yield_)
            {
                boost::system::error_code ec;
                auto ret =
                    bp::async_system(
                        ios, yield_[ec],
                        "none-existing-exe");

                BOOST_CHECK(ec);
                BOOST_CHECK_EQUAL(ret, -1);
                BOOST_CHECK(did_something_else);
            };

    boost::asio::spawn(ios, stackful);
    ios.post([&]{did_something_else = true;});
    ios.run();

    BOOST_CHECK(did_something_else);
}


BOOST_AUTO_TEST_CASE(stackless, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    boost::asio::io_service ios;

    bool did_something_else = false;

    struct stackless_t : boost::asio::coroutine
    {
        boost::asio::io_service & ios;
        bool & did_something_else;

        stackless_t(boost::asio::io_service & ios_,
                    bool & did_something_else)
                        : ios(ios_), did_something_else(did_something_else) {}
        void operator()(
                boost::system::error_code ec = boost::system::error_code(),
                std::size_t exit_code = 0)
        {
            if (!ec) reenter (this)
            {
                yield bp::async_system(
                        ios, *this,
                        master_test_suite().argv[1],
                        "test", "--exit-code", "42");

                BOOST_CHECK_EQUAL(exit_code, 42);
                BOOST_CHECK(did_something_else);
            }
        }
    } stackless{ios, did_something_else};

    ios.post([&]{stackless();});
    ios.post([&]{did_something_else = true;});

    ios.run();

    BOOST_CHECK(did_something_else);
}


BOOST_AUTO_TEST_CASE(future, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    boost::asio::io_service ios;

    std::future<int> fut = bp::async_system(
                              ios, boost::asio::use_future,
                              master_test_suite().argv[1],
                              "test", "--exit-code", "42");

    ios.run();

    int exit_code = 0;
    BOOST_CHECK_NO_THROW(exit_code = fut.get());

    BOOST_CHECK_EQUAL(exit_code, 42);
}

BOOST_AUTO_TEST_CASE(future_error, *boost::unit_test::timeout(15))
{
    using boost::unit_test::framework::master_test_suite;

    boost::asio::io_service ios;

    std::future<int> fut = bp::async_system(
                              ios, boost::asio::use_future,
                              "invalid-command");

    ios.run();

    int exit_code = 0;
    BOOST_CHECK_THROW(exit_code = fut.get(), boost::system::system_error);
}
