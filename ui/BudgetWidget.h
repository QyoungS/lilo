#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

class BudgetWidget : public QWidget {
    Q_OBJECT
public:
    explicit BudgetWidget(int userId, QWidget* parent = nullptr);
    void refresh();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();

private:
    void setupUi();
    void loadBudgets();

    int           m_userId;
    QTableWidget* m_table;
    QPushButton*  m_editBtn;
    QPushButton*  m_deleteBtn;

    QLabel* m_totalBudgetLabel  = nullptr;
    QLabel* m_totalSpentLabel   = nullptr;
    QLabel* m_remainingLabel    = nullptr;
};
