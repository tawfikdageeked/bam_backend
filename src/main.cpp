#include <iostream>
#include <drogon/drogon.h>
#include "authorization.hpp"
#include "crypto.hpp"

int main()
{
    drogon::app().setLogLevel(trantor::Logger::kDebug);
    
    std::cout << "Starting Server Engine...\n";
    
    // 1. Boot crypto
    Crypto::Init();

    // 2. Register DB connection (keep createDbClient - the warning is harmless)
    drogon::app().createDbClient("postgresql", "127.0.0.1", 5432, "task_db", "task_user", "525836", 8);

    // 3. Register route handlers
    Auth::Init();

    // 4. Hook seeding into the framework start sequence
    // This runs after Drogon initializes the DB connection pool
    drogon::app().registerBeginningAdvice([]() {
        auto db = drogon::app().getDbClient();
        if (!db) {
            std::cerr << "FATAL: Database client failed to initialize!\n";
            return;
        }

        std::string test_password = "password123";
        std::string hashed_pw = Crypto::HashPassword(test_password);

        db->execSqlAsync(
            "INSERT INTO users (company_id, tier, name, username, password_hash) "
            "VALUES (1, 2, 'Admin', 'admin', $1) ON CONFLICT (username) DO NOTHING",
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

    // 5. Start listener
    drogon::app().addListener("0.0.0.0", 8080);
    std::cout << "Listening on http://0.0.0.0:8080" << site_name << "/login\n";
    
    // 6. Start the Drogon event loop
    drogon::app().run();

    return 0;
}
