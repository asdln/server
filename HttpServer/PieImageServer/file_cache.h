#pragma once
#include <string>
#include "buffer.h"

class FileCache
{

public:

	static void SetUseFileCache(bool use_file_cache) { use_file_cache_ = use_file_cache; }

	static bool GetUseFileCache() { return use_file_cache_; }

	static void SetSavePath(const std::string& path);

	static const std::string& GetSavePath() { return path_; }

	static bool Write(const std::string& obj_name, BufferPtr buffer);

	static BufferPtr Read(const std::string& obj_name);

private:

	static bool use_file_cache_;

	static std::string path_;
};

