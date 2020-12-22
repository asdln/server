#ifndef PIEIMAGESERVER_IMAGE_GROUP_MANAGER_H_
#define PIEIMAGESERVER_IMAGE_GROUP_MANAGER_H_

#include <string>
#include <set>
#include <unordered_map>
#include <shared_mutex>

class ImageGroupManager
{
public:

	static bool AddImages(const std::string& request_body);

	static bool GetImages(const std::string& request_body, std::string& image_paths_json);

	static bool ClearImages(const std::string& request_body);

protected:

	static bool AddImages(const std::string& user, const std::string& group, const std::set<std::string>& image_paths);

	static bool GetImages(const std::string& user, const std::string& group, std::string& images_json);

	static bool ClearImages(const std::string& user, const std::string& group);

protected:

	static std::shared_mutex s_mutex_;

	static std::unordered_map<std::string, std::set<std::string>> s_user_group_image_map_;
};

#endif //PIEIMAGESERVER_IMAGE_GROUP_MANAGER_H_