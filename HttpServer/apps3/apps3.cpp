// apps3.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "gdal_priv.h"
#include <string>
#include <unordered_map>
#include <algorithm>

#include <iostream>
#include "resource_pool.h"
#include "task_record.h"
#include <thread>
#include <fstream>

using namespace std;

//  "/vsis3/pie-engine-test/NN/DEM-Gloable32.tif"
// save_bucket_name = "pie-engine-test/NN"

int make_tile(const std::string& path, int dataset_count, int thread_count
    , const std::string& aws_region, const std::string& aws_secret_access_key
    , const std::string& aws_access_key_id, const std::string& aws_s3_endpoint
    , const std::string save_bucket_name, int time_limit_sec, int force)
{

#ifndef USE_FILE
	Aws::SDKOptions options;
	Aws::InitAPI(options);

	if (path.rfind("/vsis3/", 0) == 0) 
    {
		CPLSetConfigOption("AWS_REGION", aws_region.c_str());
		CPLSetConfigOption("AWS_SECRET_ACCESS_KEY", aws_secret_access_key.c_str());
		CPLSetConfigOption("AWS_ACCESS_KEY_ID", aws_access_key_id.c_str());
		CPLSetConfigOption("AWS_S3_ENDPOINT", aws_s3_endpoint.c_str());
	}
    else
    {
        return code_fail;
    }

#endif // !USE_FILE

	GDALAllRegister();

    TaskRecord* task_record = new TaskRecord(aws_region, save_bucket_name
        , aws_secret_access_key, aws_access_key_id, time_limit_sec, force);

    if (!task_record->Open(path, dataset_count))
        return code_fail;

	int code = code_success;
    if (!task_record->IsReady())
    {
		std::vector<int> process_status;
		process_status.resize(thread_count, code_fail);

		std::vector<std::thread> threads;
		for (int i = 0; i < thread_count; i++)
		{
			threads.emplace_back(ProcessLoop, task_record, std::ref(process_status[i]));
		}

		for (auto& thread : threads)
		{
			thread.join();
		}

		for (const auto& status : process_status)
		{
			if (status == code_fail)
			{
				code = code_fail;
				std::cout << "fail" << std::endl;
				break;
			}
			else if (status == code_not_finished)
			{
				code = code_not_finished;
				std::cout << "not finished" << std::endl;
				break;
			}
		}
    }
	else
	{
		std::cout << "already finished before" << std::endl;
	}

    delete task_record;
    task_record = nullptr;

#ifndef USE_FILE
    Aws::ShutdownAPI(options);
#endif

    std::cout << "exit code " << code << std::endl;

    return code;
}

int main(int argc, char* argv[])
{
    if (1)
    {
		if (argc != 11)
		{
			std::cout << "error: argc is not 11" << std::endl;
			return code_fail;
		}

		char* arg1 = argv[1];
		std::string path(arg1, strlen(arg1));

		char* arg2 = argv[2];
		int dataset_count = std::atoi(arg2);

		char* arg3 = argv[3];
		int thread_count = std::atoi(arg3);

		char* arg4 = argv[4];
		std::string aws_region(arg4, strlen(arg4));

		char* arg5 = argv[5];
		std::string aws_secret_access_key(arg5, strlen(arg5));

		char* arg6 = argv[6];
		std::string aws_access_key_id(arg6, strlen(arg6));

		char* arg7 = argv[7];
		std::string aws_s3_endpoint(arg7, strlen(arg7));

		char* arg8 = argv[8];
		std::string save_bucket_name(arg8, strlen(arg8));

		char* arg9 = argv[9];
		int time_limit_sec = std::atoi(arg9);

		char* arg10 = argv[10];
		int force = std::atoi(arg10);

		//if (force != 0)
		//{
		//	Aws::SDKOptions options;
		//	Aws::InitAPI(options);

		//	AWSS3DeleteObject("NN/mosaic.tif.pyra/info.json", "pie-engine-test", "cn-northwest-1"
		//		, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ");

		//	Aws::ShutdownAPI(options);
		//}

		//return make_tile("/vsis3/pie-engine-test/NN/mosaic.tif", 4, 4, "cn-northwest-1"
		//	, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
		//	, "s3.cn-northwest-1.amazonaws.com.cn", "pie-engine-test", 780, 0);

		return make_tile(path, dataset_count, thread_count, aws_region
			, aws_secret_access_key, aws_access_key_id
			, aws_s3_endpoint, save_bucket_name, time_limit_sec, force);
    }

	if (0)
	{
		return make_tile("/vsis3/pie-engine-test/NN/mosaic.tif", 4, 4, "cn-northwest-1"
			, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
			, "s3.cn-northwest-1.amazonaws.com.cn", "/pie-engine-test/NN/YY/TT/", 15, 0);
	}
    
	if(0)
    {
		Aws::SDKOptions options;
		Aws::InitAPI(options);

		AWSS3PutObject_File("pie-engine-test", "NN/DEM-Gloable32.tif", "cn-northwest-1"
			, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
			, "/data/ln/mosaic.tif");

		//AWSS3DeleteObject("NN/mosaci.tif", "pie-engine-test", "cn-northwest-1"
		//      , "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ");

		Aws::ShutdownAPI(options);
    }
}