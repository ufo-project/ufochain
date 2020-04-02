// Copyright 2018 The Ufo Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <QObject>
#include <QQmlListProperty>
#include "wallet/bitcoin/client.h"
#include "model/wallet_model.h"
#include "model/settings.h"
#include "viewmodel/messages_view.h"
#include "tx_object_list.h"

class WalletViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ufoAvailable                 READ ufoAvailable              NOTIFY ufoAvailableChanged)
    Q_PROPERTY(QString ufoReceiving                 READ ufoReceiving              NOTIFY ufoReceivingChanged)
    Q_PROPERTY(QString ufoSending                   READ ufoSending                NOTIFY ufoSendingChanged)
    Q_PROPERTY(QString ufoLocked                    READ ufoLocked                 NOTIFY ufoLockedChanged)
    Q_PROPERTY(QString ufoLockedMaturing            READ ufoLockedMaturing         NOTIFY ufoLockedChanged)
    Q_PROPERTY(QString ufoReceivingChange           READ ufoReceivingChange        NOTIFY ufoReceivingChanged)
    Q_PROPERTY(QString ufoReceivingIncoming         READ ufoReceivingIncoming      NOTIFY ufoReceivingChanged)
    Q_PROPERTY(bool isAllowedUfoMWLinks            READ isAllowedUfoMWLinks       WRITE allowUfoMWLinks      NOTIFY ufoMWLinksAllowed)
    Q_PROPERTY(QAbstractItemModel* transactions     READ getTransactions            NOTIFY transactionsChanged)

public:
    WalletViewModel();

    QString  ufoAvailable() const;
    QString  ufoReceiving() const;
    QString  ufoSending() const;
    QString  ufoLocked() const;
    QString  ufoLockedMaturing() const;
    QString  ufoReceivingChange() const;
    QString  ufoReceivingIncoming() const;

    QAbstractItemModel* getTransactions();
    bool getIsOfflineStatus() const;
    bool getIsFailedStatus() const;
    QString getWalletStatusErrorMsg() const;
    void allowUfoMWLinks(bool value);

    Q_INVOKABLE void cancelTx(QVariant variantTxID);
    Q_INVOKABLE void deleteTx(QVariant variantTxID);
    Q_INVOKABLE PaymentInfoItem* getPaymentInfo(QVariant variantTxID);
    Q_INVOKABLE bool isAllowedUfoMWLinks() const;
	Q_INVOKABLE bool saveToFile(const QString str, QString path);

public slots:
    void onTxStatus(ufo::wallet::ChangeAction action, const std::vector<ufo::wallet::TxDescription>& items);

signals:
    void ufoAvailableChanged();
    void ufoReceivingChanged();
    void ufoSendingChanged();
    void ufoLockedChanged();

    void transactionsChanged();
    void ufoMWLinksAllowed();

private:
    WalletModel& _model;
    WalletSettings& _settings;
    TxObjectList _transactionsList;
};
