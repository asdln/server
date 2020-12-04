
#include "application.h"
#include <iostream>

#define GOOGLE_GLOG_DLL_DECL 
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"

#include <iostream>
#include <boost/pool/pool.hpp>     

using namespace boost;

int main(int argc, char* argv[])
{
    std::cout << "server starting" <<std::endl;

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

	FLAGS_log_dir = "./";
	FLAGS_logbufsecs = 0;
	FLAGS_alsologtostderr = 1;
	google::InitGoogleLogging(argv[0]);
	
	global_app = std::make_shared<Application>(argc, argv);
	global_app->Run();

	google::ShutdownGoogleLogging();
	return 0;
}
