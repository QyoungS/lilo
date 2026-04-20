#include "ValidationHelper.h"
#include <QRegularExpression>

bool ValidationHelper::isNotEmpty(const QString& s) {
    return !s.trimmed().isEmpty();
}

bool ValidationHelper::isValidAmount(double amount) {
    return amount > 0.0;
}

bool ValidationHelper::isValidAccountNumber(const QString& number) {
    static QRegularExpression re("^[0-9\\-]{6,20}$");
    return re.match(number.trimmed()).hasMatch();
}

bool ValidationHelper::isValidUsername(const QString& username) {
    static QRegularExpression re("^[a-zA-Z0-9_]{3,30}$");
    return re.match(username.trimmed()).hasMatch();
}

bool ValidationHelper::isValidPassword(const QString& password) {
    return password.length() >= 6;
}
