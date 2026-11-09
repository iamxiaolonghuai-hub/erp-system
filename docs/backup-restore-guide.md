# 数据库与附件备份恢复

工具在服务器宿主机执行，需要 Python3 和 Docker CLI。直接使用现有 MySQL 容器内的 mysqldump、mysql，以及 ERP 镜像内的 Python，无需公开数据库端口。

备份期间会停止指定后端，禁止该 ERP 实例继续写入；使用同一数据库的其他写入程序也必须暂停。结束或失败后仅在原后端本来运行时重新启动。数据库、附件一起打包并校验，不包含数据库密码和 .env。配置文件应另外妥善保存。

## 备份

在 Bash 中交互输入密码，避免密码进入命令历史：

```bash
read -r -s -p '数据库密码: ' ERP_DB_PASSWORD
export ERP_DB_PASSWORD
python3 deploy/data_backup.py backup \
  --mysql-container 你的MySQL容器名 --database erp_system --db-user 你的数据库用户 \
  --backend-container 你的ERP后端容器名 --output ./backups
unset ERP_DB_PASSWORD
```

数据库用户需要足够的导出权限。Docker名称可用 docker ps -a 查看。备份默认以600权限保存，应复制到其他机器或受控存储。工具不会自动删除旧备份。

## 恢复演练

先准备专用空数据库、停止的目标后端容器及空附件卷。目标容器应连接恢复环境的附件卷，不能误用原生产卷。工具拒绝向非空库或非空附件目录恢复，也拒绝目标后端仍在运行的情况。

```bash
read -r -s -p '数据库密码: ' ERP_DB_PASSWORD
export ERP_DB_PASSWORD
python3 deploy/data_backup.py restore \
  --mysql-container 你的MySQL容器名 --database erp_restore_test --db-user 你的数据库用户 \
  --backend-container 已停止的恢复环境后端容器名 --archive ./backups/实际备份文件.tar.gz
unset ERP_DB_PASSWORD
```

恢复先验证内容与校验和，再写入数据库和附件。目标数据库名可以不同于备份源。完成后不自动启动目标后端，需要先核对配置、表记录和附件，再启动验收。恢复过程中如有失败，目标可能已部分写入；保留失败环境排查，重新准备另一个空库和空卷再试，不要覆盖生产库。

该工具一次在内存中处理备份内容，适用于当前小规模数据。数据较大时应改为流式导出和恢复并评估停机窗口。应定期执行恢复演练，备份成功不等于生产环境恢复能力已经长期验证。
