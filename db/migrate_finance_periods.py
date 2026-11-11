import json
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
db = json.loads((root / 'config.json').read_text())['db_clients'][0]
sql = """
CREATE TABLE IF NOT EXISTS finance_period_control (
    id INT PRIMARY KEY,
    first_day DATE NOT NULL,
    closed_through DATE NULL
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS finance_period_closures (
    period_start DATE PRIMARY KEY,
    period_end DATE NOT NULL UNIQUE,
    closed_by BIGINT UNSIGNED NOT NULL,
    closed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS finance_postings (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    account_id BIGINT UNSIGNED NOT NULL,
    account_type VARCHAR(16) NOT NULL,
    partner_id BIGINT UNSIGNED NOT NULL,
    partner_name VARCHAR(255) NOT NULL,
    biz_no VARCHAR(64) NOT NULL,
    source_kind VARCHAR(24) NOT NULL,
    source_no VARCHAR(64) NOT NULL,
    posting_day DATE NOT NULL,
    amount_change DECIMAL(18,2) NOT NULL,
    cash_change DECIMAL(18,2) NOT NULL,
    offset_change DECIMAL(18,2) NOT NULL,
    KEY idx_finance_day (posting_day, account_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

START TRANSACTION;
INSERT IGNORE INTO finance_period_control(id,first_day) VALUES (1,CURRENT_DATE);
SET @initialize := ROW_COUNT();
INSERT INTO finance_postings(account_id,account_type,partner_id,partner_name,biz_no,source_kind,
                            source_no,posting_day,amount_change,cash_change,offset_change)
SELECT id,account_type,partner_id,partner_name,biz_no,'opening',biz_no,
       DATE_SUB(CURRENT_DATE,INTERVAL 1 DAY),amount,paid_amount,offset_amount
FROM accounts WHERE @initialize = 1;
COMMIT;
"""
subprocess.run(
    ['mysql', '-h'+db['host'], '-P'+str(db['port']), '-u'+db['user'], db['dbname']],
    input=sql, text=True, env=dict(os.environ, MYSQL_PWD=db['passwd']), check=True,
)
print('finance period ledger ready')
