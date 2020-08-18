#include "http_handler.h"



void HttpHandler::Handle(Session& ses, boost::beast::string_view doc_root, boost::beast::http::request<boost::beast::http::string_body>&& req)
{
	void* pData = nullptr;
	unsigned long nDataSize = 0;
	std::string mimeType = "image/jpeg";

	std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	pRequestProcessor->GetData(std::string(req.target()), &pData, nDataSize, mimeType);

	http::buffer_body::value_type body;
	body.data = pData;
	body.size = nDataSize;
	body.more = false;

	// Respond to HEAD request
	if (req.method() == http::verb::head)
	{
		http::response<http::empty_body> res{ http::status::ok, req.version() };
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, mimeType/*mime_type(path)*/);
		res.content_length(nDataSize);
		res.keep_alive(req.keep_alive());
		return ses.Send(std::move(res), nullptr);
	}

	// Respond to GET request
	http::response<http::buffer_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req.version()) };
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, mimeType/*mime_type(path)*/);

	res.set(http::field::access_control_allow_origin, "*");
	res.set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
	res.set(http::field::access_control_allow_credentials, "true");

	res.content_length(nDataSize);
	res.keep_alive(req.keep_alive());
	return ses.Send(std::move(res), pRequestProcessor);
}