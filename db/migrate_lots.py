exec(__import__('pathlib').Path(__file__).with_name('migrate_operations.py').read_text().split('for table,column,definition')[0])
for column in ['batch_enabled','expiry_enabled']:
 if sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='products' AND column_name='{column}'").strip()=='0':sql(f'ALTER TABLE products ADD COLUMN {column} TINYINT NOT NULL DEFAULT 0')
sql('''CREATE TABLE IF NOT EXISTS inventory_lots(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,product_id BIGINT UNSIGNED NOT NULL,warehouse_id BIGINT UNSIGNED NOT NULL,lot_no VARCHAR(64) NOT NULL,manufactured_on DATE NULL,expires_on DATE NULL,quantity INT NOT NULL DEFAULT 0,created_by BIGINT UNSIGNED NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,UNIQUE KEY uk_lot(product_id,warehouse_id,lot_no)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS lot_movements(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,lot_id BIGINT UNSIGNED NOT NULL,change_qty INT NOT NULL,biz_type VARCHAR(32) NOT NULL,biz_no VARCHAR(64) NOT NULL,operator_id BIGINT UNSIGNED NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,KEY idx_lot_history(lot_id,id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;''')
print('Product batch and expiry schema ready')

if sql("SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='stock_operations' AND column_name='lot_details'").strip()=='0':sql('ALTER TABLE stock_operations ADD COLUMN lot_details JSON NULL')
