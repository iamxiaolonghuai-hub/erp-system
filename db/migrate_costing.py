exec(__import__('pathlib').Path(__file__).with_name('migrate_operations.py').read_text().split('for table,column,definition')[0])
for table,column,definition in [('inventory','cost_initialized','TINYINT NOT NULL DEFAULT 0'),('inventory','cost_value','DECIMAL(26,6) NOT NULL DEFAULT 0'),('stock_records','cost_value','DECIMAL(26,6) NULL')]:
 if sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='{column}'").strip()=='0':sql(f'ALTER TABLE {table} ADD COLUMN {column} {definition}')
sql('UPDATE inventory SET cost_initialized=1 WHERE quantity=0 AND cost_value=0')
print('Moving-average cost fields ready; positive historical inventory remains uninitialized')

sql("CREATE TABLE IF NOT EXISTS inventory_cost_openings(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,inventory_id BIGINT UNSIGNED NOT NULL,book_qty INT NOT NULL,unit_cost DECIMAL(18,6) NOT NULL,total_value DECIMAL(26,6) NOT NULL,baseline_record_id BIGINT UNSIGNED NOT NULL,operator_id BIGINT UNSIGNED NOT NULL,reason VARCHAR(255) NOT NULL,created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),KEY idx_opening_inventory(inventory_id,id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
