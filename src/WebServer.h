#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using webcallback = std::function<void(http::request<http::dynamic_body> &, http::response<http::dynamic_body> &)>;

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
  http_connection(tcp::socket socket, webcallback &callback)
      : socket_(std::move(socket)), m_callback(callback)
  {
  }

  // Initiate the asynchronous operations associated with the connection.
  void start()
  {
    read_request();
    check_deadline();
  }

private:
  // Weather service license
  std::string m_token;

  // The socket for the currently connected client.
  tcp::socket socket_;

  // The buffer for performing reads.
  beast::flat_buffer buffer_{8192};

  // The request message.
  http::request<http::dynamic_body> request_;

  // The response message.
  http::response<http::dynamic_body> response_;

  // The timer for putting a deadline on connection processing.
  net::basic_waitable_timer<std::chrono::steady_clock> deadline_{socket_.get_executor().context(), std::chrono::seconds(60)};

  // The request/response callback.
  webcallback &m_callback;

  // Asynchronously receive a complete request message.
  void read_request()
  {
    auto self = shared_from_this();

    http::async_read(
        socket_,
        buffer_,
        request_,
        [self](beast::error_code ec,
               std::size_t bytes_transferred) {
          boost::ignore_unused(bytes_transferred);
          if (!ec)
            self->process_request();
        });
  }

  // Determine what needs to be done with the request message.
  void process_request()
  {
    auto reqMethod = request_.method();

    if (reqMethod == boost::beast::http::verb::get || reqMethod == boost::beast::http::verb::post)
    {
      response_.version(request_.version());
      response_.keep_alive(false);

      response_.result(http::status::ok);
      response_.set(http::field::server, "Beast");
      create_response();
    }
    else
    {
      // We return responses indicating an error if
      // we do not recognize the request method.
      response_.result(http::status::bad_request);
      response_.set(http::field::content_type, "text/plain");
      beast::ostream(response_.body())
          << "Unhandled request-method '"
          << request_.method_string().to_string()
          << "'";
    }

    write_response();
  }

  // Construct a response message based on the program state.
  void create_response()
  {
    m_callback(this->request_, this->response_);
  }

  // Asynchronously transmit the response message.
  void write_response()
  {
    auto self = shared_from_this();

    response_.set(http::field::content_length, response_.body().size());

    http::async_write(
        socket_,
        response_,
        [self](beast::error_code ec, std::size_t) {
          self->socket_.shutdown(tcp::socket::shutdown_send, ec);
          self->deadline_.cancel();
        });
  }

  // Check whether we have spent enough time on this connection.
  void check_deadline()
  {
    auto self = shared_from_this();

    deadline_.async_wait([self](beast::error_code ec) {
      if (!ec)
      {
        // Close socket to cancel any outstanding operation.
        self->socket_.close(ec);
      }
    });
  }
};

class WebServer
{
  std::string m_srvAddr;
  int m_srvPort;
  webcallback m_callback;

  void http_server(tcp::acceptor &acceptor, tcp::socket &socket)
  {
    acceptor.async_accept(socket, [&](beast::error_code ec) {
      if (!ec)
        std::make_shared<http_connection>(std::move(socket), m_callback)->start();

      http_server(acceptor, socket);
    });
  }

public:
  WebServer(const char *wsAddr, int wsPort = 80) : m_srvAddr(wsAddr), m_srvPort(wsPort)
  {
  }

  void Run(webcallback callback)
  {
    m_callback = std::move(callback);

    try
    {
      auto const address = net::ip::make_address(m_srvAddr.c_str());
      unsigned short port = static_cast<unsigned short>(m_srvPort);

      net::io_context ioc{1};

      tcp::acceptor acceptor{ioc, {address, port}};
      tcp::socket socket{ioc};
      http_server(acceptor, socket);

      ioc.run();
    }
    catch (std::exception const &e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }
};
