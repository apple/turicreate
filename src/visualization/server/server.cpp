/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

// Server impl code adapted from Beast example in
// boost/beast/example/http/server/async/http_server_async.cpp

#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/globals/globals.hpp>
#include <core/logging/logger.hpp>
#include <visualization/server/server.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/fusion/include/std_pair.hpp>

#include <memory>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace qi = boost::spirit::qi;
typedef std::map<std::string, std::string> pairs_type;
using namespace turi::visualization;

// Report a failure
void fail(boost::system::error_code ec, char const* what) {
    logstream(LOG_ERROR) << "Web server error: " << what << ": " << ec.message() << std::endl;
}

// Parse query string from URL
// URL parsing code adapted from Spirit example in
// boost/spirit/example/qi/key_value_sequence.cpp
template <typename Iterator>
struct url_query_string 
    : qi::grammar<Iterator, pairs_type()> {
    url_query_string()
        : url_query_string::base_type(query)
    {
        query =  pair >> *((qi::lit('&') | '&') >> pair);
        pair  =  key >> -('=' >> value);
        key   =  qi::char_("a-zA-Z_\\-") >> *qi::char_("a-zA-Z_0-9\\-");
        value = +qi::char_("a-zA-Z_0-9\\-");
    }

    qi::rule<Iterator, pairs_type()> query;
    qi::rule<Iterator, std::pair<std::string, std::string>()> pair;
    qi::rule<Iterator, std::string()> key, value;
};

// MIME type from file extension
// Adapted from Beast example in
// beast/example/http/server/fast/http_server_fast.cpp
boost::beast::string_view mime_type(boost::beast::string_view path) {
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Request handler function
template<
    class Body, class Allocator,
    class Send>
void handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send,
    const WebServer::plot_map& plots) {
    // Returns a bad request response
    auto const bad_request =
    [&req](boost::beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](boost::beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req, &send](boost::beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return send(std::move(res));
    };

    // Make sure we can handle the method= 
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    auto const respond =
    [&req, &send, &server_error](const std::string& body, const std::string& content_type) {
        if(req.method() == http::verb::head) {
            http::response<http::empty_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, content_type);
            res.content_length(body.size());
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        } else if(req.method() == http::verb::get) {
            http::response<http::string_body> res{
                std::piecewise_construct,
                std::make_tuple(body),
                std::make_tuple(http::status::ok, req.version())};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, content_type);
            res.content_length(body.size());
            res.keep_alive(req.keep_alive());
            return send(std::move(res));
        } else {
            return server_error("Unexpected request method. Expected either HEAD or GET.");
        }
    };


    try {
        auto req_target = req.target();
        if (req_target.find("/spec/") == 0) {
            const size_t spec_url_length = sizeof("/spec/") - 1;
            std::string plot_id = req_target.substr(spec_url_length).to_string();
            if (plots.find(plot_id) == plots.end()) {
                return server_error("Expected plot " + plot_id + " was not found");
            }
            const Plot& plot = plots.at(plot_id);

            // For now, force light mode, until we have dark mode
            // support for all visualizations and the web app itself.
            std::string plot_spec = plot.get_spec(tc_plot_color_light);

            std::stringstream ss;
            ss << "{\"type\":\"vega\",\"data\":";
            ss << plot_spec;
            ss << "}";
            return respond(ss.str(), "application/json");
        }
        if (req_target.find("/data/") == 0) {
            const size_t data_url_length = sizeof("/data/") - 1;
            std::string plot_id = req_target.substr(data_url_length).to_string();
            if (plots.find(plot_id) == plots.end()) {
                return server_error("Expected plot " + plot_id + " was not found");
            }
            const Plot& plot = plots.at(plot_id);
            std::string plot_data = plot.get_next_data();
            return respond(plot_data, "application/json");
        }

        // try to match a static file
        // note: server-side API caller should set static URL directory prior to any HTTP requests!
        DASSERT_NE(VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY, "");
        std::string possible_file_path = VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY + req_target.to_string();
        size_t query_pos = possible_file_path.rfind("?");
        if (query_pos != std::string::npos) {
            possible_file_path = possible_file_path.substr(0, query_pos);
        }
        turi::fileio::file_status possible_file_status = turi::fileio::get_file_status(possible_file_path);
        if (possible_file_status == turi::fileio::file_status::REGULAR_FILE) {
            // we can serve a file from this path
            turi::general_ifstream stream(possible_file_path);
            std::string file_contents{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
            std::string type = mime_type(possible_file_path).to_string();
            return respond(file_contents, type);
        }

        // did not match any expected URL
        logstream(LOG_ERROR) << "WebServer: unrecognized destination requested in URL:  " << req_target << std::endl;
        return send(not_found(req_target));

    } catch (const std::exception& e) {
        return server_error(e.what());
    } catch (const std::string& s) {
        return server_error(s);
    } catch (...) {
        return server_error("Unknown exception");
    }
};


// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
private:
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda {
        session& self_;

        explicit
        send_lambda(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            http::async_write(
                self_.socket_,
                *sp,
                boost::asio::bind_executor(
                    self_.strand_,
                    std::bind(
                        &session::on_write,
                        self_.shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2,
                        sp->need_eof())));
        }
    };

    tcp::socket socket_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;
    const WebServer::plot_map& m_plots; // reference to the uuid->plot dictionary

public:
    // Take ownership of the socket
    explicit session(
        tcp::socket socket,
        const WebServer::plot_map& plots)
        : socket_(std::move(socket))
        , strand_(socket_.get_executor())
        , lambda_(*this)
        , m_plots(plots) { }

    // Start the asynchronous operation
    void run() {
        do_read();
    }

    void do_read() {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Read a request
        http::async_read(socket_, buffer_, req_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // Send the response
        handle_request(std::move(req_), lambda_, m_plots);
    }

    void on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred,
        bool close) {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // We're done with the response so delete it
        res_ = nullptr;

        // Read another request
        do_read();
    }

    void do_close() {
        // Send a TCP shutdown
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    const WebServer::plot_map& m_plots; // reference to the uuid->plot dictionary

public:
    listener(
        boost::asio::io_context& ioc,
        tcp::endpoint endpoint,
        const WebServer::plot_map& plots)
        : acceptor_(ioc)
        , socket_(ioc)
        , m_plots(plots) {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run() {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }

    void do_accept() {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &listener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }

    void on_accept(boost::system::error_code ec) {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
                std::move(socket_),
                m_plots)->run();
        }

        // Accept another connection
        do_accept();
    }
};

static unsigned short find_port() {
    for (unsigned short port=8000; port<=9000; port++) {
        logstream(LOG_DEBUG) << "WebServer: checking port " << port << std::endl;
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            if (errno == EMFILE) {
                log_and_throw("Unable to open a port between 8000 and 9000 (inclusive) to host Turi Create visualizations: too many open file handles.");
            }
            log_and_throw("Unable to open a port between 8000 and 9000 (inclusive) to host Turi Create visualizations: unknown error.");
        }
        int optval = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(inet_addr("0.0.0.0"));
        int error = bind(sock, (struct sockaddr*) &addr, sizeof(addr));
        if (error == 0) {
            return port;
        } else {
            logstream(LOG_DEBUG) << "WebServer: return was " << error << ", error was " << errno << " (" << std::strerror(errno) << ")" << std::endl;
        }

    }
    log_and_throw("Unable to open a port between 8000 and 9000 (inclusive) to host Turi Create visualizations: all ports seem to be in use.");
}

class WebServer::Impl {
public:
    unsigned short m_port;
    const static size_t NUM_THREADS = 6; // handle requests on 6 threads - TODO optimize this number
    boost::asio::io_context m_ioc{NUM_THREADS}; 
    std::vector<std::thread> m_threads; // listener threads
    std::shared_ptr<listener> m_listener = nullptr;

    Impl(const plot_map& plots) {
        logstream(LOG_DEBUG) << "WebServer: starting WebServer::Impl\n";
        m_port = find_port();
        const auto address = boost::asio::ip::make_address("127.0.0.1");

        // Create and launch a listening port
        m_listener = std::make_shared<listener>(
            m_ioc,
            tcp::endpoint{address, m_port},
            plots);
        m_listener->run();

        // Run the ioc on background threads
        for (size_t i=0; i<NUM_THREADS; i++) {
            m_threads.push_back(std::thread([&]() {
                m_ioc.run();
            }));
        }

        logstream(LOG_DEBUG) << "WebServer: finished starting WebServer::Impl\n";
    }
    ~Impl() {
        logstream(LOG_DEBUG) << "WebServer: destroying WebServer::Impl\n";
        m_ioc.stop();
        for (auto& thread : m_threads) {
            thread.join();
        }
        logstream(LOG_DEBUG) << "WebServer: finished destroying WebServer::Impl\n";
    }
};

WebServer::WebServer() : m_impl(new Impl(m_plots)) {
    logstream(LOG_DEBUG) << "WebServer: starting WebServer\n";
}
WebServer::~WebServer() {
    logstream(LOG_DEBUG) << "WebServer: destroying WebServer\n";
}

WebServer& WebServer::get_instance() {
    static std::unique_ptr<WebServer> server = nullptr;
    if (server == nullptr) {
        server = std::unique_ptr<WebServer>(new WebServer());
    }
    return *server;
}

std::string WebServer::get_url_for_plot(const Plot& plot) {
    return get_instance().add_plot(plot);
}

std::string WebServer::add_plot(const Plot& plot) {

    // add to dictionary with UUID
    const std::string& uuid_str = plot.get_id();
    m_plots[uuid_str] = plot;

    // return formatted URL
    std::string port_str = std::to_string(m_impl->m_port);
    return "http://localhost:" + port_str + "/index.html?" + uuid_str;
}

namespace turi {
    namespace visualization {
        // The root directory for static files for the visualization web server
        EXPORT std::string VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY;
        REGISTER_GLOBAL(std::string, VISUALIZATION_WEB_SERVER_ROOT_DIRECTORY, true);
    }
}
