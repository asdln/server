
#include "application.h"
#include <iostream>

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#include <iostream>
#include <boost/pool/pool.hpp>
#include <aws/core/Aws.h>
#include <signal.h>

using namespace boost;

void sig_handler(int sig)
{
	if (sig == SIGINT)
	{
		std::cout << "exit" << std::endl;
		exit(0);
	}
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sig_handler);

	//if (argc != 5)
	//{
	//	std::cerr <<
	//		"Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
	//		"Example:\n" <<
	//		"    http-server-async 0.0.0.0 8080 . 1\n";
	//	return -1;
	//}

	//boost::pool<> pl(sizeof(char) * 1024 * 1024 * 1);             //一个可分配int的内存池          

	//for (int i = 0; i < 100; i ++)
	//{
	//	int* p = (int*)pl.malloc();        //必须把void*转换成需要的类型         

	//	assert(pl.is_from(p));

	//	pl.free(p);
	//}

	Aws::SDKOptions options;
	char* AWS_SECRET_ACCESS_KEY = std::getenv("AWS_SECRET_ACCESS_KEY");
	if (AWS_SECRET_ACCESS_KEY != nullptr)
	{
		Aws::InitAPI(options);
	}

	FLAGS_log_dir = "./log/";
	FLAGS_logbufsecs = 0;
	FLAGS_alsologtostderr = 1;
	google::InitGoogleLogging(argv[0]);
	
	global_app = std::make_shared<Application>(argc, argv);
	global_app->Run();

	google::ShutdownGoogleLogging();

	if (AWS_SECRET_ACCESS_KEY != nullptr)
	{
		Aws::ShutdownAPI(options);
	}

	return 0;
}
