exec(__import__('pathlib').Path(__file__).with_name('migrate_operations.py').read_text().split('for table,column,definition')[0])
sql('''CREATE TABLE IF NOT EXISTS operational_settings(setting_key VARCHAR(64) PRIMARY KEY,setting_value JSON NOT NULL,updated_by BIGINT UNSIGNED NULL,updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS backup_requests(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,status VARCHAR(16) NOT NULL DEFAULT 'queued',requested_by BIGINT UNSIGNED NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,started_at DATETIME NULL,completed_at DATETIME NULL,details JSON NULL,KEY idx_backup_queue(status,id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS backup_agent(id INT PRIMARY KEY,last_seen_at DATETIME NOT NULL) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;''')
print('Backup configuration, queue and agent status ready')
