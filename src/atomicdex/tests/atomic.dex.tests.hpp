/******************************************************************************
 * Copyright © 2013-2019 The Komodo Platform Developers.                      *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * Komodo Platform software, including this file may be copied, modified,     *
 * propagated or distributed except according to the terms contained in the   *
 * LICENSE file                                                               *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

//! Deps
#include <antara/gaming/world/world.app.hpp>

//! Project
#include "atomicdex/managers/qt.wallet.manager.hpp"
#include "atomicdex/pages/qt.portfolio.page.hpp"
#include "atomicdex/services/mm2/mm2.service.hpp"

struct tests_context : public antara::gaming::world::app
{
    tests_context([[maybe_unused]] char** argv)
    {
#if !defined(WIN32) && !defined(_WIN32)
        //! Creates mm2 service.
        const auto& mm2 = system_manager_.create_system<atomic_dex::mm2_service>(system_manager_);

        //! Creates special wallet for the unit tests then logs to it.
        auto& wallet_manager = system_manager_.create_system<atomic_dex::qt_wallet_manager>(system_manager_);
        system_manager_.create_system<atomic_dex::portfolio_page>(system_manager_);
        if (not wallet_manager.get_wallets().contains("smartfi-desktop_tests"))
        {
            wallet_manager.create("smartfi-desktop_tests", "asdkl lkdsa", "smartfi-desktop_tests");
        }
        else
        {
            SPDLOG_INFO("smartfi-desktop_tests already exists - skipping");
        }
        wallet_manager.login("smartfi-desktop_tests", "smartfi-desktop_tests");

        //! Waits for mm2 to be initialized before running tests
        while (!mm2.is_mm2_running()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
#endif
    }

    antara::gaming::ecs::system_manager&
    system_manager() noexcept
    {
        return system_manager_;
    }
};

extern std::unique_ptr<tests_context> g_context;