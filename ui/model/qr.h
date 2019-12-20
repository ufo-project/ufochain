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
// limitations under the License

#pragma once

#include <QObject>
#include <QtCore>
#include "utility/common.h"

class QR : public QObject {
    Q_OBJECT
public:
    QR();
    QR(const QString& addr,
       uint width = 200,
       uint height = 200,
       ufo::Amount amount = 0);
       ~QR();
    void setAmount(ufo::Amount amount);
    void setAddr(const QString& addr);
    void setDimensions(uint width, uint height);

    QString getEncoded() const;

signals:
    void qrDataChanged();

private:
    void update();

    QString m_addr;
    uint m_width = 200;
    uint m_height = 200;
    ufo::Amount m_amountGrothes = 0;
    QString m_qrData;
};
