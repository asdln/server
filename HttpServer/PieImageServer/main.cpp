
#include "application.h"

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#include <iostream>

int main(int argc, char* argv[])
{
	//if (argc != 5)
	//{
	//	std::cerr <<
	//		"Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
	//		"Example:\n" <<
	//		"    http-server-async 0.0.0.0 8080 . 1\n";
	//	return -1;
	//}

	FLAGS_log_dir = "./";
	FLAGS_logbufsecs = 0;
	google::InitGoogleLogging(argv[0]);
	
	global_app = std::make_shared<Application>(argc, argv);
	global_app->Run();

	LOG(INFO) << "PIEImageServer started";  //输出一个Info日志

	google::ShutdownGoogleLogging();
	return 0;
}
