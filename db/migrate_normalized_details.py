"""Move business lists to relational rows. Stop ERP writers before running this migration."""
from pathlib import Path
import sys
from decimal import Decimal
exec(Path(__file__).with_name('migrate_operations.py').read_text().split('for table,column,definition')[0])
def exists(table,column):return sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='{column}'").strip()!='0'
serials=[];deliveries=[]
returns=[('purchase_returns','supplier_id','purchase_orders'),('sales_returns','customer_id','sales_orders')]
for table,column,source in returns:
 if exists(table,column) and sql(f'SELECT COUNT(*) FROM {table} r LEFT JOIN {source} o ON o.id=r.order_id WHERE o.id IS NULL OR r.{column}<>o.{column}').strip()!='0':raise RuntimeError(f'{table}: original order and partner disagree; migration refused')
for table in ['purchase_order_items','sales_order_items','purchase_return_items','sales_return_items']:
 if sql(f'SELECT COUNT(*) FROM {table} WHERE amount<>quantity*price OR quantity<=0 OR price<0').strip()!='0':raise RuntimeError(f'{table}: invalid amounts; migration refused')
if sql('SELECT COUNT(*) FROM stock_check_items WHERE diff_qty<>actual_qty-book_qty').strip()!='0':raise RuntimeError('Invalid stock check differences; migration refused')
if exists('stock_operations','serial_nos'):
 for line in sql("SELECT id,HEX(COALESCE(serial_nos,'')) FROM stock_operations ORDER BY id").splitlines():
  pieces=line.split('\t');oid=int(pieces[0]);raw=bytes.fromhex(pieces[1] if len(pieces)>1 else '').decode();values=[value.strip() for value in raw.split(',') if value.strip()]
  if len(values)!=len(set(values)) or any(len(value)>128 for value in values):raise RuntimeError(f'Operation {oid}: invalid legacy serials; migration refused')
  serials.append((oid,raw,values))
if exists('order_deliveries','items'):
 for line in sql('SELECT id,order_type,order_id,amount,HEX(items) FROM order_deliveries ORDER BY id').splitlines():
  did,kind,oid,amount,encoded=line.split('\t');did=int(did);oid=int(oid)
  if kind not in ('purchase','sales'):raise RuntimeError(f'Delivery {did}: unknown order type')
  raw=bytes.fromhex(encoded).decode();items=json.loads(raw);rows=[];seen=set();total=Decimal('0')
  if not isinstance(items,list) or not items:raise RuntimeError(f'Delivery {did}: no lines')
  for item in items:
   pid=int(item['product_id']);qty=int(item['quantity'])
   if qty<=0 or pid in seen:raise RuntimeError(f'Delivery {did}: invalid line')
   seen.add(pid);price=sql(f'SELECT price FROM {kind}_order_items WHERE order_id={oid} AND product_id={pid}')
   if not price or '\n' in price:raise RuntimeError(f'Delivery {did}: original price unavailable; migration refused')
   price=Decimal(price);total+=qty*price;rows.append((pid,qty,price))
  if total!=Decimal(amount):raise RuntimeError(f'Delivery {did}: header and detail amounts differ; migration refused')
  deliveries.append((did,raw,rows))
if '--check-only' in sys.argv:
 print('Normalized detail preflight passed; no data changed')
 raise SystemExit(0)
sql('''CREATE TABLE IF NOT EXISTS normalization_archive(entity_type VARCHAR(32) NOT NULL,entity_id BIGINT UNSIGNED NOT NULL,legacy_payload JSON NOT NULL,created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,PRIMARY KEY(entity_type,entity_id)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS stock_operation_serials(operation_id BIGINT UNSIGNED NOT NULL,line_no INT UNSIGNED NOT NULL,serial_no VARCHAR(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,PRIMARY KEY(operation_id,serial_no),UNIQUE KEY uk_operation_line(operation_id,line_no),CONSTRAINT fk_serial_operation FOREIGN KEY(operation_id) REFERENCES stock_operations(id) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
CREATE TABLE IF NOT EXISTS order_delivery_items(delivery_id BIGINT UNSIGNED NOT NULL,product_id BIGINT UNSIGNED NOT NULL,quantity INT NOT NULL,unit_price DECIMAL(12,2) NOT NULL,PRIMARY KEY(delivery_id,product_id),CONSTRAINT fk_delivery_item_header FOREIGN KEY(delivery_id) REFERENCES order_deliveries(id) ON DELETE CASCADE,CONSTRAINT fk_delivery_item_product FOREIGN KEY(product_id) REFERENCES products(id),CHECK(quantity>0),CHECK(unit_price>=0)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4''')
def literal(value):return "CONVERT(0x"+value.encode().hex()+" USING utf8mb4)" if value else "''"
for oid,raw,values in serials:
 statements=[f"INSERT IGNORE INTO normalization_archive VALUES('stock_serials',{oid},JSON_OBJECT('serial_nos',{literal(raw)}),NOW())",f'DELETE FROM stock_operation_serials WHERE operation_id={oid}']
 statements += [f'INSERT INTO stock_operation_serials(operation_id,line_no,serial_no) VALUES({oid},{index},{literal(value)})' for index,value in enumerate(values,1)]
 sql('START TRANSACTION;'+';'.join(statements)+';COMMIT;')
for did,raw,rows in deliveries:
 statements=[f"INSERT IGNORE INTO normalization_archive VALUES('delivery_items',{did},{literal(raw)},NOW())",f'DELETE FROM order_delivery_items WHERE delivery_id={did}']
 statements += [f'INSERT INTO order_delivery_items(delivery_id,product_id,quantity,unit_price) VALUES({did},{pid},{qty},{price})' for pid,qty,price in rows]
 sql('START TRANSACTION;'+';'.join(statements)+';COMMIT;')
if exists('stock_operations','serial_nos'):sql('ALTER TABLE stock_operations DROP COLUMN serial_nos')
if exists('order_deliveries','items'):sql('ALTER TABLE order_deliveries DROP COLUMN items')
for table,column,source in returns:
 if exists(table,column):
  sql(f"INSERT IGNORE INTO normalization_archive(entity_type,entity_id,legacy_payload) SELECT '{table}',id,JSON_OBJECT('order_id',order_id,'{column}',{column}) FROM {table}")
  sql(f'ALTER TABLE {table} DROP COLUMN {column}')
 constraint='fk_normalized_'+table
 if sql(f"SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS WHERE CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='{table}' AND CONSTRAINT_NAME='{constraint}'").strip()=='0':sql(f'ALTER TABLE {table} ADD CONSTRAINT {constraint} FOREIGN KEY(order_id) REFERENCES {source}(id)')
for table in ['purchase_order_items','sales_order_items','purchase_return_items','sales_return_items','stock_check_items']:
 constraint='chk_normalized_'+table
 rule='diff_qty=actual_qty-book_qty' if table=='stock_check_items' else 'amount=quantity*price AND quantity>0 AND price>=0'
 if sql(f"SELECT COUNT(*) FROM information_schema.TABLE_CONSTRAINTS WHERE CONSTRAINT_SCHEMA=DATABASE() AND TABLE_NAME='{table}' AND CONSTRAINT_NAME='{constraint}'").strip()=='0':sql(f'ALTER TABLE {table} ADD CONSTRAINT {constraint} CHECK({rule})')
print('Serials and delivery details normalized; original payloads retained in immutable migration archives')
