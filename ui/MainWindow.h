#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>

class DashboardWidget;
class AccountWidget;
class TransactionWidget;
class TransferDialog;
class BudgetWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(int userId, const QString& username, QWidget* parent = nullptr);

private slots:
    void onToggleDarkMode(bool enabled);
    void onLogout();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupTabs();
    void applyTheme(bool dark);

    int     m_userId;
    QString m_username;

    QTabWidget*       m_tabs;
    DashboardWidget*  m_dashboard;
    AccountWidget*    m_accounts;
    TransactionWidget* m_transactions;
    BudgetWidget*     m_budgets;

    QLabel* m_statusUser;
    bool    m_darkMode = false;
};
