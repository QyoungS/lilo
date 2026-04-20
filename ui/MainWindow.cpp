#include "MainWindow.h"
#include "DashboardWidget.h"
#include "AccountWidget.h"
#include "TransactionWidget.h"
#include "TransferDialog.h"
#include "BudgetWidget.h"
#include "SettingsDialog.h"
#include "AuthManager.h"
#include "NotificationManager.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>

MainWindow::MainWindow(int userId, const QString& username, QWidget* parent)
    : QMainWindow(parent), m_userId(userId), m_username(username)
{
    setWindowTitle("계정 관리자");
    setMinimumSize(1100, 700);

    NotificationManager::instance().initialize(this);

    setupTabs();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();

    // 저장된 사용자 테마 우선 로드, 없으면 기본 라이트 테마
    QString saved = SettingsDialog::loadSavedQss();
    if (!saved.isEmpty())
        qApp->setStyleSheet(saved);
    else
        applyTheme(false);
}

void MainWindow::setupTabs() {
    m_tabs = new QTabWidget(this);

    m_dashboard    = new DashboardWidget(m_userId, this);
    m_accounts     = new AccountWidget(m_userId, this);
    m_transactions = new TransactionWidget(m_userId, this);
    m_budgets      = new BudgetWidget(m_userId, this);

    m_tabs->addTab(m_dashboard,    "대시보드");
    m_tabs->addTab(m_accounts,     "계좌");
    m_tabs->addTab(m_transactions, "거래");
    m_tabs->addTab(m_budgets,      "예산");

    setCentralWidget(m_tabs);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("파일(&F)");

    auto* transferAct = fileMenu->addAction("이체...");
    connect(transferAct, &QAction::triggered, this, [this]() {
        TransferDialog dlg(m_userId, this);
        dlg.exec();
    });

    fileMenu->addSeparator();

    auto* logoutAct = fileMenu->addAction("로그아웃");
    connect(logoutAct, &QAction::triggered, this, &MainWindow::onLogout);

    auto* exitAct = fileMenu->addAction("종료");
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* viewMenu = menuBar()->addMenu("보기(&V)");
    auto* darkAct  = viewMenu->addAction("다크 모드");
    darkAct->setCheckable(true);
    connect(darkAct, &QAction::toggled, this, &MainWindow::onToggleDarkMode);

    viewMenu->addSeparator();
    auto* themeAct = viewMenu->addAction("테마 및 스타일 편집기...");
    connect(themeAct, &QAction::triggered, this, [this]() {
        SettingsDialog dlg(this);
        dlg.exec();
    });

    auto* helpMenu = menuBar()->addMenu("도움말(&H)");
    auto* aboutAct = helpMenu->addAction("정보");
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "정보", "계정 관리자 v1.0\n과제 프로젝트 — Qt6 / C++17");
    });
}

void MainWindow::setupToolBar() {
    auto* tb = addToolBar("메인");
    tb->setMovable(false);

    // 입금 버튼
    auto* depositBtn = new QPushButton("입금", tb);
    depositBtn->setStyleSheet(
        "QPushButton { background:#F0FDF4; color:#059669; border:1px solid #A7F3D0; "
        "border-radius:6px; padding:4px 14px; font-weight:600; font-size:9pt; min-height:28px; }"
        "QPushButton:hover { background:#059669; color:#FFFFFF; border-color:#059669; }");
    connect(depositBtn, &QPushButton::clicked, this, [this]() {
        m_tabs->setCurrentWidget(m_accounts);
        m_accounts->onDeposit();
    });
    tb->addWidget(depositBtn);

    // 출금 버튼
    auto* withdrawBtn = new QPushButton("출금", tb);
    withdrawBtn->setStyleSheet(
        "QPushButton { background:#FFFBEB; color:#D97706; border:1px solid #FDE68A; "
        "border-radius:6px; padding:4px 14px; font-weight:600; font-size:9pt; min-height:28px; }"
        "QPushButton:hover { background:#D97706; color:#FFFFFF; border-color:#D97706; }");
    connect(withdrawBtn, &QPushButton::clicked, this, [this]() {
        m_tabs->setCurrentWidget(m_accounts);
        m_accounts->onWithdraw();
    });
    tb->addWidget(withdrawBtn);

    tb->addSeparator();

    auto* transferAct = tb->addAction("이체");
    connect(transferAct, &QAction::triggered, this, [this]() {
        TransferDialog dlg(m_userId, this);
        dlg.exec();
    });

    tb->addSeparator();

    auto* refreshAct = tb->addAction("새로 고침");
    connect(refreshAct, &QAction::triggered, this, [this]() {
        m_dashboard->refresh();
        m_accounts->refresh();
        m_transactions->refresh();
        m_budgets->refresh();
    });
}

void MainWindow::setupStatusBar() {
    m_statusUser = new QLabel(QString("  로그인: %1  ").arg(m_username));
    statusBar()->addPermanentWidget(m_statusUser);
    statusBar()->showMessage("준비");
}

void MainWindow::onToggleDarkMode(bool enabled) {
    m_darkMode = enabled;
    applyTheme(enabled);
}

void MainWindow::applyTheme(bool dark) {
    QFile f(dark ? ":/styles/dark.qss" : ":/styles/light.qss");
    if (f.open(QFile::ReadOnly))
        qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    else
        qApp->setStyleSheet("");
}

void MainWindow::onLogout() {
    AuthManager::instance().logout();
    qApp->setStyleSheet("");
    hide();
    QApplication::exit(0);
}
