#include "session.h"
#include "handler.h"

// Report a failure
void
fail(beast::error_code ec, char const* what)
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

	// Returns a not found response
	auto const not_found =
		[&req](beast::string_view target)
	{
		http::response<http::string_body> res{ http::status::not_found, req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "The resource '" + std::string(target) + "' was not found.";
		res.prepare_payload();
		return res;
	};

	// Returns a server error response
	auto const server_error =
		[&req](beast::string_view what)
	{
		http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, "text/html");
		res.keep_alive(req.keep_alive());
		res.body() = "An error occurred: '" + std::string(what) + "'";
		res.prepare_payload();
		return res;
	};

	// Make sure we can handle the method
	if (req.method() != http::verb::get &&
		req.method() != http::verb::head)
		return Send(bad_request("Unknown HTTP-method"), nullptr);

	// Request path must be absolute and not contain "..".
	if (req.target().empty() ||
		req.target()[0] != '/' ||
		req.target().find("..") != beast::string_view::npos)
		return Send(bad_request("Illegal request-target"), nullptr);

	HandlerMapping* pHandlerMapping = HandlerMapping::GetInstance();
	Handler* pHandler = pHandlerMapping->GetHandler(std::string(req.target()));
	pHandler->Handle(*this, doc_root, std::move(req));

	//void* pData = nullptr;
	//unsigned long nDataSize = 0;
	//std::string mimeType = "image/jpeg";

	//std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	//pRequestProcessor->GetData(std::string(req.target()), &pData, nDataSize, mimeType);

	//http::buffer_body::value_type body;
	//body.data = pData;
	//body.size = nDataSize;
	//body.more = false;

	//// Respond to HEAD request
	//if (req.method() == http::verb::head)
	//{
	//	http::response<http::empty_body> res{ http::status::ok, req.version() };
	//	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	//	res.set(http::field::content_type, mimeType/*mime_type(path)*/);
	//	res.content_length(nDataSize);
	//	res.keep_alive(req.keep_alive());
	//	return Send(std::move(res), nullptr);
	//}

	//// Respond to GET request
	//http::response<http::buffer_body> res{
	//	std::piecewise_construct,
	//	std::make_tuple(std::move(body)),
	//	std::make_tuple(http::status::ok, req.version()) };
	//res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	//res.set(http::field::content_type, mimeType/*mime_type(path)*/);

	//res.set(http::field::access_control_allow_origin, "*");
	//res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
	//res.set(http::field::access_control_allow_credentials, "true");

	//res.content_length(nDataSize);
	//res.keep_alive(req.keep_alive());
	//return Send(std::move(res), pRequestProcessor);
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
	requestProcessor_ = nullptr;

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