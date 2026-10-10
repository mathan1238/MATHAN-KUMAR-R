#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../models/Product.h"

namespace mathan_mart {

class ProductService {
public:
    static ProductService& getInstance();

    ProductService(const ProductService&) = delete;
    ProductService& operator=(const ProductService&) = delete;

    struct ServiceResult {
        bool success = false;
        std::string message;
        int statusCode = 200;
        Product product;
    };

    std::vector<Product> getProducts(const std::string& search = "", 
                                    const std::string& category = "", 
                                    int sellerId = 0);

    std::optional<Product> getProductById(int id);

    ServiceResult createProduct(int sellerId, 
                                const std::string& name, 
                                const std::string& description, 
                                double price, 
                                const std::string& category, 
                                int stock, 
                                const std::string& imageUrl);

    ServiceResult updateProduct(int productId, 
                                int currentUserId, 
                                const std::string& currentUserRole, 
                                const std::string& name, 
                                const std::string& description, 
                                double price, 
                                const std::string& category, 
                                int stock, 
                                const std::string& imageUrl);

    ServiceResult deleteProduct(int productId, 
                                int currentUserId, 
                                const std::string& currentUserRole);

private:
    ProductService() = default;
};

} // namespace mathan_mart
