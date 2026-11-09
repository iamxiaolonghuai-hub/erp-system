#include "FileController.h"

#include "../utils/ApiResponse.h"
#include <drogon/drogon.h>
#include <drogon/MultiPart.h>

#include <chrono>
#include <sstream>
#include <cctype>

using namespace drogon;

namespace erp
{

// 生成唯一文件名
static std::string genFileName(const std::string &ext)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    std::ostringstream oss;
    oss << "img_" << ms << "." << ext;
    return oss.str();
}

void FileController::uploadImage(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    MultiPartParser parser;
    if (parser.parse(req) != 0 || parser.getFiles().empty())
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "未收到文件"));
        return;
    }

    const HttpFile &file = parser.getFiles()[0];

    // 校验扩展名
    std::string ext(file.getFileExtension());
    for (auto &c : ext)
        c = std::tolower((unsigned char)c);

    static const std::string allowed[] = {"jpg", "jpeg", "png", "gif", "webp"};
    bool ok = false;
    for (const auto &a : allowed)
        if (ext == a)
            ok = true;

    if (!ok)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST,
                                    "仅支持 jpg/jpeg/png/gif/webp 图片"));
        return;
    }

    std::string newName = genFileName(ext);

    // 保存到 uploads 目录（saveAs 相对路径会拼到 upload_path 下）
    if (file.saveAs(newName) != 0)
    {
        callback(ApiResponse::error(ErrorCode::INTERNAL_ERROR, "保存文件失败"));
        return;
    }

    Json::Value data;
    data["url"] = "/uploads/" + newName;
    data["filename"] = newName;
    callback(ApiResponse::ok(std::string("上传成功"), data));
}

}  // namespace erp
