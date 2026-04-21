#pragma once
#include <QWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QLabel>
#include <QFrame>
#include "AccountModel.h"

class AccountWidget : public QWidget {
    Q_OBJECT
public:
    explicit AccountWidget(int userId, QWidget* parent = nullptr);
    void refresh();

public slots:
    void onDeposit();
    void onWithdraw();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onAdd();

private:
    void setupUi();
    void buildCards();
    void buildList();
    void setSelectedAccount(int id);
    void doDeposit(int accountId);
    void doWithdraw(int accountId);
    void doEdit(int accountId);
    void doDelete(int accountId);
    void doTransfer(int accountId);
    void showAmountDialog(int accountId, bool isDeposit);

    int           m_userId;
    int           m_selectedId = -1;
    bool          m_cardMode   = true;
    AccountModel* m_model      = nullptr;

    QLabel*         m_totalLabel    = nullptr;
    QStackedWidget* m_stack         = nullptr;
    QScrollArea*    m_cardScroll    = nullptr;
    QScrollArea*    m_listScroll    = nullptr;
    QPushButton*    m_cardModeBtn   = nullptr;
    QPushButton*    m_listModeBtn   = nullptr;

    QList<QFrame*>  m_cardFrames;
    QList<QFrame*>  m_listFrames;
};
