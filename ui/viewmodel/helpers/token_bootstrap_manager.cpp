// Copyright 2019 The UFO Team
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

#include "token_bootstrap_manager.h"

#include "model/app_model.h"

TokenBootstrapManager::TokenBootstrapManager()
    : _wallet_model(*AppModel::getInstance().getWallet())
{
    connect(
        &_wallet_model,
        SIGNAL(txStatus(ufo::wallet::ChangeAction,
                        const std::vector<ufo::wallet::TxDescription>&)),
        SLOT(onTxStatus(ufo::wallet::ChangeAction,
                        const std::vector<ufo::wallet::TxDescription>&)));
    _wallet_model.getAsync()->getWalletStatus();
}

TokenBootstrapManager::~TokenBootstrapManager() {}

void TokenBootstrapManager::onTxStatus(
    ufo::wallet::ChangeAction action,
    const std::vector<ufo::wallet::TxDescription>& items)
{
    if (action != ufo::wallet::ChangeAction::Reset)
    {
        _wallet_model.getAsync()->getWalletStatus();
        return;   
    }

    _myTxIds.clear();
    _myTxIds.reserve(items.size());
    for (const auto& item : items)
    {
        const auto& txId = item.GetTxID();
        if (txId)
        {
            _myTxIds.emplace_back(txId.value());
        }
    }

    checkIsTxPreviousAccepted();
}

void TokenBootstrapManager::checkTokenForDuplicate(const QString& token)
{
    auto parameters = ufo::wallet::ParseParameters(token.toStdString());
    if (!parameters)
    {
        LOG_ERROR() << "Can't parse token params";
        return;
    }

    auto parametrsValue = parameters.value();
    auto peerID = parametrsValue.GetParameter<ufo::wallet::WalletID>(
        ufo::wallet::TxParameterID::PeerID);
    if (peerID && _wallet_model.isOwnAddress(*peerID))
    {
        emit tokenOwnGenerated(token);
        return;
    }

    auto txId = parametrsValue.GetTxID();
    if (!txId)
    {
        LOG_ERROR() << "Empty tx id in txParams";
        return;
    }
    auto txIdValue = txId.value();
    _tokensInProgress[txIdValue] = token;

    _myTxIds.empty()
        ? _wallet_model.getAsync()->getWalletStatus()
        : checkIsTxPreviousAccepted();
}

void TokenBootstrapManager::checkIsTxPreviousAccepted()
{
    if (!_tokensInProgress.empty())
    {
        for (const auto& txId : _myTxIds)
        {
            const auto& it = _tokensInProgress.find(txId);
            if (it != _tokensInProgress.end())
            {
                emit tokenPreviousAccepted(it->second);
                _tokensInProgress.erase(it);
            }
        }

        for (const auto& token : _tokensInProgress)
        {
            emit tokenFirstTimeAccepted(token.second);
        }
        _tokensInProgress.clear();
    }
}
