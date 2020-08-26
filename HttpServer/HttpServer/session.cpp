#include "session.h"
#include "handler.h"

// Report a failure
void fail(beast::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

// Take ownership of the stream
Session::Session(
	tcp::socket&& socket,
	std::shared_ptr<std::string const> const& doc_root)
	: stream_(std::move(socket))
	, doc_root_(doc_root)
{
}

void Session::handle_request(
	beast::string_view doc_root,
	http::request<http::string_body>&& req)
{
	// Make sure we can handle the method
	if (req.method() != http::verb::get &&
		req.method() != http::verb::post || 
		req.target().empty() ||
		req.target()[0] != '/' ||
		req.target().find("..") != beast::string_view::npos)
	{
		// Returns a bad request response
		auto const bad_request =
			[&req](beast::string_view why)
		{
			http::response<http::string_body> res{ http::status::bad_request, req.version() };
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = std::string(why);
			res.prepare_payload();
			return res;
		};

		return Send(bad_request("Unknown HTTP-method"));
	}

	std::string mimeType = "image/jpeg";
	HandlerMapping* pHandlerMapping = HandlerMapping::GetInstance();
	Url url(std::string(req.target()));
	
	std::string request_body = req.body().data();
	Handler* pHandler = pHandlerMapping->GetHandler(url);

	auto result = std::make_shared<HandleResult>(req.version(), req.keep_alive());
	bool bRes = pHandler->Handle(doc_root, url, request_body, mimeType, result);

	if (result->IsEmpty())
	{
		// Returns a not found response
		auto const not_found =
			[&req](beast::string_view target)
		{
			http::response<http::string_body> res{ http::status::not_found, req.version() };
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			//res.body() = "The resource '" + std::string(target) + "' was not found.";
			res.prepare_payload();
			return res;
		};

		Send(not_found(mimeType));
	}
	else
	{
		Send(result);
	}
}

void Session::do_read()
{
	// Make the request empty before reading,
	// otherwise the operation behavior is undefined.
	req_ = {};

	// Set the timeout.
	stream_.expires_after(std::chrono::seconds(30));

	// Read a request
	http::async_read(stream_, buffer_, req_,
		beast::bind_front_handler(
			&Session::on_read,
			shared_from_this()));
}

void Session::on_read(
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

void Session::on_write(
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
	result_ = nullptr;

	// Read another request
	do_read();
}

void Session::do_close()
{
	// Send a TCP shutdown
	beast::error_code ec;
	stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

	// At this point the connection is closed gracefully
}