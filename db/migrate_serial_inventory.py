import json
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
db = json.loads((root / 'config.json').read_text())['db_clients'][0]
sql = """
CREATE TABLE IF NOT EXISTS inventory_serial_units (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    product_id BIGINT UNSIGNED NOT NULL,
    serial_no VARCHAR(128) COLLATE utf8mb4_bin NOT NULL,
    warehouse_id BIGINT UNSIGNED NULL,
    unit_state VARCHAR(16) NOT NULL,
    lot_no VARCHAR(64) NOT NULL DEFAULT '',
    manufactured_on DATE NULL,
    expires_on DATE NULL,
    purchase_no VARCHAR(64) NOT NULL DEFAULT '',
    sale_no VARCHAR(64) NOT NULL DEFAULT '',
    transfer_no VARCHAR(64) NOT NULL DEFAULT '',
    UNIQUE KEY uq_serial_product (product_id,serial_no),
    KEY idx_serial_warehouse (product_id,warehouse_id,unit_state),
    KEY idx_serial_lookup (serial_no),
    FOREIGN KEY (product_id) REFERENCES products(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS inventory_serial_events (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    unit_id BIGINT UNSIGNED NOT NULL,
    warehouse_id BIGINT UNSIGNED NOT NULL,
    action VARCHAR(32) NOT NULL,
    biz_no VARCHAR(64) NOT NULL,
    operator_id BIGINT UNSIGNED NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    KEY idx_serial_events (unit_id,id),
    FOREIGN KEY (unit_id) REFERENCES inventory_serial_units(id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
"""
subprocess.run(
    ['mysql', '-h'+db['host'], '-P'+str(db['port']), '-u'+db['user'], db['dbname']],
    input=sql, text=True, env=dict(os.environ, MYSQL_PWD=db['passwd']), check=True,
)
print('serial inventory ledger ready')
