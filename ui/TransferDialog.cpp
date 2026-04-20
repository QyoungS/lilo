#include "TransferDialog.h"
#include "TransferManager.h"
#include "AccountModel.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

TransferDialog::TransferDialog(int userId, QWidget* parent)
    : QDialog(parent), m_userId(userId)
{
    setWindowTitle("계좌 이체");
    setMinimumWidth(380);

    AccountModel model;
    model.loadAccounts(userId);

    auto* form = new QFormLayout(this);

    m_fromCombo = new QComboBox(this);
    m_toCombo   = new QComboBox(this);
    for (const Account& a : model.accounts()) {
        QString label = QString("%1 (%2) — %3 %4")
            .arg(a.name, a.number, a.currency).arg(a.balance, 0, 'f', 2);
        m_fromCombo->addItem(label, a.id);
        m_toCombo->addItem(label, a.id);
    }

    m_amtSpin = new QDoubleSpinBox(this);
    m_amtSpin->setRange(0.01, 1e12);
    m_amtSpin->setDecimals(2);
    m_amtSpin->setSingleStep(1000);

    m_memoEdit = new QLineEdit(this);
    m_memoEdit->setPlaceholderText("메모 (선택사항)...");

    form->addRow("출금 계좌:", m_fromCombo);
    form->addRow("입금 계좌:", m_toCombo);
    form->addRow("금액:",      m_amtSpin);
    form->addRow("메모:",      m_memoEdit);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton("이체", this);
    auto* can  = new QPushButton("취소", this);
    ok->setDefault(true);
    btns->addWidget(ok);
    btns->addWidget(can);
    form->addRow(btns);

    connect(ok,  &QPushButton::clicked, this, &TransferDialog::onTransfer);
    connect(can, &QPushButton::clicked, this, &QDialog::reject);
}

void TransferDialog::onTransfer() {
    int fromId = m_fromCombo->currentData().toInt();
    int toId   = m_toCombo->currentData().toInt();
    double amt = m_amtSpin->value();

    if (fromId == toId) {
        QMessageBox::warning(this, "오류", "출금 계좌와 입금 계좌는 달라야 합니다.");
        return;
    }
    if (!TransferManager::instance().transfer(fromId, toId, amt, m_memoEdit->text())) {
        QMessageBox::warning(this, "오류", "이체에 실패했습니다. 잔액을 확인하세요.");
        return;
    }
    QMessageBox::information(this, "성공", "이체가 완료되었습니다.");
    accept();
}
