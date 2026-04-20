#pragma once
#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include "AccountModel.h"

class AccountWidget : public QWidget {
    Q_OBJECT
public:
    explicit AccountWidget(int userId, QWidget* parent = nullptr);
    void refresh();

public slots:
    void onDeposit();
    void onWithdraw();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();

private:
    void setupUi();
    int  selectedAccountId();

    int           m_userId;
    QTableView*   m_view;
    AccountModel* m_model;
    QPushButton*  m_editBtn;
    QPushButton*  m_deleteBtn;
    QPushButton*  m_depositBtn;
    QPushButton*  m_withdrawBtn;
};
