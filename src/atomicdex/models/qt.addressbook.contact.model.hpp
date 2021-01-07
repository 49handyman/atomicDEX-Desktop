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

#pragma once

//! Project Headers
#include "atomicdex/managers/addressbook.manager.hpp" //> addressbook_manager.
#include "atomicdex/models/qt.addressbook.contact.addresses.model.hpp"

namespace atomic_dex
{
    class ENTT_API addressbook_contact_model final : public QAbstractListModel
    {
        // Tells QT this class uses signal/slots mechanisms and/or has GUI elements.
        Q_OBJECT
      
        friend class addressbook_model;
    
      public:
        enum ContactRoles
        {
            WalletInfoRole = Qt::UserRole + 1
        };
        Q_ENUMS(ContactRoles)

        explicit addressbook_contact_model(ag::ecs::system_manager& system_manager, QString name, QObject* parent = nullptr);
        ~addressbook_contact_model() noexcept final;
    
        /// QAbstractListModel functions
        ////////////////////////////////
    
        [[nodiscard]] QVariant               data(const QModelIndex& index, int role) const final;
        [[nodiscard]] int                    rowCount(const QModelIndex& parent = QModelIndex()) const final;
        [[nodiscard]] QHash<int, QByteArray> roleNames() const final;
    
        ////////////////////////////////
        
        /// QML API
        ///////////
        
        Q_INVOKABLE bool add_category(const QString& category) noexcept;
        
        Q_INVOKABLE void remove_category(const QString& category) noexcept;
    
        // Gets the contact address of a specific ticker.
        // Returns the corresponding `addressbook_contact_addresses_model*` as `QVariant according to the given ticker
        [[nodiscard]]
        Q_INVOKABLE QVariant get_addresses(const QString& ticker);
        
        // Resets this model then reloads its data from the persistent data.
        Q_INVOKABLE void reload();
    
        // Saves the model modifications in the persistent data.
        Q_INVOKABLE void save();
    
        Q_PROPERTY(QString name READ get_name WRITE set_name NOTIFY nameChanged)
        [[nodiscard]]
        const QString& get_name() const noexcept;
        void set_name(const QString& name) noexcept;
    
        Q_PROPERTY(QStringList categories READ get_categories WRITE set_categories NOTIFY categoriesChanged)
        [[nodiscard]]
        const QStringList& get_categories() const noexcept;
        void set_categories(QStringList categories) noexcept;
        
      signals:
        void nameChanged();
        void categoriesChanged();
    
        ///////////

      private:
        // Loads this model data from the persistent data.
        void populate();
        
        // Clears this model data.
        void clear();

      private:
        ag::ecs::system_manager&                      m_system_manager;
    
        QString                                       m_name;
    
        QStringList                                   m_categories;
        
        QVector<addressbook_contact_addresses_model*> m_model_data;
    };
}