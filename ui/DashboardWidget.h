#pragma once
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QGroupBox>

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(int userId, QWidget* parent = nullptr);
    void refresh();

private:
    void setupUi();
    void setupCharts();
    void updateSummaryCards();
    void updateBarChart();
    void updatePieChart();
    void fetchExchangeRates();
    void updateExchangeRateCard(const QString& code, double krwPerUnit,
                                double krwPer100Jpy = -1);

    int     m_userId;
    QString m_username;

    // 요약 카드
    QLabel* m_totalBalanceLabel;
    QLabel* m_monthIncomeLabel;
    QLabel* m_monthExpenseLabel;

    // 환율 카드 레이블
    QLabel* m_usdLabel;
    QLabel* m_jpyLabel;
    QLabel* m_eurLabel;
    QLabel* m_cnyLabel;
    QLabel* m_rateUpdateLabel;

    // 차트
    QWidget* m_barChartView;
    QWidget* m_pieChartView;

    // 자동 갱신 타이머 (5분)
    QTimer* m_rateTimer;
};
