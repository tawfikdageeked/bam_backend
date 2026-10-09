#include "authorization.hpp"
#include "crypto.hpp"
#include <jwt-cpp/jwt.h>
#include <iostream>

namespace Auth
{
    void Init()
    {
        std::string login_route = std::string(site_name) + "/login";
        drogon::app().registerHandler(login_route, &Auth::LoginHandler, {drogon::Post});
    }

    User GetUser(const drogon::HttpRequestPtr& req)
    {
        return User(
            req->attributes()->get<int64_t>("user_id"),
            req->attributes()->get<int64_t>("company_id"),
            req->attributes()->get<int>("tier")
        );
    }

    void LoginHandler(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback)
    {
        std::cout << "[HTTP] -> /bam/login received\n";
        auto json = req->getJsonObject();
        if(!json)
        {
            std::cout << "[HTTP] -> No JSON in request body\n";
            auto res = drogon::HttpResponse::newHttpResponse();
            res->setStatusCode(drogon::k400BadRequest);
            callback(res);
            return;
        }

        std::string username = (*json)["username"].asString();
        std::string password = (*json)["password"].asString();

        std::cout << "[HTTP] -> Querying DB for user: " << username << "\n";

        auto db = drogon::app().getDbClient();

        db->execSqlAsync(
            "SELECT id, company_id, tier, password_hash FROM users WHERE username = $1",
            [callback, password](const drogon::orm::Result &result)
            {
                std::cout << "[DB] -> Query returned " << result.size() << " rows\n";
                if(result.empty())
                {
                    auto res = drogon::HttpResponse::newHttpResponse();
                    res->setStatusCode(drogon::k401Unauthorized);
                    callback(res);
                    return;
                }

                int64_t db_user_id = result[0]["id"].as<int64_t>();
                int64_t db_company_id = result[0]["company_id"].as<int64_t>();
                int db_tier = result[0]["tier"].as<int>();
                std::string db_hash = result[0]["password_hash"].as<std::string>();

                if(!Crypto::VerifyPassword(db_hash, password))
                {
                    auto res = drogon::HttpResponse::newHttpResponse();
                    res->setStatusCode(drogon::k401Unauthorized);
                    callback(res);
                    return;
                }

                std::string token = jwt::create()
                .set_type("JWT")
                .set_payload_claim("user_id", jwt::claim(std::to_string(db_user_id)))
                .set_payload_claim("company_id", jwt::claim(std::to_string(db_company_id)))
                .set_payload_claim("tier", jwt::claim(std::to_string(db_tier)))
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds(900))
                .sign(jwt::algorithm::hs256{"CLASSIFIED"});

                Json::Value ret;
                ret["token"] = token;
                auto res = drogon::HttpResponse::newHttpJsonResponse(ret);
                res->setStatusCode(drogon::k200OK);
                callback(res);
            },
            [callback](const drogon::orm::DrogonDbException &e)
            {
                std::cerr << "Login DB Error: " << e.base().what() << std::endl;
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setStatusCode(drogon::k500InternalServerError);
                callback(res);
            },
            username
        );
    }
}
