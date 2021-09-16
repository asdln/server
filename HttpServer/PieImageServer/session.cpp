#include "session.h"
#include "wmts_handler.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

// Report a failure
void fail(beast::error_code ec, char const* what)
{
	//std::cerr << what << ": " << ec.message() << "\n";
}

// Take ownership of the stream
Session::Session(
	tcp::socket&& socket,
	std::shared_ptr<std::string const> const& doc_root)
	: stream_(std::move(socket))
	, doc_root_(doc_root)
{
}

void Session::run()
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

void Session::thread_func()
{
	HandlerMapping* pHandlerMapping = HandlerMapping::GetInstance();
	std::string request_body = req_.body().data();
	auto result = std::make_shared<HandleResult>(req_.version(), req_.keep_alive());

	try
	{
		WMTSHandler handler;
		handler.GetThumbnail(request_body, result);
	}
	catch (std::exception e)
	{
		LOG(ERROR) << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "handle_request error";
	}

	//重置超时
	stream_.expires_after(std::chrono::seconds(10));

	if (result->IsEmpty())
	{
		http::request<http::string_body>& req = req_;
		// Returns a not found response
		auto const not_found =
			[&req](beast::string_view target)
		{
			http::response<http::string_body> res{ http::status::not_found, req.version() };
			res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(http::field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			//res.body() = "The resource '" + std::string(target) + "' was not found.";
			res.set(http::field::access_control_allow_origin, "*");
			res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
			res.set(http::field::access_control_allow_credentials, "true");
			res.prepare_payload();
			return res;
		};

		Send(not_found(""));
	}
	else
	{
		Send(result);
	}
}

void Session::handle_request(
	const Url& url,
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
			res.set(http::field::access_control_allow_origin, "*");
			res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
			res.set(http::field::access_control_allow_credentials, "true");
			res.prepare_payload();
			return res;
		};

		return Send(bad_request("Unknown HTTP-method"));
	}

	HandlerMapping* pHandlerMapping = HandlerMapping::GetInstance();
	std::string request_body = req.body().data();
	Handler* pHandler = pHandlerMapping->GetHandler(url);

	auto result = std::make_shared<HandleResult>(req.version(), req.keep_alive());
	
	try
	{
		bool bRes = pHandler->Handle(url, request_body, result);
	}
	catch (std::exception e)
	{
		LOG(ERROR) << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "handle_request error";
	}

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
			res.set(http::field::access_control_allow_origin, "*");
			res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
			res.set(http::field::access_control_allow_credentials, "true");
			res.prepare_payload();
			return res;
		};

		Send(not_found(""));
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

	Url url(std::string(req_.target()));
	std::string request;

	//如果是获取拇指图的请求，为了减少对并发性能的影响，单独开一个线程进行处理。
	if (url.QueryValue("request", request) && request.compare("GetThumbnail") == 0)
	{
		auto shared_this = shared_from_this();
		std::thread func_thread([shared_this]
			{
				shared_this->thread_func();
			});

		func_thread.detach();
	}
	else
	{
		// Send the response
		handle_request(url, std::move(req_));
	}
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