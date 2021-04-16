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
	Aws::SDKOptions options;
	Aws::InitAPI(options);

    GDALAllRegister();

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

    TaskRecord* task_record = new TaskRecord(aws_region, save_bucket_name
        , aws_secret_access_key, aws_access_key_id, time_limit_sec);

    if (!task_record->Open(path, dataset_count))
        return code_fail;

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

    int code = code_success;
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

    delete task_record;
    task_record = nullptr;

    Aws::ShutdownAPI(options);
    std::cout << "success" << std::endl;

    return code;
}

void make_tile_test()
{
	//TaskRecord* task_record = new TaskRecord("d:/linux_share/GF2_PMS1__L1A0001064454-MSS1.tif", 6);
    //TaskRecord* task_record = new TaskRecord("d:/linux_share/mosaic.tif", 4);
    TaskRecord* task_record = new TaskRecord("", "", "", "");
    //task_record->Open("d:/test/GF1/GF1_PMS1_E109.1_N34.4_20150213_L1A0000650074-MSS1.tiff", 4);
    //task_record->Open("d:/linux_share/world.tif", 4);
    task_record->Open("/vsis3/pie-engine-test/gdal/GF1_Test.tif", 1);
    
    //ProcessLoop(task_record);

 //	std::thread t1(ProcessLoop, task_record);
 // 	std::thread t2(ProcessLoop, task_record);
 // 	std::thread t3(ProcessLoop, task_record);
 // 	std::thread t4(ProcessLoop, task_record);
 // 	std::thread t5(ProcessLoop, task_record);
 // 	std::thread t6(ProcessLoop, task_record);
	//std::thread t7(ProcessLoop, task_record);
	//std::thread t8(ProcessLoop, task_record);
 //
 //	t1.join();
 // 	t2.join();
 // 	t3.join();
 // 	t4.join();
 // 	t5.join();
 // 	t6.join();
	//t7.join();
	//t8.join();
}

int main(int argc, char* argv[])
{
    //make_tile_test();

    return make_tile("/vsis3/pie-engine-test/NN/world.tif", 4, 4, "cn-northwest-1"
        , "uGXq6F4CXnVsRXTU/bLiBFJLjgpD+MPFrTM+z13e", "AKIAT2NCQYSI3X7D52BZ"
        , "s3.cn-northwest-1.amazonaws.com.cn", "pie-engine-test", 780);
}