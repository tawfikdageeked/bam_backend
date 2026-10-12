#pragma once
#include <drogon/drogon.h>

namespace TaskAPI
{
    void Init();

    // 1. Fetch all tasks (supports ?assigned_to=X filtering)
    void GetTasks(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    
    // 2. Fetch one specific task
    void GetTaskById(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id);
    
    // 3. Create a task (enforces tier hierarchy)
    void CreateTask(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    
    // 4. Update task status (only the person assigned to it can do this)
    void UpdateTaskStatus(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id);
    
    // 5. Delete a task (only the person who created it can do this)
    void DeleteTask(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback, std::string task_id);
}
