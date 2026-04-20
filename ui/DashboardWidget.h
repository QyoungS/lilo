#pragma once
#include <QWidget>
#include <QLabel>

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

    int m_userId;

    QLabel* m_totalBalanceLabel;
    QLabel* m_monthIncomeLabel;
    QLabel* m_monthExpenseLabel;
    QLabel* m_ratesLabel;

    QWidget* m_barChartView;
    QWidget* m_pieChartView;
};
