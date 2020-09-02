#ifndef HTTPSERVER_HANDLE_RESULT_H_
#define HTTPSERVER_HANDLE_RESULT_H_

#include <boost/beast/http.hpp>

#include "buffer.h"
#include <memory>


class HandleResult
{
public:

	HandleResult() = default;

	HandleResult(unsigned int version, bool keep_alive);

	std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> buffer_body();

	void set_buffer_body(std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> buffer_body);

	std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> string_body();

	void set_string_body(std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> string_body);

	std::shared_ptr<Buffer> buffer();

	void set_buffer(std::shared_ptr<Buffer> buffer);

	unsigned int version() { return version_; }

	void set_version(unsigned int version) { version_ = version; }

	bool keep_alive() { return keep_alive_; }

	void set_keep_alive(bool keep_alive) { keep_alive_ = keep_alive; }

	bool IsEmpty();

private:

	std::shared_ptr<Buffer> buffer_;

	std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> buffer_body_;

	std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> string_body_;

	unsigned int version_;

	bool keep_alive_;
};

#endif //HTTPSERVER_HANDLE_RESULT_H_