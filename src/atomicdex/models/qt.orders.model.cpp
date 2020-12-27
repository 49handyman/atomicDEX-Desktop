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

#if defined(linux) || defined(__APPLE__)
#    define BOOST_STACKTRACE_USE_ADDR2LINE
#    if defined(__APPLE__)
#        define _GNU_SOURCE
#    endif
#    include <boost/stacktrace.hpp>
#endif


//! Project
#include "atomicdex/events/qt.events.hpp"
#include "atomicdex/models/qt.orders.model.hpp"
#include "atomicdex/pages/qt.settings.page.hpp"
#include "atomicdex/services/mm2/mm2.service.hpp"
#include "atomicdex/services/price/global.provider.hpp"
#include "atomicdex/utilities/qt.utilities.hpp"

namespace atomic_dex
{
    orders_model::orders_model(ag::ecs::system_manager& system_manager, entt::dispatcher& dispatcher, QObject* parent) noexcept :
        QAbstractListModel(parent), m_system_manager(system_manager), m_dispatcher(dispatcher), m_model_proxy(new orders_proxy_model(this))
    {
        this->m_model_proxy->setSourceModel(this);
        this->m_model_proxy->setDynamicSortFilter(true);
        this->m_model_proxy->setSortRole(UnixTimestampRole);
        this->m_model_proxy->setFilterRole(TickerPairRole);
        this->m_model_proxy->sort(0, Qt::DescendingOrder);

        this->m_dispatcher.sink<current_currency_changed>().connect<&orders_model::on_current_currency_changed>(this);
    }

    int
    orders_model::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return this->m_model_data.orders_and_swaps.size();
    }

    bool
    orders_model::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (!hasIndex(index.row(), index.column(), index.parent()) || !value.isValid())
        {
            return false;
        }

        t_order_swaps_data& item = m_model_data.orders_and_swaps[index.row()];
        switch (static_cast<OrdersRoles>(role))
        {
        case BaseCoinRole:
            item.base_coin = value.toString();
            break;
        case RelCoinRole:
            item.rel_coin = value.toString();
            break;
        case TickerPairRole:
            item.ticker_pair = value.toString();
            break;
        case BaseCoinAmountRole:
            item.base_amount = value.toString();
            break;
        case BaseCoinAmountCurrentCurrencyRole:
            item.base_amount_fiat = value.toString();
            break;
        case RelCoinAmountRole:
            item.rel_amount = value.toString();
            break;
        case RelCoinAmountCurrentCurrencyRole:
            item.rel_amount_fiat = value.toString();
            break;
        case OrderTypeRole:
            item.order_type = value.toString();
            break;
        case HumanDateRole:
            item.human_date = value.toString();
            break;
        case UnixTimestampRole:
            item.unix_timestamp = value.toULongLong();
            break;
        case OrderIdRole:
            item.order_id = value.toString();
            break;
        case OrderStatusRole:
            item.order_status = value.toString();
            break;
        case MakerPaymentIdRole:
            item.maker_payment_id = value.toString();
            break;
        case TakerPaymentIdRole:
            item.taker_payment_id = value.toString();
            break;
        case CancellableRole:
            item.is_cancellable = value.toBool();
            break;
        case IsMakerRole:
            item.is_maker = value.toBool();
            break;
        case IsSwapRole:
            item.is_swap = value.toBool();
        case IsRecoverableRole:
            item.is_recoverable = value.toBool();
        case OrderErrorStateRole:
            item.order_error_state = value.toString();
        case OrderErrorMessageRole:
            item.order_error_message = value.toString();
        case EventsRole:
            item.events = value.toJsonArray();
        case SuccessEventsRole:
            item.success_events = value.toStringList();
        case ErrorEventsRole:
            item.error_events = value.toStringList();
        }

        emit dataChanged(index, index, {role});
        return true;
    }

    QVariant
    orders_model::data(const QModelIndex& index, int role) const
    {
        if (!hasIndex(index.row(), index.column(), index.parent()))
        {
            return {};
        }

        const t_order_swaps_data& item = m_model_data.orders_and_swaps.at(index.row());
        switch (static_cast<OrdersRoles>(role))
        {
        case BaseCoinRole:
            return item.base_coin;
        case RelCoinRole:
            return item.rel_coin;
        case TickerPairRole:
            return item.ticker_pair;
        case BaseCoinAmountRole:
            return item.base_amount;
        case BaseCoinAmountCurrentCurrencyRole:
            return item.base_amount_fiat;
        case RelCoinAmountRole:
            return item.rel_amount;
        case RelCoinAmountCurrentCurrencyRole:
            return item.rel_amount_fiat;
        case OrderTypeRole:
            return item.order_type;
        case HumanDateRole:
            return item.human_date;
        case UnixTimestampRole:
            return item.unix_timestamp;
        case OrderIdRole:
            return item.order_id;
        case OrderStatusRole:
            return item.order_status;
        case MakerPaymentIdRole:
            return item.maker_payment_id;
        case TakerPaymentIdRole:
            return item.taker_payment_id;
        case CancellableRole:
            return item.is_cancellable;
        case IsMakerRole:
            return item.is_maker;
        case IsSwapRole:
            return item.is_swap;
        case IsRecoverableRole:
            return item.is_recoverable;
        case OrderErrorStateRole:
            return item.order_error_state;
        case OrderErrorMessageRole:
            return item.order_error_message;
        case EventsRole:
            return item.events;
        case SuccessEventsRole:
            return item.success_events;
        case ErrorEventsRole:
            return item.error_events;
        }
        return {};
    }

    bool
    orders_model::removeRows(int position, int rows, [[maybe_unused]] const QModelIndex& parent)
    {
        SPDLOG_DEBUG("(orders_model::removeRows) removing {} elements at position {}", rows, position);

        beginRemoveRows(QModelIndex(), position, position + rows - 1);
        for (int row = 0; row < rows; ++row)
        {
            this->m_model_data.orders_and_swaps.erase(begin(m_model_data.orders_and_swaps) + position);
            emit lengthChanged();
        }
        endRemoveRows();

        return true;
    }

    /*void
    orders_model::update_swap(const ::mm2::api::swap_contents& contents) noexcept
    {
        if (const auto res = this->match(index(0, 0), OrderIdRole, QString::fromStdString(contents.uuid)); not res.isEmpty())
        {
            const QModelIndex& idx      = res.at(0);
            bool               is_maker = boost::algorithm::to_lower_copy(contents.type) == "maker";
            update_value(OrdersRoles::IsRecoverableRole, contents.funds_recoverable, idx, *this);
            auto&& [prev_value, new_value, is_change] =
                update_value(OrdersRoles::OrderStatusRole, determine_order_status_from_last_event(contents), idx, *this);
            update_value(
                OrdersRoles::UnixTimestampRole, not contents.events.empty() ? contents.events.back().at("timestamp").get<unsigned long long>() : 0, idx, *this);
            auto&& [prev_value_d, new_value_d, _] = update_value(
                OrdersRoles::HumanDateRole,
                not contents.events.empty() ? QString::fromStdString(contents.events.back().at("human_timestamp").get<std::string>()) : "", idx, *this);
            if (is_change)
            {
                const QString& base_coin = data(idx, OrdersRoles::BaseCoinRole).toString();
                const QString& rel_coin  = data(idx, OrdersRoles::RelCoinRole).toString();
                m_dispatcher.trigger<swap_status_notification>(
                    QString::fromStdString(contents.uuid), prev_value.toString(), new_value.toString(), base_coin, rel_coin, new_value_d.toString());
            }
            update_value(OrdersRoles::MakerPaymentIdRole, determine_payment_id(contents, is_maker, false), idx, *this);
            update_value(OrdersRoles::TakerPaymentIdRole, determine_payment_id(contents, is_maker, true), idx, *this);
            auto [state, msg] = extract_error(contents);
            update_value(OrdersRoles::OrderErrorStateRole, state, idx, *this);
            update_value(OrdersRoles::OrderErrorMessageRole, msg, idx, *this);
            update_value(OrdersRoles::EventsRole, nlohmann_json_array_to_qt_json_array(contents.events), idx, *this);

            update_value(OrdersRoles::SuccessEventsRole, vector_std_string_to_qt_string_list(contents.success_events), idx, *this);
            update_value(OrdersRoles::ErrorEventsRole, vector_std_string_to_qt_string_list(contents.error_events), idx, *this);

            //! Updates values in current currency of amounts traded.
            auto&& [base_coin_amount_fiat, rel_coin_amount_fiat] = determine_amounts_in_current_currency(contents);
            update_value(OrdersRoles::BaseCoinAmountCurrentCurrencyRole, QString::fromStdString(base_coin_amount_fiat), idx, *this);
            update_value(OrdersRoles::RelCoinAmountCurrentCurrencyRole, QString::fromStdString(rel_coin_amount_fiat), idx, *this);

            emit lengthChanged();
        }
    }

    void
    orders_model::update_existing_order(const ::mm2::api::my_order_contents& contents) noexcept
    {
        if (const auto res = this->match(index(0, 0), OrderIdRole, QString::fromStdString(contents.order_id)); not res.isEmpty())
        {
            const QModelIndex& idx = res.at(0);
            update_value(OrdersRoles::CancellableRole, contents.cancellable, idx, *this);
            update_value(OrdersRoles::IsMakerRole, contents.order_type == "maker", idx, *this);
            update_value(OrdersRoles::OrderTypeRole, QString::fromStdString(contents.order_type), idx, *this);
            if (contents.order_type == "maker")
            {
                update_value(OrdersRoles::BaseCoinAmountRole, QString::fromStdString(contents.base_amount), idx, *this);
                update_value(OrdersRoles::RelCoinAmountRole, QString::fromStdString(contents.rel_amount), idx, *this);
            }
            emit lengthChanged();
        }
    }*/

    /*void
    orders_model::refresh_or_insert_orders() noexcept
    {
        std::error_code ec;
        const auto&     mm2    = m_system_manager.get_system<mm2_service>();
        const auto      orders = mm2.get_raw_orders();

        auto functor_process_orders = [this](auto&& orders) {
            std::vector<order_swaps_data> to_init;
            for (auto&& [key, value]: orders)
            {
                if (this->m_orders_id_registry.find(value.order_id) != this->m_orders_id_registry.end())
                {
                    //! Find update needed
                    this->update_existing_order(value);
                }
                else
                {
                    m_orders_id_registry.emplace(to_init.emplace_back(from_order_content(value)).order_id.toStdString());
                    //! Not found, insert and initialize.
                    // this->initialize_order(value);
                }
            }

            if (not to_init.empty())
            {
                this->common_insert(to_init, "orders");
            }
        };

        if (not orders.maker_orders.empty())
        {
            functor_process_orders(orders.maker_orders);
        }
        if (not orders.taker_orders.empty())
        {
            functor_process_orders(orders.taker_orders);
        }

        //! Check for cleaning orders that are not present anymore
        std::unordered_set<std::string> to_remove;
        for (auto&& id: this->m_orders_id_registry)
        {
            //! Check if the current id from the model registry is present in the orders collection

            //! Check in maker_orders
            bool res = std::none_of(begin(orders.maker_orders), end(orders.maker_orders), [id](auto&& contents) { return contents.second.order_id == id; });

            //! And compute with taker orders
            res &= std::none_of(begin(orders.taker_orders), end(orders.taker_orders), [id](auto&& contents) { return contents.second.order_id == id; });
            if (res)
            {
                //! If it's the case retrieve the index of the row that match this id
                auto res_list = this->match(index(0, 0), OrderIdRole, QString::fromStdString(id));
                if (not res_list.empty())
                {
                    //! And then delete it
                    this->removeRow(res_list.at(0).row());
                    to_remove.emplace(id);
                }
            }
        }
        for (auto&& cur_to_remove: to_remove) { m_orders_id_registry.erase(cur_to_remove); }
    }*/

    /*void
    orders_model::refresh_or_insert_swaps() noexcept
    {
        const auto& mm2          = m_system_manager.get_system<mm2_service>();
        const auto  result       = mm2.get_swaps();
        const auto  orders       = mm2.get_raw_orders();
        const auto  current_size = static_cast<int>((result.swaps.size()) + orders.maker_orders.size() + orders.taker_orders.size());

        this->set_average_events_time_registry(nlohmann_json_object_to_qt_json_object(result.average_events_time));
        std::vector<order_swaps_data> to_init;
        int                     difference = current_size - static_cast<int>(this->m_model_data.size());
        for (auto&& current_swap: result.swaps)
        {
            if (this->m_swaps_id_registry.find(current_swap.uuid) != this->m_swaps_id_registry.end())
            {
                this->update_swap(current_swap);
            }
            else
            {
                if (difference > 0)
                {
                    m_swaps_id_registry.emplace(to_init.emplace_back(from_swap_content(current_swap)).order_id.toStdString());
                }
            }
        }

        if (not to_init.empty())
        {
            this->common_insert(to_init, "swaps");
        }
    }*/

    QHash<int, QByteArray>
    orders_model::roleNames() const
    {
        return {
            {BaseCoinRole, "base_coin"},
            {RelCoinRole, "rel_coin"},
            {TickerPairRole, "ticker_pair"},
            {BaseCoinAmountRole, "base_amount"},
            {BaseCoinAmountCurrentCurrencyRole, "base_amount_current_currency"},
            {RelCoinAmountRole, "rel_amount"},
            {RelCoinAmountCurrentCurrencyRole, "rel_amount_current_currency"},
            {OrderTypeRole, "type"},
            {IsMakerRole, "is_maker"},
            {HumanDateRole, "date"},
            {UnixTimestampRole, "timestamp"},
            {OrderIdRole, "order_id"},
            {OrderStatusRole, "order_status"},
            {MakerPaymentIdRole, "maker_payment_id"},
            {TakerPaymentIdRole, "taker_payment_id"},
            {IsSwapRole, "is_swap"},
            {CancellableRole, "cancellable"},
            {IsRecoverableRole, "recoverable"},
            {OrderErrorStateRole, "order_error_state"},
            {OrderErrorMessageRole, "order_error_message"},
            {EventsRole, "events"},
            {SuccessEventsRole, "success_events"},
            {ErrorEventsRole, "error_events"}};
    }

    int
    orders_model::get_length() const noexcept
    {
        return this->m_model_proxy->rowCount(QModelIndex());
    }

    orders_proxy_model*
    orders_model::get_orders_proxy_mdl() const noexcept
    {
        return m_model_proxy;
    }

    void
    orders_model::reset() noexcept
    {
        SPDLOG_DEBUG("clearing orders");
        this->beginResetModel();
        this->m_swaps_id_registry.clear();
        this->m_orders_id_registry.clear();
        this->m_model_data = {};
        this->endResetModel();
    }

    /*std::pair<std::string, std::string>
    orders_model::determine_amounts_in_current_currency(
        const std::string& base_coin, const std::string& base_amount, const std::string& rel_coin, const std::string& rel_amount) noexcept
    {
        const auto&     settings_system     = m_system_manager.get_system<settings_page>();
        const auto&     current_currency    = settings_system.get_current_currency().toStdString();
        const auto&     global_price_system = m_system_manager.get_system<global_price_service>();
        std::string     base_amount_in_currency;
        std::string     rel_amount_in_currency;

        base_amount_in_currency = global_price_system.get_price_as_currency_from_amount(current_currency, base_coin, base_amount);
        rel_amount_in_currency  = global_price_system.get_price_as_currency_from_amount(current_currency, rel_coin, rel_amount);
        return std::make_pair(base_amount_in_currency, rel_amount_in_currency);
    }

    std::pair<std::string, std::string>
    orders_model::determine_amounts_in_current_currency(const ::mm2::api::swap_contents& contents)
    {
        bool is_maker = boost::algorithm::to_lower_copy(contents.type) == "maker";

        if (is_maker)
        {
            return determine_amounts_in_current_currency(contents.maker_coin, contents.maker_amount, contents.taker_coin, contents.taker_amount);
        }
        return determine_amounts_in_current_currency(contents.taker_coin, contents.taker_amount, contents.maker_coin, contents.maker_amount);
    }*/

    void
    orders_model::on_current_currency_changed([[maybe_unused]] const current_currency_changed&) noexcept
    {
        auto& mm2 = m_system_manager.get_system<mm2_service>();

        mm2.batch_fetch_orders_and_swap();
    }
} // namespace atomic_dex

namespace atomic_dex
{
    QVariant
    atomic_dex::orders_model::get_average_events_time_registry() const noexcept
    {
        return m_json_time_registry;
    }

    void
    atomic_dex::orders_model::set_average_events_time_registry(const QVariant& average_time_registry) noexcept
    {
        m_json_time_registry = average_time_registry;
        emit onAverageEventsTimeRegistryChanged();
    }

    bool
    atomic_dex::orders_model::swap_is_in_progress(const QString& coin) const noexcept
    {
        for (auto&& cur_hist_swap: m_model_data.orders_and_swaps)
        {
            if ((cur_hist_swap.base_coin == coin || cur_hist_swap.rel_coin == coin) &&
                (cur_hist_swap.order_status == "matched" || cur_hist_swap.order_status == "ongoing" || cur_hist_swap.order_status == "matching"))
            {
                return true;
            }
        }
        return false;
    }

    /*order_swaps_data
    orders_model::from_swap_content(const ::mm2::api::swap_contents& contents)
    {
        bool       is_maker = boost::algorithm::to_lower_copy(contents.type) == "maker";
        order_swaps_data data{
            .is_maker         = is_maker,
            .base_coin        = is_maker ? QString::fromStdString(contents.maker_coin) : QString::fromStdString(contents.taker_coin),
            .rel_coin         = is_maker ? QString::fromStdString(contents.taker_coin) : QString::fromStdString(contents.maker_coin),
            .base_amount      = is_maker ? QString::fromStdString(contents.maker_amount) : QString::fromStdString(contents.taker_amount),
            .rel_amount       = is_maker ? QString::fromStdString(contents.taker_amount) : QString::fromStdString(contents.maker_amount),
            .order_type       = is_maker ? "maker" : "taker",
            .human_date       = not contents.events.empty() ? QString::fromStdString(contents.events.back().at("human_timestamp").get<std::string>()) : "",
            .unix_timestamp   = not contents.events.empty() ? contents.events.back().at("timestamp").get<unsigned long long>() : 0,
            .order_id         = QString::fromStdString(contents.uuid),
            .order_status     = determine_order_status_from_last_event(contents),
            .maker_payment_id = determine_payment_id(contents, is_maker, false),
            .taker_payment_id = determine_payment_id(contents, is_maker, true),
            .is_swap          = true,
            .is_cancellable   = false,
            .is_recoverable   = contents.funds_recoverable,
            .events           = nlohmann_json_array_to_qt_json_array(contents.events),
            .error_events     = vector_std_string_to_qt_string_list(contents.error_events),
            .success_events   = vector_std_string_to_qt_string_list(contents.success_events)};

        //! Sets amounts in fiat.
        auto&& [base_fiat_value, rel_fiat_value] = determine_amounts_in_current_currency(
            data.base_coin.toStdString(), data.base_amount.toStdString(), data.rel_coin.toStdString(), data.rel_amount.toStdString());
        data.base_amount_fiat = QString::fromStdString(base_fiat_value);
        data.rel_amount_fiat  = QString::fromStdString(rel_fiat_value);

        data.ticker_pair = data.base_coin + "/" + data.rel_coin;
        if (data.order_status == "failed")
        {
            auto error               = extract_error(contents);
            data.order_error_state   = error.first;
            data.order_error_message = error.second;
        }

        if (data.order_status == "matched")
        {
            using namespace std::string_literals;
            m_dispatcher.trigger<swap_status_notification>(data.order_id, "matching", "matched", data.base_coin, data.rel_coin, data.human_date);
        }

        return data;
    }*/

    /*order_swaps_data
    orders_model::from_order_content(const ::mm2::api::my_order_contents& contents)
    {
        order_swaps_data data{
            .is_maker       = contents.order_type == "maker",
            .base_coin      = contents.action == "Sell" ? QString::fromStdString(contents.base) : QString::fromStdString(contents.rel),
            .rel_coin       = contents.action == "Sell" ? QString::fromStdString(contents.rel) : QString::fromStdString(contents.base),
            .base_amount    = contents.action == "Sell" ? QString::fromStdString(contents.base_amount) : QString::fromStdString(contents.rel_amount),
            .rel_amount     = contents.action == "Sell" ? QString::fromStdString(contents.rel_amount) : QString::fromStdString(contents.base_amount),
            .order_type     = QString::fromStdString(contents.order_type),
            .human_date     = QString::fromStdString(contents.human_timestamp),
            .unix_timestamp = static_cast<unsigned long long>(contents.timestamp),
            .order_id       = QString::fromStdString(contents.order_id),
            .order_status   = "matching",
            .is_swap        = false,
            .is_cancellable = contents.cancellable,
            .is_recoverable = false};
        if (contents.action.empty() && contents.order_type == "maker")
        {
            data.base_coin   = QString::fromStdString(contents.base);
            data.rel_coin    = QString::fromStdString(contents.rel);
            data.base_amount = QString::fromStdString(contents.base_amount);
            data.rel_amount  = QString::fromStdString(contents.rel_amount);
        }

        //! Sets amounts in fiat.
        auto&& [base_fiat_value, rel_fiat_value] = determine_amounts_in_current_currency(
            data.base_coin.toStdString(), data.base_amount.toStdString(), data.rel_coin.toStdString(), data.rel_amount.toStdString());
        data.base_amount_fiat = QString::fromStdString(base_fiat_value);
        data.rel_amount_fiat  = QString::fromStdString(rel_fiat_value);

        data.ticker_pair = data.base_coin + "/" + data.rel_coin;

        return data;
    }*/

    void
    orders_model::update_existing_order(const t_order_swaps_data& contents) noexcept
    {
        if (const auto res = this->match(index(0, 0), OrderIdRole, contents.order_id); not res.isEmpty())
        {
            const QModelIndex& idx = res.at(0);
            update_value(OrdersRoles::CancellableRole, contents.is_cancellable, idx, *this);
            update_value(OrdersRoles::IsMakerRole, contents.order_type == "maker", idx, *this);
            update_value(OrdersRoles::OrderTypeRole, contents.order_type, idx, *this);
            if (contents.order_type == "maker")
            {
                update_value(OrdersRoles::BaseCoinAmountRole, contents.base_amount, idx, *this);
                update_value(OrdersRoles::RelCoinAmountRole, contents.rel_amount, idx, *this);
            }
            emit lengthChanged();
        }
    }

    void
    orders_model::init_model(const orders_and_swaps& contents)
    {
        const auto size = contents.orders_and_swaps.size();
        SPDLOG_DEBUG("First time initialization, inserting {} elements", size);
        beginResetModel();
        m_model_data = std::move(contents);
        endResetModel();
        m_orders_id_registry = std::move(m_model_data.orders_registry);
        emit lengthChanged();
        emit currentPageChanged();
        emit nbPageChanged();
        this->set_average_events_time_registry(nlohmann_json_object_to_qt_json_object(m_model_data.average_events_time));
    }

    void
    orders_model::common_insert(const std::vector<t_order_swaps_data>& contents, const std::string& kind)
    {
        SPDLOG_INFO("common_insert, nb elements to insert: {}", contents.size());
        auto& data = m_model_data.orders_and_swaps;
        beginInsertRows(QModelIndex(), rowCount(), rowCount() + contents.size() - 1);
        data.insert(end(data), begin(contents), end(contents));
        if (kind == "orders")
        {
            m_model_data.nb_orders += contents.size();
        }
        endInsertRows();
        emit lengthChanged();
        SPDLOG_DEBUG("{} model size: {}", kind, rowCount());
    }

    void
    orders_model::refresh_or_insert(bool after_manual_reset)
    {
        if (after_manual_reset)
        {
            this->set_fetching_busy(false);
        }

        if (is_fetching_busy())
        {
            return;
        }
        const auto& mm2      = m_system_manager.get_system<mm2_service>();
        const auto  contents = mm2.get_orders_and_swaps();

        //! If model is empty let's init it once
        if (m_model_data.orders_and_swaps.size() == 0)
        {
            init_model(contents);
        }
        else
        {
            this->set_common_data(contents);
            update_or_insert_orders(contents);
            // update_or_insert_swaps(contents);
        }
    }

    void
    orders_model::update_or_insert_orders(const orders_and_swaps& contents)
    {
        const auto&                     data = contents.orders_and_swaps;
        std::unordered_set<std::string> are_present;
        if (contents.nb_orders > 0)
        {
            std::vector<t_order_swaps_data> to_init;
            std::for_each(begin(data), begin(data) + contents.nb_orders, [this, &to_init, &are_present](auto&& cur) {
                if (this->m_orders_id_registry.contains(cur.order_id.toStdString()))
                {
                    this->update_existing_order(cur);
                }
                else
                {
                    m_orders_id_registry.emplace(to_init.emplace_back(cur).order_id.toStdString());
                }
                are_present.emplace(cur.order_id.toStdString());
            });

            if (not to_init.empty())
            {
                this->common_insert(to_init, "orders");
            }
        }

        remove_orders(are_present);
    }

    void
    orders_model::remove_orders(const t_orders_id_registry& are_present)
    {
        std::vector<std::string> to_remove;
        for (auto&& id: this->m_orders_id_registry)
        {
            if (!are_present.contains(id))
            {
                //! If it's the case retrieve the index of the row that match this id
                auto res_list = this->match(index(0, 0), OrderIdRole, QString::fromStdString(id));
                if (not res_list.empty())
                {
                    //! And then delete it
                    this->removeRow(res_list.at(0).row());
                    m_model_data.nb_orders -= 1;
                    to_remove.emplace_back(id);
                }
            }
        }
        for (auto&& cur_to_remove: to_remove) { m_orders_id_registry.erase(cur_to_remove); }
    }

    int
    orders_model::get_current_page() const noexcept
    {
        return static_cast<int>(m_model_data.current_page);
    }

    void
    orders_model::set_current_page(int current_page) noexcept
    {
        if (static_cast<std::size_t>(current_page) != m_model_data.current_page)
        {
            this->reset(); ///< We change page, we need to clear
            this->set_fetching_busy(true);
            auto& mm2 = this->m_system_manager.get_system<mm2_service>();
            mm2.set_orders_and_swaps_pagination_infos(static_cast<std::size_t>(current_page));
        }
    }

    bool
    orders_model::is_fetching_busy() const noexcept
    {
        return m_fetching_busy.load();
    }

    void
    orders_model::set_fetching_busy(bool fetching_status) noexcept
    {
        if (fetching_status != m_fetching_busy)
        {
            m_fetching_busy = fetching_status;
            emit fetchingStatusChanged();
        }
    }

    int
    orders_model::get_nb_pages() const noexcept
    {
        return m_model_data.nb_pages;
    }

    void
    orders_model::set_common_data(const orders_and_swaps& contents) noexcept
    {
        this->set_average_events_time_registry(nlohmann_json_object_to_qt_json_object(contents.average_events_time));
        m_model_data.nb_orders = contents.nb_orders;
        if (m_model_data.nb_pages != contents.nb_pages)
        {
            SPDLOG_INFO("nb page changed");
            m_model_data.nb_pages = contents.nb_pages;
            emit nbPageChanged();
        }
        if (m_model_data.current_page != contents.current_page)
        {
            SPDLOG_INFO("Page is different from mm2 contents, force change");
            this->set_current_page(contents.current_page);
        }
    }
} // namespace atomic_dex
