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

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


// Report a failure
void fail(beast::error_code ec, char const* what);

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
public:

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.

	void handle_request(beast::string_view doc_root, http::request<http::string_body>&& req);

	template<bool isRequest, class Body, class Fields>
	void Send(http::message<isRequest, Body, Fields>&& msg, std::shared_ptr<TileProcessor> requestProcessor)
	{
		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<
			http::message<isRequest, Body, Fields>>(std::move(msg));

		// Store a type-erased version of the shared
		// pointer in the class to keep it alive.
		res_ = sp;

		requestProcessor_ = requestProcessor;

		// Write the response
		http::async_write(
			stream_,
			*sp,
			beast::bind_front_handler(
				&session::on_write,
				shared_from_this(),
				sp->need_eof()));
	}

	// Take ownership of the stream
	session(
		tcp::socket&& socket,
		std::shared_ptr<std::string const> const& doc_root)
		: stream_(std::move(socket))
		, doc_root_(doc_root)
	{
	}

	// Start the asynchronous operation
	void run()
	{
		// We need to be executing within a strand to perform async operations
		// on the I/O objects in this session. Although not strictly necessary
		// for single-threaded contexts, this example code is written to be
		// thread-safe by default.
		net::dispatch(stream_.get_executor(),
			beast::bind_front_handler(
				&session::do_read,
				shared_from_this()));
	}

	void do_read()
	{
		// Make the request empty before reading,
		// otherwise the operation behavior is undefined.
		req_ = {};

		// Set the timeout.
		stream_.expires_after(std::chrono::seconds(30));

		// Read a request
		http::async_read(stream_, buffer_, req_,
			beast::bind_front_handler(
				&session::on_read,
				shared_from_this()));
	}

	void on_read(
			beast::error_code ec,
			std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// This means they closed the connection
		if (ec == http::error::end_of_stream)
			return do_close();

		if (ec)
			return fail(ec, "read");

		// Send the response
		handle_request(*doc_root_, std::move(req_));
	}

	void on_write(
			bool close,
			beast::error_code ec,
			std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
			return fail(ec, "write");

		if (close)
		{
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			return do_close();
		}

		// We're done with the response so delete it
		res_ = nullptr;
		requestProcessor_ = nullptr;

		// Read another request
		do_read();
	}

	void do_close()
	{
		// Send a TCP shutdown
		beast::error_code ec;
		stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}

private:

	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	std::shared_ptr<std::string const> doc_root_;
	http::request<http::string_body> req_;
	std::shared_ptr<void> res_;
	std::shared_ptr<void> requestProcessor_;
};

#endif //HTTPSERVER_SESSION_H_