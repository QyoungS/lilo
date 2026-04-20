#include "LoginWindow.h"
#include "MainWindow.h"
#include "AuthManager.h"
#include "ValidationHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QApplication>

LoginWindow::LoginWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("계정 관리자 — 로그인");
    setMinimumWidth(360);
    setModal(true);
    setupUi();
}

void LoginWindow::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(16);
    root->setContentsMargins(32, 32, 32, 32);

    auto* title = new QLabel("<h2>계정 관리자</h2>", this);
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* form = new QFormLayout;
    m_userEdit = new QLineEdit(this);
    m_passEdit = new QLineEdit(this);
    m_passEdit->setEchoMode(QLineEdit::Password);
    form->addRow("사용자명:", m_userEdit);
    form->addRow("비밀번호:", m_passEdit);
    root->addLayout(form);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();
    root->addWidget(m_errorLabel);

    auto* btnLayout = new QHBoxLayout;
    m_loginBtn    = new QPushButton("로그인", this);
    m_registerBtn = new QPushButton("회원가입", this);
    m_loginBtn->setDefault(true);
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    root->addLayout(btnLayout);

    connect(m_loginBtn,    &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegister);
    connect(m_passEdit, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
}

void LoginWindow::onLogin() {
    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text();

    if (!ValidationHelper::isNotEmpty(user) || !ValidationHelper::isNotEmpty(pass)) {
        m_errorLabel->setText("사용자명과 비밀번호를 입력하세요.");
        m_errorLabel->show();
        return;
    }

    if (!AuthManager::instance().login(user, pass)) {
        m_errorLabel->setText("사용자명 또는 비밀번호가 올바르지 않습니다.");
        m_errorLabel->show();
        m_passEdit->clear();
        return;
    }

    accept();
}

void LoginWindow::onRegister() {
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("회원가입");
    dlg->setMinimumWidth(320);

    auto* layout = new QFormLayout(dlg);
    auto* uEdit  = new QLineEdit(dlg);
    auto* pEdit  = new QLineEdit(dlg);
    auto* p2Edit = new QLineEdit(dlg);
    pEdit->setEchoMode(QLineEdit::Password);
    p2Edit->setEchoMode(QLineEdit::Password);
    layout->addRow("사용자명:", uEdit);
    layout->addRow("비밀번호:", pEdit);
    layout->addRow("비밀번호 확인:", p2Edit);

    auto* btns   = new QHBoxLayout;
    auto* ok     = new QPushButton("회원가입", dlg);
    auto* cancel = new QPushButton("취소", dlg);
    btns->addWidget(ok);
    btns->addWidget(cancel);
    layout->addRow(btns);

    connect(cancel, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(ok, &QPushButton::clicked, dlg, [=]() {
        QString u  = uEdit->text().trimmed();
        QString p  = pEdit->text();
        QString p2 = p2Edit->text();

        if (!ValidationHelper::isValidUsername(u)) {
            QMessageBox::warning(dlg, "오류", "사용자명은 3~30자의 영문/숫자여야 합니다.");
            return;
        }
        if (!ValidationHelper::isValidPassword(p)) {
            QMessageBox::warning(dlg, "오류", "비밀번호는 6자 이상이어야 합니다.");
            return;
        }
        if (p != p2) {
            QMessageBox::warning(dlg, "오류", "비밀번호가 일치하지 않습니다.");
            return;
        }
        if (!AuthManager::instance().registerUser(u, p)) {
            QMessageBox::warning(dlg, "오류", "이미 사용 중인 사용자명입니다.");
            return;
        }
        QMessageBox::information(dlg, "성공", "계정이 생성되었습니다. 로그인하세요.");
        dlg->accept();
    });

    dlg->exec();
    dlg->deleteLater();
}
