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

//! Qt
#include <QAbstractListModel> //> QAbstractListModel.
#include <QObject>            //> Q_OBJECT, Q_PROPERTY.
#include <QMap>               //> QMap.

//! Project
#include "atomicdex/managers/addressbook.manager.hpp"

namespace atomic_dex
{
    class addressbook_contact_addresses_model final : public QAbstractTableModel
    {
        /// \brief Tells QT this class uses signal/slots mechanisms and/or has GUI elements.
        Q_OBJECT

        struct address
        {
            QString key;
            QString value;
        };
        
      public:
        enum AddressesRole
        {
            TypeRole = Qt::UserRole + 1,
            KeyRole,
            AddressRole
        };
        Q_ENUMS(ContactRoles)
    
        explicit addressbook_contact_addresses_model(ag::ecs::system_manager& system_manager, const QString& name, QString type, QObject* parent = nullptr);
        ~addressbook_contact_addresses_model() noexcept final = default;

        /// QAbstractListModel functions
        ////////////////////////////////
    
        [[nodiscard]] QVariant               data(const QModelIndex& index, int role) const final;
        bool                                 setData(const QModelIndex& index, const QVariant& value, int role) final;
        [[nodiscard]] int                    columnCount(const QModelIndex &parent = QModelIndex()) const final;
        [[nodiscard]] int                    rowCount([[maybe_unused]] const QModelIndex& parent = QModelIndex()) const final;
        [[nodiscard]] QHash<int, QByteArray> roleNames() const final;
    
        /////////////////////////////
    
        /// QML API
        ///////////
        
        Q_INVOKABLE bool add_address_entry(QString key, QString value);

        Q_INVOKABLE void remove_address_entry(int row);

        Q_PROPERTY(QString type READ get_type NOTIFY typeChanged)
        [[nodiscard]]
        Q_INVOKABLE const QString& get_type() const noexcept;
        
      signals:
        void typeChanged();
    
        ///////////
        
      public:
        // Loads this model data from the persistent data.
        void populate();
        
        // Unloads this model data.
        void clear();
        
        // Saves this model data to the persistent data.
        void save();
        
      private:
        ag::ecs::system_manager& m_system_manager;
    
        // Name of the contact.
        const QString& m_name;
        
        // Type of the wallet info (e.g. BTC, ERC-20).
        QString m_type{"KMD"};
        
        // Array of addresses. First value is key, second is value.
        QVector<address> m_model_data;
    };
}