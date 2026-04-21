#pragma once
#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

class TransferDialog : public QDialog {
    Q_OBJECT
public:
    explicit TransferDialog(int userId, QWidget* parent = nullptr);
    void setFromAccount(int accountId);

private slots:
    void onTransfer();

private:
    int           m_userId;
    QComboBox*    m_fromCombo;
    QComboBox*    m_toCombo;
    QDoubleSpinBox* m_amtSpin;
    QLineEdit*    m_memoEdit;
};
