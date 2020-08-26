#ifndef HTTPSERVER_SESSION_H_
#define HTTPSERVER_SESSION_H_

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tile_processor.h"
#include "handler_mapping.h"
#include "handle_result.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// Report a failure
void fail(beast::error_code ec, char const* what);

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
public:

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.

	void handle_request(beast::string_view doc_root, http::request<http::string_body>&& req);

	template<bool isRequest, class Body, class Fields>
	void Send(http::message<isRequest, Body, Fields>&& msg)
	{
		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<
			http::message<isRequest, Body, Fields>>(std::move(msg));

		// Store a type-erased version of the shared
		// pointer in the class to keep it alive.
		res_ = sp;

		// Write the response
		http::async_write(
			stream_,
			*sp,
			beast::bind_front_handler(
				&Session::on_write,
				shared_from_this(),
				sp->need_eof()));
	}

	void Send(std::shared_ptr<HandleResult> result)
	{
		//保存一下智能指针对象。在session发送完毕，释放的时候才析构result
		result_ = result;

		if (result->buffer_body() != nullptr)
		{
			// Write the response
			http::async_write(
				stream_,
				*(result->buffer_body()),
				beast::bind_front_handler(
					&Session::on_write,
					shared_from_this(),
					result->buffer_body()->need_eof()));
		}
		else if (result->string_body() != nullptr)
		{
			// Write the response
			http::async_write(
				stream_,
				*(result->string_body()),
				beast::bind_front_handler(
					&Session::on_write,
					shared_from_this(),
					result->string_body()->need_eof()));
		}

	}

	// Take ownership of the stream
	Session(tcp::socket&& socket, std::shared_ptr<std::string const> const& doc_root);

	~Session() {}

	// Start the asynchronous operation
	void run()
	{
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(stream_.get_executor(),
			beast::bind_front_handler(
				&Session::do_read,
				shared_from_this()));
	}

	void do_read();

	void on_read(beast::error_code ec, std::size_t bytes_transferred);

	void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred);

	void do_close();

private:

	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	std::shared_ptr<std::string const> doc_root_;
	http::request<http::string_body> req_;
	std::shared_ptr<void> res_;
	std::shared_ptr<HandleResult> result_;
};

#endif //HTTPSERVER_SESSION_H_