#include "handle_result.h"


std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> HandleResult::msg()
{
	return msg_;
}

void HandleResult::set_msg(std::shared_ptr<boost::beast::http::response<boost::beast::http::buffer_body>> msg)
{
	msg_ = msg;
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
	return buffer_ == nullptr ? true : false;
}