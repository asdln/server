#include "file_cache.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "png_buffer.h"

bool FileCache::use_file_cache_ = false;

std::string FileCache::path_;

void FileCache::SetSavePath(const std::string& path)
{
	path_ = path;

	if (path_[path.size() - 1] == '/' || path_[path.size() - 1] == '\\')
	{
		path_ = path_.substr(0, path_.size() - 1);
	}

	if (!std::filesystem::exists(path_))
	{
		if (!std::filesystem::create_directories(path_))
		{
			std::cout << "create path failed: " << path << std::endl;
		}
	}
}

bool FileCache::Write(const std::string& obj_name, BufferPtr buffer)
{
	std::ofstream outFile(path_ + '/' + obj_name, std::ios::binary | std::ios::out);

	if (!outFile.is_open())
	{
		//失败可能是因为没有创建目录，创建目录再试
		std::string group_path = path_ + '/' + obj_name;
		int pos = group_path.rfind("/");
		group_path = group_path.substr(0, pos);

		if (!std::filesystem::exists(group_path))
		{
			if (!std::filesystem::create_directories(group_path))
			{
				std::cout << "create path failed: " << group_path << std::endl;
			}
		}

		if (!outFile.is_open())
		{
			std::cout << "ln_debug: file cache open failed" << std::endl;
			return false;
		}
	}

	outFile.write((char*)buffer->data(), buffer->size());

	if (!outFile.good())
	{
		std::cout << "ln_debug: file cache write failed" << std::endl;
		return false;
	}

	outFile.close();

	return true;
}

BufferPtr FileCache::Read(const std::string& obj_name)
{
	std::ifstream inFile2(path_ + '/' + obj_name, std::ios::binary | std::ios::in);
	if (!inFile2.is_open())
		return nullptr;

	inFile2.seekg(0, std::ios_base::end);
	int FileSize2 = inFile2.tellg();

	inFile2.seekg(0, std::ios::beg);

	auto png_buffer = std::make_shared<PngBuffer>();
	std::vector<unsigned char>& buffer = png_buffer->GetVec();

	//char* buffer2 = new char[FileSize2];
	buffer.resize(FileSize2);
	inFile2.read((char*)buffer.data(), FileSize2);
	inFile2.close();

	return png_buffer;
}