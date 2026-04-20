#pragma once
#include <QString>

class ValidationHelper {
public:
    static bool isNotEmpty(const QString& s);
    static bool isValidAmount(double amount);
    static bool isValidAccountNumber(const QString& number);
    static bool isValidUsername(const QString& username);
    static bool isValidPassword(const QString& password);
};
