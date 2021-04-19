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

int make_tile(const std::string& path, int dataset_count, int thread_count
    , const std::string& aws_region, const std::string& aws_secret_access_key
    , const std::string& aws_access_key_id, const std::string& aws_s3_endpoint
    , const std::string save_bucket_name, int time_limit_sec)
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
        , aws_secret_access_key, aws_access_key_id, time_limit_sec);

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
    if (0)
    {
		Aws::SDKOptions options;
		Aws::InitAPI(options);

		AWSS3DeleteObject("NN/mosaic.tif.pyra/info.json", "pie-engine-test", "cn-northwest-1"
	      , "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ");

		Aws::ShutdownAPI(options);

		return make_tile("/vsis3/pie-engine-test/NN/mosaic.tif", 4, 4, "cn-northwest-1"
			, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
			, "s3.cn-northwest-1.amazonaws.com.cn", "pie-engine-test", 780);
    }

	if (1)
	{
		return make_tile("d:/linux_share/DEM-Gloable32.tif", 4, 4, "cn-northwest-1"
			, "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
			, "s3.cn-northwest-1.amazonaws.com.cn", "pie-engine-test", 15);
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