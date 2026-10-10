#pragma once

#include <drogon/HttpController.h>

namespace mathan_mart {

class ReviewController : public drogon::HttpController<ReviewController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReviewController::createReview, "/api/products/{productId}/reviews", drogon::Post, drogon::Options);
    ADD_METHOD_TO(ReviewController::getReviews, "/api/products/{productId}/reviews", drogon::Get, drogon::Options);
    METHOD_LIST_END

    void createReview(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
    void getReviews(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, int productId);
};

} // namespace mathan_mart
