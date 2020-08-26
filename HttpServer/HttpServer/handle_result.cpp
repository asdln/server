#include "handle_result.h"

HandleResult::HandleResult(unsigned int version, bool keep_alive)
{
	version_ = version;
	keep_alive_ = keep_alive;
}

std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> HandleResult::buffer_body()
{
	return buffer_body_;
}

void HandleResult::set_buffer_body(std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> buffer_body)
{
	buffer_body_ = buffer_body;
}

std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> HandleResult::string_body()
{
	return string_body_;
}

void HandleResult::set_string_body(std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> string_body)
{
	string_body_ = string_body;
}

std::shared_ptr<Buffer> HandleResult::buffer()
{
	return buffer_;
}

void HandleResult::set_buffer(std::shared_ptr<Buffer> buffer)
{
	buffer_ = buffer;
}

bool HandleResult::IsEmpty()
{
	return buffer_ == nullptr && string_body_ == nullptr ? true : false;
}