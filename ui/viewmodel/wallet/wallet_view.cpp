// Copyright 2018 The UFO Team
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

#include "wallet_view.h"

#include <iomanip>

#include <QApplication>
#include <QtGui/qimage.h>
#include <QtCore/qbuffer.h>
#include <QUrlQuery>
#include <QClipboard>
#include <QDesktopServices>

#include "qrcode/QRCodeGenerator.h"
#include "utility/helpers.h"
#include "model/app_model.h"
#include "model/qr.h"
#include "viewmodel/ui_helpers.h"

using namespace ufo;
using namespace ufo::wallet;
using namespace std;
using namespace ufoui;

WalletViewModel::WalletViewModel()
    : _model(*AppModel::getInstance().getWallet())
    , _settings(AppModel::getInstance().getSettings())
{
    connect(&_model, SIGNAL(txStatus(ufo::wallet::ChangeAction, const std::vector<ufo::wallet::TxDescription>&)),
        SLOT(onTxStatus(ufo::wallet::ChangeAction, const std::vector<ufo::wallet::TxDescription>&)));

    connect(&_model, SIGNAL(availableChanged()), this, SIGNAL(ufoAvailableChanged()));
    connect(&_model, SIGNAL(receivingChanged()), this, SIGNAL(ufoReceivingChanged()));
    connect(&_model, SIGNAL(sendingChanged()), this, SIGNAL(ufoSendingChanged()));
    connect(&_model, SIGNAL(maturingChanged()), this, SIGNAL(ufoLockedChanged()));
    connect(&_model, SIGNAL(receivingChangeChanged()), this, SIGNAL(ufoReceivingChanged()));
    connect(&_model, SIGNAL(receivingIncomingChanged()), this, SIGNAL(ufoReceivingChanged()));

    // TODO: This also refreshes TXs and addresses. Need to make this more transparent
    _model.getAsync()->getWalletStatus();
}

QAbstractItemModel* WalletViewModel::getTransactions()
{
    return &_transactionsList;
}

void WalletViewModel::cancelTx(QVariant variantTxID)
{
    if (!variantTxID.isNull() && variantTxID.isValid())
    {
        auto txId = variantTxID.value<ufo::wallet::TxID>();
        _model.getAsync()->cancelTx(txId);
    }
}

void WalletViewModel::deleteTx(QVariant variantTxID)
{
    if (!variantTxID.isNull() && variantTxID.isValid())
    {
        auto txId = variantTxID.value<ufo::wallet::TxID>();
        _model.getAsync()->deleteTx(txId);
    }
}

PaymentInfoItem* WalletViewModel::getPaymentInfo(QVariant variantTxID)
{
    if (!variantTxID.isNull() && variantTxID.isValid())
    {
        auto txId = variantTxID.value<ufo::wallet::TxID>();
        return new MyPaymentInfoItem(txId, this);
    }
    else return Q_NULLPTR;
}

void WalletViewModel::onTxStatus(ufo::wallet::ChangeAction action, const std::vector<ufo::wallet::TxDescription>& transactions)
{
    vector<shared_ptr<TxObject>> modifiedTransactions;
    modifiedTransactions.reserve(transactions.size());

    for (const auto& t : transactions)
    {
        if (t.GetParameter<TxType>(TxParameterID::TransactionType) != TxType::AtomicSwap)
        {
            modifiedTransactions.push_back(make_shared<TxObject>(t));
        }
    }

    switch (action)
    {
        case ChangeAction::Reset:
            {
                _transactionsList.reset(modifiedTransactions);
                break;
            }

        case ChangeAction::Removed:
            {
                _transactionsList.remove(modifiedTransactions);
                break;
            }

        case ChangeAction::Added:
            {
                _transactionsList.insert(modifiedTransactions);
                break;
            }
        
        case ChangeAction::Updated:
            {
                _transactionsList.update(modifiedTransactions);
                break;
            }

        default:
            assert(false && "Unexpected action");
            break;
    }

    emit transactionsChanged();
}

QString WalletViewModel::ufoAvailable() const
{
    return ufoui::AmountToString(_model.getAvailable());
}

QString WalletViewModel::ufoReceiving() const
{
    // TODO:SWAP return real value
    return ufoui::AmountToString(_model.getReceivingChange() + _model.getReceivingIncoming());
}

QString WalletViewModel::ufoSending() const
{
    return ufoui::AmountToString(_model.getSending());
}

QString WalletViewModel::ufoReceivingChange() const
{
    // TODO:SWAP return real value
    return ufoui::AmountToString(_model.getReceivingChange());
}

QString WalletViewModel::ufoReceivingIncoming() const
{
    // TODO:SWAP return real value
    return ufoui::AmountToString(_model.getReceivingIncoming());
}

QString WalletViewModel::ufoLocked() const
{
    return ufoLockedMaturing();
}

QString WalletViewModel::ufoLockedMaturing() const
{
    return ufoui::AmountToString(_model.getMaturing());
}

bool WalletViewModel::isAllowedUfoMWLinks() const
{
    return _settings.isAllowedUfoMWLinks();
}

bool WalletViewModel::saveToFile(const QString str, QString path)
{
	if (path.startsWith("file:///")) path.remove(0, 8);
	QFile file(path);
	if (file.open(QFile::WriteOnly | QFile::Truncate)) {
		QTextStream out(&file);
		out << str;
		file.close();
		QDesktopServices::openUrl(QDir::currentPath());
		return true;
	}
	else {
		return false;
	}
}

void WalletViewModel::allowUfoMWLinks(bool value)
{
    _settings.setAllowedUfoMWLinks(value);
}
