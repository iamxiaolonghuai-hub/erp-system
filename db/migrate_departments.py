"""Add stable department references without deleting historical records."""
from pathlib import Path
import sys
exec(Path(__file__).with_name('migrate_operations.py').read_text().split('for table,column,definition')[0])
if sql('SELECT COUNT(*) FROM (SELECT user_id FROM employees WHERE user_id IS NOT NULL GROUP BY user_id HAVING COUNT(*)>1) duplicate_accounts').strip()!='0':raise RuntimeError('Multiple employee profiles share an account; resolve duplicates before migration')
if '--check-only' in sys.argv:
 print('Department preflight passed; no data changed')
 raise SystemExit(0)
sql('''CREATE TABLE IF NOT EXISTS employee_account_repair_log(employee_id BIGINT UNSIGNED PRIMARY KEY,missing_user_id BIGINT UNSIGNED NOT NULL,reason VARCHAR(255) NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4''')
sql("INSERT IGNORE INTO employee_account_repair_log(employee_id,missing_user_id,reason) SELECT e.id,e.user_id,'关联账号已不存在，保留档案并解除无效关联' FROM employees e LEFT JOIN users u ON u.id=e.user_id WHERE e.user_id IS NOT NULL AND u.id IS NULL;UPDATE employees e LEFT JOIN users u ON u.id=e.user_id SET e.user_id=NULL WHERE e.user_id IS NOT NULL AND u.id IS NULL;")
for table in ['users','employees']:
 if sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='department_id'").strip()=='0':
  sql(f'ALTER TABLE {table} ADD COLUMN department_id BIGINT UNSIGNED NULL, ADD INDEX idx_{table}_department_id(department_id)')
if sql("SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='departments' AND column_name='status'").strip()=='0':sql('ALTER TABLE departments ADD COLUMN status TINYINT NOT NULL DEFAULT 1')
def column_exists(table,column):return sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='{column}'").strip()!='0'
sql("CREATE TABLE IF NOT EXISTS employee_department_migration_log(employee_id BIGINT UNSIGNED PRIMARY KEY,user_id BIGINT UNSIGNED NOT NULL,employee_department VARCHAR(64) NOT NULL,account_department VARCHAR(64) NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4")
if column_exists('users','department') and column_exists('employees','department'):
 sql("INSERT INTO departments(name,code,description) SELECT n,CONCAT('legacy_',LEFT(SHA1(n),16)),'历史部门迁移' FROM (SELECT DISTINCT TRIM(department) n FROM users UNION SELECT DISTINCT TRIM(department) FROM employees) old WHERE n<>'' AND NOT EXISTS(SELECT 1 FROM departments d WHERE d.name=old.n)")
 sql("UPDATE users u JOIN departments d ON d.name=TRIM(u.department) SET u.department_id=d.id WHERE u.department_id IS NULL;UPDATE employees e JOIN departments d ON d.name=TRIM(e.department) SET e.department_id=d.id WHERE e.department_id IS NULL;")
 sql("INSERT IGNORE INTO employee_department_migration_log(employee_id,user_id,employee_department,account_department) SELECT e.id,u.id,e.department,u.department FROM employees e JOIN users u ON u.id=e.user_id WHERE e.department<>u.department;UPDATE employees e JOIN users u ON u.id=e.user_id SET e.department_id=u.department_id;")
for table in ['users','employees']:
 constraint='fk_'+table+'_department'
 if sql(f"SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS WHERE CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='{table}' AND CONSTRAINT_NAME='{constraint}'").strip()=='0':sql(f'ALTER TABLE {table} ADD CONSTRAINT {constraint} FOREIGN KEY(department_id) REFERENCES departments(id)')
if sql("SELECT COUNT(*) FROM information_schema.statistics WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='employees' AND INDEX_NAME='uk_employee_account'").strip()=='0':sql('ALTER TABLE employees ADD UNIQUE KEY uk_employee_account(user_id)')
if sql("SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS WHERE CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='employees' AND CONSTRAINT_NAME='fk_employee_account'").strip()=='0':sql('ALTER TABLE employees ADD CONSTRAINT fk_employee_account FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE SET NULL')
sql('''CREATE TABLE IF NOT EXISTS department_legacy_labels(entity_type VARCHAR(16) NOT NULL,entity_id BIGINT UNSIGNED NOT NULL,department_text VARCHAR(64) NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY(entity_type,entity_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4''')
for table in ['users','employees']:
 if column_exists(table,'department'):
  sql(f"INSERT IGNORE INTO department_legacy_labels(entity_type,entity_id,department_text) SELECT '{table}',id,department FROM {table}")
  sql(f'ALTER TABLE {table} DROP COLUMN department')
sql("CREATE OR REPLACE SQL SECURITY INVOKER VIEW user_directory AS SELECT u.*,COALESCE(d.name,'') AS department FROM users u LEFT JOIN departments d ON d.id=u.department_id")
sql("CREATE OR REPLACE SQL SECURITY INVOKER VIEW employee_directory AS SELECT e.*,COALESCE(d.name,'') AS department FROM employees e LEFT JOIN departments d ON d.id=e.department_id")
print('Department IDs are authoritative; labels are joined through directory views; legacy mismatches archived')
