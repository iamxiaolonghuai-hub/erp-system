#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 文件上传控制器：图片上传（单据附件 / 商品图 / 供应商资质等）
class FileController : public drogon::HttpController<FileController>
{
  public:
    METHOD_LIST_BEGIN
    // 上传图片（multipart/form-data，字段名 file）
    ADD_METHOD_TO(FileController::uploadImage, "/api/upload/image",
                  drogon::Post, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void uploadImage(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
