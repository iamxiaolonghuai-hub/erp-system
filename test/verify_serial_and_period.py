"""Test serial posting and period close against the disposable ERP verification database."""
from pathlib import Path
from datetime import date, timedelta
from decimal import Decimal

exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
assert db['dbname'] == 'erp_remaining_qa', 'Run only against the isolated verification database'
today = date.fromisoformat(sql('SELECT CURRENT_DATE'))
previous = today.replace(day=1)-timedelta(days=1)
first = previous.replace(day=1)
sql(f"DELETE FROM finance_period_closures WHERE period_start='{first}' AND period_end='{previous}'; "
    "UPDATE finance_period_control SET first_day=CURRENT_DATE,closed_through=NULL WHERE id=1")

def pick(department, role):
    return next(u for u in users if u['department'] == department and role in u['roles'].split(','))

def ok(path, user, payload=None):
    _, response = api(path, user, payload)
    assert response.get('code') == 0, (path, response)
    return response['data']

def reject(path, user, payload):
    _, response = api(path, user, payload)
    assert response.get('code') not in (0, None), (path, response)

buyer, seller = pick('采购部', 'staff'), pick('销售部', 'staff')
picker, approver = pick('仓库部', 'staff'), pick('仓库部', 'leader')
warehouse = int(sql(f"SELECT warehouse_id FROM users WHERE id={picker['id']}"))
supplier = int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'))
customer = int(sql('SELECT id FROM customers WHERE status=1 LIMIT 1'))
tag = 'QA_SER_' + uuid.uuid4().hex[:18]
_, product = api('/products', admin, dict(sku=tag, name=tag, supplier_id=supplier, price=10,
    unit='件', has_serial=1, batch_enabled=1, expiry_enabled=1))
pid = int(product['id'])

def line(q, **extra):
    return dict(product_id=pid, quantity=q, **extra)

def batch(kind, step, order, user, item):
    return dict(type=kind,step=step,order_id=order['order_id'],warehouse_id=warehouse,
                items=[item],request_id=uuid.uuid4().hex)

purchase = ok('/purchase/orders',buyer,dict(supplier_id=supplier,warehouse_id=warehouse,items=[line(2,price=10)]))
ok('/purchase/approve',admin,dict(order_id=purchase['order_id'],approve=True))
dates = dict(lot_no=tag, manufactured_on='2026-01-01', expires_on='2036-01-01')
ok('/orders/batch',picker,batch('purchase','receive',purchase,picker,line(1,serial_nos=tag+'A',**dates)))
reject('/orders/batch',picker,batch('purchase','receive',purchase,picker,line(1,serial_nos=tag+'A',**dates)))
ok('/orders/batch',picker,batch('purchase','receive',purchase,picker,line(1,serial_nos=tag+'B',**dates)))

sale = ok('/sales/orders',seller,dict(customer_id=customer,warehouse_id=warehouse,items=[line(1,price=15)]))
ok('/sales/approve',admin,dict(order_id=sale['order_id'],approve=True))
ok('/orders/batch',picker,batch('sales','prepare',sale,picker,line(1)))
reject('/orders/batch',approver,batch('sales','release',sale,approver,line(1,serial_nos=tag+'C')))
assert sql(f'SELECT reserved_qty FROM inventory WHERE product_id={pid} AND warehouse_id={warehouse}') == '1'
ok('/orders/batch',approver,batch('sales','release',sale,approver,line(1,serial_nos=tag+'A')))

ret = ok('/sales/return',seller,dict(order_id=sale['order_id'],warehouse_id=warehouse,items=[line(1)]))
ok('/sales/return/approve',admin,dict(return_id=ret['return_id'],approve=True))
ret_item = int(sql(f"SELECT id FROM sales_return_items WHERE return_id={ret['return_id']}"))
reject('/sales/return/warehouse',picker,dict(return_id=ret['return_id'],approve=True,
    serial_items=[dict(item_id=ret_item,serial_nos=tag+'B')]))
ok('/sales/return/warehouse',picker,dict(return_id=ret['return_id'],approve=True,
    serial_items=[dict(item_id=ret_item,serial_nos=tag+'A')]))
history = ok('/stock/serials?serial_no='+tag+'A',picker)
assert [entry['action'] for entry in history] == ['sales_return','sale_out','purchase_in'], history
assert history[0]['unit_state'] == 'in_stock'
print('PASS serialized purchase, duplicate rejection, sales release, reservation rollback and original-order return')

report = ok('/finance/period?month='+today.strftime('%Y-%m'),admin)
row = next(r for r in report['rows'] if r['biz_no']==purchase['order_no'])
assert Decimal(row['amount_change'])==20 and Decimal(row['closing'])==20, row
account = row['account_id']
sql(f"UPDATE finance_period_control SET first_day='{first}',closed_through=NULL WHERE id=1")
cash = pick('财务部','staff')
reject('/finance/period/close',cash,dict(month=previous.strftime('%Y-%m')))
month = previous.strftime('%Y-%m')
pending = sql(f"SELECT id FROM finance_actions WHERE status=0 AND pay_date<='{previous}'")
if not pending:
    pending = str(ok('/finance/actions',cash,dict(kind='payment',account_id=account,
        amount=1,pay_date=str(previous)))['action_id'])
assert '待复核' in api('/finance/period/close',admin,dict(month=month))[1]['message']
# Earlier suites leave pending fixtures in this disposable database.
for fixture in pending.splitlines():
    owner = sql(f'SELECT created_by FROM finance_actions WHERE id={int(fixture)}')
    reviewer = admin if str(admin['id']) != owner else pick('财务部','leader')
    ok('/finance/actions/review',reviewer,dict(action_id=int(fixture),approve=False))
ok('/finance/period/close',admin,dict(month=previous.strftime('%Y-%m')))
reject('/finance/period/close',admin,dict(month=previous.strftime('%Y-%m')))
reject('/finance/actions',cash,dict(kind='payment',account_id=account,amount=5,pay_date=str(previous)))
payment = ok('/finance/actions',cash,dict(kind='payment',account_id=account,amount=5,pay_date=str(today)))
ok('/finance/actions/review',admin,dict(action_id=payment['action_id'],approve=True))
row = next(r for r in ok('/finance/period?month='+today.strftime('%Y-%m'),cash)['rows']
           if r['biz_no']==purchase['order_no'])
assert Decimal(row['cash_change'])==5 and Decimal(row['closing'])==15, row
print('PASS period posting, financial-role close, sequential freeze, backdated payment rejection and open-period settlement')
