#include "http_handler.h"
#include "jpg_buffer.h"
#include "style_manager.h"

bool HttpHandler::Handle(boost::beast::string_view doc_root, const Url& url, const std::string& request_body, const std::string& mimeType, std::shared_ptr<HandleResult> result)
{
	std::list<std::string> paths;
	QueryDataPath(url, paths);

	std::vector<std::string> tokens;
	std::string style_str = QueryStyle(url);
	Split(style_str, tokens, ":");

	StylePtr style;
	if (tokens.size() == 3)
	{
		style = StyleManager::GetStyle(tokens[0] + ":" + tokens[1], atoi(tokens[2].c_str()));
	}

	if (style == nullptr)
		style = std::make_shared<Style>();

	int nx = QueryX(url);
	int ny = QueryY(url);
	int nz = QueryZ(url);

	Envelop env;
	GetEnvFromTileIndex(nx, ny, nz, env);

	std::shared_ptr<TileProcessor> pRequestProcessor = std::make_shared<TileProcessor>();
	BufferPtr buffer = pRequestProcessor->GetTileData(paths, env, 256, style.get(), mimeType);

	if (buffer != nullptr)
	{
		result->set_buffer(buffer);

		http::buffer_body::value_type body;
		body.data = result->buffer()->data();
		body.size = result->buffer()->size();
		body.more = false;

		auto msg = std::make_shared<http::response<http::buffer_body>>(
			std::piecewise_construct,
			std::make_tuple(std::move(body)),
			std::make_tuple(http::status::ok, result->version()));

		msg->set(http::field::server, BOOST_BEAST_VERSION_STRING);
		msg->set(http::field::content_type, mimeType/*mime_type(path)*/);

		msg->set(http::field::access_control_allow_origin, "*");
		msg->set(http::field::access_control_allow_methods, "POST, GET, OPTIONS, DELETE");
		msg->set(http::field::access_control_allow_credentials, "true");

		msg->content_length(result->buffer()->size());
		msg->keep_alive(result->keep_alive());

		result->set_buffer_body(msg);
	}

	return true;
}
