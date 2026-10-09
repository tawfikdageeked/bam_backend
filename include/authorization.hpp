#pragma once 
#include <drogon/drogon.h>
#include <string>
#include <string_view>
#include <cstdint>

// site name
constexpr std::string_view site_name = "/bam";



// site_name is a global variable
namespace Auth
{
    // representaion of a user
    struct User
    {
        User(int64_t us_id, int64_t cm_id, int t) : user_id(us_id), company_id(cm_id), tier(t) {}
        int64_t user_id = 0;
        int64_t company_id = 0;
        int tier = -1;
    };

    // initialize the login handler
    void Init();

    // takes an http request checks if there is anyone logged in and return who the fuck is him
    User GetUser(const drogon::HttpRequestPtr& req);

    //takes http request reques from the from the login handler and hanlde the fuck out of loggin in
    void LoginHandler(const drogon::HttpRequestPtr& req, std::function<void (const drogon::HttpResponsePtr&)>&& callback);

}
