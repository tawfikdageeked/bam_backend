#include "task.hpp"
#include "authorization.hpp"
#include <string>

void TaskAPI::Init()
{
    std::string base_route = std::string(site_name) + "/tasks";


    drogon::app().registerHandler(base_route, &TaskAPI::GetTasks, {drogon::Get, "Auth::JwtFilter"});
    drogon::app().registerHandler(base_route, &TaskAPI::CreateTask, {drogon::Post, "Auth::JwtFilter"});
    drogon::app().registerHandler(base_route + "/{1}", &TaskAPI::GetTaskById, {drogon::Get, "Auth::JwtFilter"});
    drogon::app().registerHandler(base_route + "/{1}", &TaskAPI::DeleteTask, {drogon::Delete, "Auth::JwtFilter"});
    drogon::app().registerHandler(base_route + "/{1}/status", &TaskAPI::UpdateTaskStatus, {drogon::Patch, "Auth::JwtFilter"});
}


void TaskAPI::GetTasks(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    Auth::User caller = Auth::GetUser(req);
    auto db = drogon::app().getDbClient();

    std::string filter_assigned_to = req->getParameter("assigned_to");

    auto db_callback = [callback](const drogon::orm::Result &result) {
        Json::Value tasks(Json::arrayValue);
        for (const auto &row : result) {
            Json::Value task;
            task["id"] = row["id"].as<int64_t>();
            task["assigned_by"] = row["assigned_by"].as<int64_t>();
            task["assigned_to"] = row["assigned_to"].as<int64_t>();
            task["title"] = row["title"].as<std::string>();
            task["description"] = row["description"].isNull() ? "" : row["description"].as<std::string>();
            task["status"] = row["status"].as<std::string>();
            tasks.append(task);
        }
        Json::Value ret;
        ret["status"] = "success";
        ret["tasks"] = tasks;
        callback(drogon::HttpResponse::newHttpJsonResponse(ret));
    };

    auto db_err = [callback](const drogon::orm::DrogonDbException &e) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k500InternalServerError);
        callback(res);
    };

    if (!filter_assigned_to.empty()) {
        try {
            int64_t target_user = std::stoll(filter_assigned_to);
            db->execSqlAsync("SELECT * FROM tasks WHERE company_id = $1 AND assigned_to = $2 ORDER BY id DESC", 
            db_callback, db_err, caller.company_id, target_user);
        } catch (...) {
            auto res = drogon::HttpResponse::newHttpResponse();
            res->setStatusCode(drogon::k400BadRequest);
            res->setBody("{\"error\":\"Invalid assigned_to parameter\"}");
            callback(res);
        }
    } else {
        db->execSqlAsync("SELECT * FROM tasks WHERE company_id = $1 ORDER BY id DESC", 
        db_callback, db_err, caller.company_id);
    }
}


void TaskAPI::GetTaskById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id)
{
    Auth::User caller = Auth::GetUser(req);
    auto db = drogon::app().getDbClient();

    try {
        int64_t t_id = std::stoll(task_id);
        db->execSqlAsync(
        "SELECT * FROM tasks WHERE id = $1 AND company_id = $2",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setStatusCode(drogon::k404NotFound);
                callback(res);
                return;
            }
            Json::Value task;
            task["id"] = result[0]["id"].as<int64_t>();
            task["title"] = result[0]["title"].as<std::string>();
            task["status"] = result[0]["status"].as<std::string>();
            
            Json::Value ret;
            ret["status"] = "success";
            ret["task"] = task;
            callback(drogon::HttpResponse::newHttpJsonResponse(ret));
        },
        [](const drogon::orm::DrogonDbException &e) {},
        t_id, caller.company_id
        );
    } catch (...) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k400BadRequest);
        callback(res);
    }
}


void TaskAPI::CreateTask(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if(!json || !json->isMember("title") || !json->isMember("assigned_to")) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k400BadRequest);
        res->setBody("{\"error\":\"title and assigned_to are required\"}");
        callback(res);
        return;
    }

    Auth::User caller = Auth::GetUser(req);
    int64_t assigned_to = (*json)["assigned_to"].asInt64();
    std::string title = (*json)["title"].asString();
    std::string description = json->get("description", "").asString();

    auto db = drogon::app().getDbClient();

    db->execSqlAsync(
    "SELECT tier FROM users WHERE id = $1 AND company_id = $2",
    [callback, caller, title, description, assigned_to, db](const drogon::orm::Result &result) {
        if (result.empty()) {
            auto res = drogon::HttpResponse::newHttpResponse();
            res->setStatusCode(drogon::k404NotFound);
            res->setBody("{\"error\":\"Target user not found in your company\"}");
            callback(res);
            return;
        }

        if (caller.tier > result[0]["tier"].as<int>()) {
            auto res = drogon::HttpResponse::newHttpResponse();
            res->setStatusCode(drogon::k403Forbidden);
            res->setBody("{\"error\":\"Cannot assign tasks to higher tier users\"}");
            callback(res);
            return;
        }

        db->execSqlAsync(
        "INSERT INTO tasks (company_id, assigned_by, assigned_to, title, description) VALUES ($1, $2, $3, $4, $5) RETURNING id",
        [callback](const drogon::orm::Result &insert_res) {
            Json::Value ret;
            ret["status"] = "success";
            ret["task_id"] = insert_res[0]["id"].as<int64_t>();
            callback(drogon::HttpResponse::newHttpJsonResponse(ret));
        },
        [](const drogon::orm::DrogonDbException &e) {},
        caller.company_id, caller.user_id, assigned_to, title, description
        );
    },
    [](const drogon::orm::DrogonDbException &e) {},
    assigned_to, caller.company_id
    );
}


void TaskAPI::UpdateTaskStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id)
{
    auto json = req->getJsonObject();
    if(!json || !json->isMember("status")) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k400BadRequest);
        callback(res);
        return;
    }

    Auth::User caller = Auth::GetUser(req);
    std::string new_status = (*json)["status"].asString();
    auto db = drogon::app().getDbClient();

    try {
        int64_t t_id = std::stoll(task_id);


        db->execSqlAsync(
        "UPDATE tasks SET status = $1 WHERE id = $2 AND company_id = $3 AND assigned_to = $4 RETURNING id",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setStatusCode(drogon::k403Forbidden);
                res->setBody("{\"error\":\"Task not found or you are not the assignee\"}");
                callback(res);
                return;
            }
            Json::Value ret;
            ret["status"] = "success";
            callback(drogon::HttpResponse::newHttpJsonResponse(ret));
        },
        [](const drogon::orm::DrogonDbException &e) {},
        new_status, t_id, caller.company_id, caller.user_id
        );
    } catch (...) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k400BadRequest);
        callback(res);
    }
}

void TaskAPI::DeleteTask(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id)
{
    Auth::User caller = Auth::GetUser(req);
    auto db = drogon::app().getDbClient();

    try {
        int64_t t_id = std::stoll(task_id);

        db->execSqlAsync(
        "DELETE FROM tasks WHERE id = $1 AND company_id = $2 AND assigned_by = $3 RETURNING id",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto res = drogon::HttpResponse::newHttpResponse();
                res->setStatusCode(drogon::k403Forbidden);
                res->setBody("{\"error\":\"Task not found or you lack permission to delete it\"}");
                callback(res);
                return;
            }
            Json::Value ret;
            ret["status"] = "success";
            callback(drogon::HttpResponse::newHttpJsonResponse(ret));
        },
        [](const drogon::orm::DrogonDbException &e) {},
        t_id, caller.company_id, caller.user_id
        );
    } catch (...) {
        auto res = drogon::HttpResponse::newHttpResponse();
        res->setStatusCode(drogon::k400BadRequest);
        callback(res);
    }
}
