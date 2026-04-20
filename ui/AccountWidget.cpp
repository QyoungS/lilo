#include "AccountWidget.h"
#include "AccountManager.h"
#include "TransactionManager.h"
#include "ValidationHelper.h"
#include "AccountModel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QWidget>

AccountWidget::AccountWidget(int userId, QWidget* parent)
    : QWidget(parent), m_userId(userId)
{
    setupUi();
    refresh();

    connect(&AccountManager::instance(), &AccountManager::accountsChanged,
            this, &AccountWidget::refresh);
    connect(&TransactionManager::instance(), &TransactionManager::transactionsChanged,
            this, [this](int) { refresh(); });
}

void AccountWidget::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    m_model = new AccountModel(this);
    m_view  = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->verticalHeader()->hide();
    m_view->setAlternatingRowColors(true);
    root->addWidget(m_view);

    auto* btns    = new QHBoxLayout;
    auto* addBtn  = new QPushButton("+ 계좌 추가", this);
    m_editBtn     = new QPushButton("수정",   this);
    m_deleteBtn   = new QPushButton("삭제",   this);
    m_depositBtn  = new QPushButton("입금",   this);
    m_withdrawBtn = new QPushButton("출금",   this);

    btns->addWidget(addBtn);
    btns->addWidget(m_editBtn);
    btns->addWidget(m_deleteBtn);
    btns->addStretch();
    btns->addWidget(m_depositBtn);
    btns->addWidget(m_withdrawBtn);
    root->addLayout(btns);

    connect(addBtn,        &QPushButton::clicked, this, &AccountWidget::onAdd);
    connect(m_editBtn,     &QPushButton::clicked, this, &AccountWidget::onEdit);
    connect(m_deleteBtn,   &QPushButton::clicked, this, &AccountWidget::onDelete);
    connect(m_depositBtn,  &QPushButton::clicked, this, &AccountWidget::onDeposit);
    connect(m_withdrawBtn, &QPushButton::clicked, this, &AccountWidget::onWithdraw);
}

void AccountWidget::refresh() {
    m_model->loadAccounts(m_userId);
    m_view->resizeColumnsToContents();
    m_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
}

int AccountWidget::selectedAccountId() {
    auto idx = m_view->selectionModel()->currentIndex();
    if (!idx.isValid()) return -1;
    return m_model->accountAt(idx.row()).id;
}

void AccountWidget::onAdd() {
    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("계좌 추가");
    dlg->setMinimumWidth(340);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    auto* nameEdit = new QLineEdit(dlg);
    nameEdit->setPlaceholderText("예: 주거래 통장");
    auto* numEdit  = new QLineEdit(dlg);
    numEdit->setPlaceholderText("예: 110-123-456789");
    auto* typeBox  = new QComboBox(dlg);
    typeBox->addItems({"입출금", "저축", "투자", "현금"});

    // 통화는 KRW 고정 — 선택 UI 제거
    auto* currencyLbl = new QLabel("₩ KRW (대한민국 원)", dlg);
    currencyLbl->setStyleSheet("color:#64748B; font-size:9pt;");

    form->addRow("계좌명:", nameEdit);
    form->addRow("계좌번호:", numEdit);
    form->addRow("유형:", typeBox);
    form->addRow("통화:", currencyLbl);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton("생성", dlg);
    auto* can  = new QPushButton("취소", dlg);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok,  &QPushButton::clicked, dlg, [=]() {
        if (!ValidationHelper::isNotEmpty(nameEdit->text())) {
            QMessageBox::warning(dlg, "오류", "계좌명을 입력하세요."); return;
        }
        if (!ValidationHelper::isValidAccountNumber(numEdit->text())) {
            QMessageBox::warning(dlg, "오류", "계좌번호는 6~20자리 숫자/하이픈이어야 합니다."); return;
        }
        if (!AccountManager::instance().createAccount(
                m_userId, nameEdit->text(), numEdit->text(), typeBox->currentText(), "KRW")) {
            QMessageBox::warning(dlg, "오류", "이미 존재하는 계좌번호입니다."); return;
        }
        dlg->accept();
    });
    dlg->exec();
    dlg->deleteLater();
}

void AccountWidget::onEdit() {
    int id = selectedAccountId();
    if (id < 0) { QMessageBox::information(this, "선택", "계좌를 선택하세요."); return; }

    Account a  = AccountManager::instance().getAccount(id);
    auto* dlg  = new QDialog(this);
    dlg->setWindowTitle("계좌 수정");
    dlg->setMinimumWidth(320);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    auto* nameEdit = new QLineEdit(a.name, dlg);
    auto* typeBox  = new QComboBox(dlg);
    typeBox->addItems({"입출금", "저축", "투자", "현금"});
    typeBox->setCurrentText(a.type);

    form->addRow("계좌명:", nameEdit);
    form->addRow("유형:", typeBox);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton("저장", dlg);
    auto* can  = new QPushButton("취소", dlg);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok,  &QPushButton::clicked, dlg, [=]() {
        if (!ValidationHelper::isNotEmpty(nameEdit->text())) {
            QMessageBox::warning(dlg, "오류", "계좌명을 입력하세요."); return;
        }
        AccountManager::instance().updateAccount(id, nameEdit->text(), typeBox->currentText(), "KRW");
        dlg->accept();
    });
    dlg->exec();
    dlg->deleteLater();
}

void AccountWidget::onDelete() {
    int id = selectedAccountId();
    if (id < 0) { QMessageBox::information(this, "선택", "계좌를 선택하세요."); return; }

    if (QMessageBox::question(this, "계좌 삭제",
            "이 계좌와 모든 거래 내역을 삭제하시겠습니까?\n이 작업은 되돌릴 수 없습니다.")
            == QMessageBox::Yes) {
        AccountManager::instance().deleteAccount(id);
    }
}

static void showAmountDialog(QWidget* parent, int accountId, bool isDeposit, int userId) {
    auto* dlg  = new QDialog(parent);
    dlg->setWindowTitle(isDeposit ? "입금" : "출금");
    dlg->setMinimumWidth(320);
    auto* form = new QFormLayout(dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);

    auto* amtSpin = new QDoubleSpinBox(dlg);
    amtSpin->setRange(1, 1e12);
    amtSpin->setDecimals(0);
    amtSpin->setSingleStep(10000);
    amtSpin->setGroupSeparatorShown(true);
    amtSpin->setPrefix("₩ ");

    auto* catBox = new QComboBox(dlg);
    catBox->addItems({"급여", "식비", "교통", "쇼핑", "의료", "여가", "이체", "기타"});
    auto* descEdit = new QLineEdit(dlg);
    descEdit->setPlaceholderText("거래 설명 (선택사항)");

    form->addRow("금액:", amtSpin);
    form->addRow("카테고리:", catBox);
    form->addRow("설명:", descEdit);

    auto* btns = new QHBoxLayout;
    auto* ok   = new QPushButton(isDeposit ? "입금" : "출금", dlg);
    auto* can  = new QPushButton("취소", dlg);
    btns->addStretch(); btns->addWidget(ok); btns->addWidget(can);
    form->addRow(btns);

    QObject::connect(can, &QPushButton::clicked, dlg, &QDialog::reject);
    QObject::connect(ok,  &QPushButton::clicked, dlg, [=]() {
        double amt = amtSpin->value();
        bool ok2 = isDeposit
            ? TransactionManager::instance().deposit(accountId, amt, catBox->currentText(), descEdit->text())
            : TransactionManager::instance().withdraw(accountId, amt, catBox->currentText(), descEdit->text());
        if (!ok2)
            QMessageBox::warning(dlg, "오류", isDeposit ? "입금에 실패했습니다." : "잔액이 부족합니다.");
        else
            dlg->accept();
    });
    dlg->exec();
    dlg->deleteLater();
}

void AccountWidget::onDeposit() {
    int id = selectedAccountId();
    if (id < 0) { QMessageBox::information(this, "선택", "계좌를 선택하세요."); return; }
    showAmountDialog(this, id, true, m_userId);
}

void AccountWidget::onWithdraw() {
    int id = selectedAccountId();
    if (id < 0) { QMessageBox::information(this, "선택", "계좌를 선택하세요."); return; }
    showAmountDialog(this, id, false, m_userId);
}
