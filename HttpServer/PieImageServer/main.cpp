//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#include <iostream>
#include "application.h"

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		std::cerr <<
			"Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
			"Example:\n" <<
			"    http-server-async 0.0.0.0 8080 . 1\n";
		return -1;
	}

	global_app = std::make_shared<Application>(argc, argv);
	global_app->Run();

	return 0;
}
