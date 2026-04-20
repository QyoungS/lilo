#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QDialog {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void onLogin();
    void onRegister();

private:
    void setupUi();

    QLineEdit*   m_userEdit;
    QLineEdit*   m_passEdit;
    QPushButton* m_loginBtn;
    QPushButton* m_registerBtn;
    QLabel*      m_errorLabel;
};
