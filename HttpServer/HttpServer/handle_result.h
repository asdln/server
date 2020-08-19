#ifndef HTTPSERVER_HANDLE_RESULT_H_
#define HTTPSERVER_HANDLE_RESULT_H_

#include <boost/beast/http.hpp>

#include "buffer.h"
#include <memory>


class HandleResult
{
public:

	HandleResult() = default;

	std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> msg();

	void set_msg(std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> msg);

	std::shared_ptr<Buffer> buffer();

	void set_buffer(std::shared_ptr<Buffer> buffer);

	bool IsEmpty();

private:

	std::shared_ptr<Buffer> buffer_;

	std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> msg_;
};

#endif //HTTPSERVER_HANDLE_RESULT_H_