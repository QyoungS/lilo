# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**AccountManager (계정 관리자)** — Qt6/C++17 personal finance desktop application with SQLite persistence. Korean-language UI. Built with Qt Creator / CMake targeting Windows.

## Build Commands

```bash
# Configure (Debug)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --config Debug

# Run (adjust path to match your Qt kit folder)
./build/Desktop_Qt_6_11_0_MinGW_64_bit-Debug/AccountManager.exe
```

Qt components required: `Qt6::Widgets Qt6::Sql Qt6::Charts Qt6::Network`

There are no tests. There is no lint step. `CMAKE_AUTOMOC` and `CMAKE_AUTORCC` are ON — no manual moc/rcc invocations needed.

## Architecture

Three-tier layout under separate directories, all compiled as a flat executable:

```
data/    — Qt table models + DatabaseManager singleton
logic/   — Business logic singletons (one per domain)
ui/      — QWidget subclasses (one per screen/dialog)
```

All include directories are flat (`-I data -I logic -I ui`), so `#include "Foo.h"` works from anywhere.

### Data Tier

**`DatabaseManager`** (singleton) — opens/creates `%APPDATA%\CourseProject\account_manager.db` (SQLite, WAL mode, FK constraints). Owns the `QSqlDatabase` connection. All other classes call `DatabaseManager::instance().database()` to get a `QSqlQuery`-ready handle.

Schema: `users`, `accounts`, `transactions`, `transfers`, `budgets`, `audit_log`. All child tables cascade-delete from their parent.

**`AccountModel` / `TransactionModel`** — `QAbstractTableModel` subclasses used directly in `QTableView`. `AccountModel::formatKRW(double)` is the shared KRW formatter (₩1,234,567).

### Logic Tier — All Singletons

| Class | Responsibility |
|---|---|
| `AuthManager` | Login/register (SHA-256 passwords), session state (`currentUserId`, `currentUsername`) |
| `AccountManager` | Account CRUD; emits `accountsChanged()` |
| `TransactionManager` | Deposit/withdraw/edit/delete; calls `recalculateBalance()` on every edit/delete to keep `accounts.balance` consistent; emits `transactionsChanged(accountId)` |
| `TransferManager` | Atomic 5-step transfer (2 balance updates + 1 transfer record + 2 transaction records); emits `transferCompleted(fromId, toId)` |
| `SearchEngine` | Builds dynamic SQL WHERE clause from `SearchCriteria` struct; returns `QList<Transaction>` |
| `NotificationManager` | Checks budgets vs. actual spending; fires system-tray warnings at 80 % and 100 % |
| `ValidationHelper` | Static-only helpers (no instance): amount, account number, username, password |

### UI Tier

`main.cpp` runs a `while(true)` restart loop — `LoginWindow::exec()` blocks until login, then `MainWindow` is shown and `app.exec()` runs. On logout, `MainWindow` calls `QApplication::exit(0)` which breaks `app.exec()` with code 0, causing the loop to re-show `LoginWindow`.

`MainWindow` hosts four `QTabWidget` tabs: `DashboardWidget`, `AccountWidget`, `TransactionWidget`, `BudgetWidget`. Menu bar provides Transfer dialog, logout, and `SettingsDialog` (QSS theme editor).

`DashboardWidget` fetches live exchange rates from `https://open.er-api.com/v6/latest/KRW` (KRW-denominated, so rates are inverted: `1.0 / rates[code]`). A `QTimer` refreshes every 5 minutes.

## Key Conventions

- **Korean string literals** throughout: transaction types are the string literals `"입금"` (deposit) and `"출금"` (withdrawal). Searching/comparing these must use the exact Korean strings.
- **Balance integrity**: `accounts.balance` is authoritative. `TransactionManager::recalculateBalance()` recomputes it from the full transaction history after any edit or delete. Deposits/withdrawals use `balance ± amount` directly for performance.
- **DB transactions**: every multi-step write uses `db.transaction()` / `db.commit()` / `db.rollback()`. Always follow this pattern for new operations.
- **Signal propagation**: UI widgets listen to manager signals for refresh — don't call `refresh()` directly after a manager call; let the signal trigger it.

## Incomplete / Known Issues

- **`audit_log` table** — created but never written to. All audit trail functionality is missing.
- **Password hashing** — single-pass SHA-256 with no salt. Vulnerable to rainbow tables.
- **`updateTransaction` balance bug** — when editing a transaction, `recalculateBalance()` runs inside the same DB transaction but after the UPDATE, which is correct. However, the balance is *not* pre-checked for withdrawals going negative — no guard exists on edits.
- **CSV export** (`TransactionWidget::onExportCsv`) — slot is declared and connected but the implementation needs to be verified; the file-write logic may be a stub.
- **`SettingsDialog` QSS persistence** — `loadSavedQss()` / `saveQss()` exist but the call site in `MainWindow` startup is not confirmed; custom themes may not survive app restart.
- **Exchange rate network leak** — `DashboardWidget::fetchExchangeRates()` creates a new `QNetworkAccessManager` on every call (parent-owned, so eventually collected, but wasteful). Should reuse a member `QNetworkAccessManager`.
