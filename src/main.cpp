#include <iostream>
#include <drogon/drogon.h>
#include "authorization.hpp"
#include "crypto.hpp"
#include "task.hpp" // <-- Added your new Task API header

int main()
{
    drogon::app().setLogLevel(trantor::Logger::kDebug);
    std::cout << "Starting Server Engine...\n";
    
    // 1. Boot Subsystems
    Crypto::Init();
    drogon::app().createDbClient("postgresql", "127.0.0.1", 5432, "task_db", "task_user", "525836", 8);

    // 2. Register Route Handlers
    Auth::Init();
    TaskAPI::Init(); // <-- Boot the tasks module

    // 3. Database Seeder
    drogon::app().registerBeginningAdvice([]() {
        auto db = drogon::app().getDbClient();
        if (!db) {
            std::cerr << "FATAL: Database client failed to initialize!\n";
            return;
        }

        std::string test_password = "password123";
        std::string hashed_pw = Crypto::HashPassword(test_password);

        // Seeding the admin with Tier 0 (highest rank)
        db->execSqlAsync(
            "INSERT INTO users (company_id, tier, name, username, password_hash) "
            "VALUES (1, 0, 'Admin', 'admin', $1) ON CONFLICT (username) DO NOTHING",
            [test_password](const drogon::orm::Result &r) {
                std::cout << "\n============================================\n";
                std::cout << "Database verified! Ready for login testing.\n";
                std::cout << "Username: admin\n";
                std::cout << "Password: " << test_password << "\n";
                std::cout << "============================================\n\n";
            },
            [](const drogon::orm::DrogonDbException &e) {
                std::cerr << "Database Seeding Warning: " << e.base().what() << "\n";
            },
            hashed_pw
        );
    });

    // 4. Start listener
    drogon::app().addListener("0.0.0.0", 8080);
    std::cout << "Listening on http://0.0.0.0:8080" << site_name << "\n";
    
    drogon::app().run();
    return 0;
}
