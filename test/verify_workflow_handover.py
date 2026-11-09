"""Exercise return scope, batch history and atomic handover with temporary local records."""
from pathlib import Path

exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])

tag = 'QA_FLOW_' + uuid.uuid4().hex[:20]
created_users = []
order = returned = product = rejected_application = None
seller = next(u for u in users if u['department'] == '销售部' and 'staff' in u['roles'].split(','))
receiver = next(u for u in users if u['department'] == '销售部' and 'leader' in u['roles'].split(','))
picker = next(u for u in users if u['department'] == '仓库部' and 'staff' in u['roles'].split(','))
warehouse_leader = next(u for u in users if u['department'] == '仓库部' and 'leader' in u['roles'].split(','))
warehouse = int(sql(f"SELECT warehouse_id FROM users WHERE id={picker['id']}"))
other_warehouse = int(sql(f'SELECT id FROM warehouses WHERE id<>{warehouse} AND status=1 LIMIT 1'))
sales_dept = int(sql("SELECT id FROM departments WHERE code='sales'"))
purchase_dept = int(sql("SELECT id FROM departments WHERE code='purchase'"))
warehouse_dept = int(sql("SELECT id FROM departments WHERE code='warehouse'"))

def ok(path, user, data=None):
    _, result = api(path, user, data)
    assert result.get('code') == 0, (path, result)
    return result['data']

def denied(path, user, data, code=40300):
    _, result = api(path, user, data)
    assert result.get('code') == code, (path, result)

def temporary_user(suffix, department_id, department, role, warehouse_id='NULL'):
    name = tag + suffix
    sql(f"INSERT INTO users(username,password,status,department_id,warehouse_id) VALUES('{name}','unused',1,{department_id},{warehouse_id})")
    uid = int(sql(f"SELECT id FROM users WHERE username='{name}'"))
    created_users.append(uid)
    sql(f"INSERT INTO user_roles(user_id,role_id) SELECT {uid},id FROM roles WHERE code='{role}'")
    return dict(id=uid, username=name, department=department, roles=role)

def batch(step, user):
    return ok('/orders/batch', user, dict(type='sales', step=step, order_id=order['order_id'],
        warehouse_id=warehouse, items=[dict(product_id=product, quantity=1)], request_id=uuid.uuid4().hex))

try:
    old_leader = temporary_user('lead', sales_dept, '销售部', 'leader')
    outsider = temporary_user('sale', sales_dept, '销售部', 'staff')
    other_picker = temporary_user('wh', warehouse_dept, '仓库部', 'staff', other_warehouse)
    supplier = int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'))
    customer = int(sql('SELECT id FROM customers WHERE status=1 LIMIT 1'))
    _, result = api('/products', admin, dict(sku=tag, name=tag, supplier_id=supplier, unit='件', price=10, has_serial=0))
    product = int(result['id'])
    ok('/stock/operation/approve', admin, dict(approve=True, operation_id=ok('/stock/operation', picker,
        dict(type='in', to_warehouse_id=warehouse, product_id=product, qty=3))['operation_id']))
    application_payload = dict(type='customer', title=tag, payload=dict(contact='QA'),
        assigned_approver_id=old_leader['id'])
    rejected_application = ok('/application/create', seller, application_payload)['application_id']
    ok('/application/approve', old_leader, dict(application_id=rejected_application, approve=False))
    order = ok('/sales/orders', seller, dict(customer_id=customer, warehouse_id=warehouse,
        assigned_approver_id=old_leader['id'], items=[dict(product_id=product, quantity=2, price=10)]))
    oid = order['order_id']
    preview_path = f"/departments/handover?user_id={old_leader['id']}"
    denied(preview_path, seller, None)
    preview = ok(preview_path, admin)
    assert any(g['department'] == '销售部' and g['count'] == 1 and
        receiver['id'] in {x['id'] for x in g['candidates']} for g in preview), preview
    assignment = dict(user_id=old_leader['id'], department_id=purchase_dept)
    _, delete_result = api('/users/'+str(old_leader['id']), admin, method='DELETE')
    assert delete_result.get('code') == 40900, delete_result
    denied('/users/status', admin, dict(user_id=old_leader['id'], status=0), 40900)
    denied('/departments/assign', admin, assignment, 40900)
    denied('/departments/assign', admin, dict(assignment, handover={'销售部': seller['id']}), 40900)
    assert int(sql(f"SELECT department_id FROM users WHERE id={old_leader['id']}")) == sales_dept
    assert int(sql(f'SELECT assigned_approver_id FROM sales_orders WHERE id={oid}')) == old_leader['id']
    assert ok('/departments/assign', admin, dict(assignment, handover={'销售部': receiver['id']}))['transferred'] == 1
    assert int(sql(f"SELECT department_id FROM users WHERE id={old_leader['id']}")) == purchase_dept
    assert int(sql(f"SELECT COUNT(*) FROM business_events WHERE document_type='sales_orders' AND document_id={oid} AND action='approval_handover' AND operator_id={admin['id']}")) == 1
    ok('/application/resubmit', seller, dict(application_id=rejected_application, title=tag, payload=dict(contact='QA')))
    assert sql(f'SELECT status,assigned_approver_id FROM applications WHERE id={rejected_application}') == '0\tNULL'
    ok('/sales/approve', receiver, dict(order_id=oid, approve=True))
    assert [r['action'] for r in ok('/approval/my-actions?number='+order['order_no']+'&action=approve', receiver)['rows']] == ['approve']

    batch('prepare', picker)
    first = batch('release', warehouse_leader)['delivery_no']
    assert int(sql(f'SELECT status FROM sales_orders WHERE id={oid}')) == 1
    assert [r['batch_no'] for r in ok('/approval/my-actions?mode=warehouse&number='+first, warehouse_leader)['rows']] == [first]
    batch('prepare', picker)
    second = batch('release', admin)['delivery_no']
    history = ok('/approval/my-actions?mode=warehouse&action=ship&number='+order['order_no'], warehouse_leader)['rows']
    assert [r['batch_no'] for r in history] == [first], history
    assert ok('/approval/my-actions?mode=warehouse&number='+second, warehouse_leader)['rows'] == []
    processed_day = history[0]['action_at'][:10]
    assert len(ok(f'/approval/my-actions?from={processed_day}&to={processed_day}&number={first}', warehouse_leader)['rows']) == 1
    assert ok(f'/approval/my-actions?to=2000-01-01&number={first}', warehouse_leader)['rows'] == []

    payload = dict(order_id=oid, warehouse_id=warehouse, items=[dict(product_id=product, quantity=1)])
    denied('/sales/return', outsider, payload)
    returned = ok('/sales/return', seller, payload)
    rid = returned['return_id']
    ok('/sales/return/approve', receiver, dict(return_id=rid, approve=True))
    _, queue = api('/sales/returns?warehouse_queue=1', picker)
    assert any(row['id'] == rid and row['warehouse_id'] == warehouse for row in queue), queue
    _, metadata = api('/approval/assignments?table=sales_returns', picker)
    assert any(row['id'] == rid and row['warehouse_id'] == warehouse for row in metadata), metadata
    _, other_queue = api('/sales/returns?warehouse_queue=1', other_picker)
    assert not any(row['id'] == rid for row in other_queue)
    denied('/sales/return/warehouse', receiver, dict(return_id=rid, approve=True))
    denied('/sales/return/warehouse', other_picker, dict(return_id=rid, approve=True))
    ok('/sales/return/warehouse', picker, dict(return_id=rid, approve=True))
    _, queue = api('/sales/returns?warehouse_queue=1', picker)
    assert not any(row['id'] == rid for row in queue)
    print('PASS handover eligibility and atomicity, approval time, two independent warehouse batches, return ownership and warehouse queue')
finally:
    if rejected_application:
        sql(f"DELETE FROM business_events WHERE document_type='applications' AND document_id={rejected_application};DELETE FROM applications WHERE id={rejected_application}")
    if returned:
        rid = returned['return_id']
        sql(f"DELETE FROM accounts WHERE biz_no=(SELECT return_no FROM sales_returns WHERE id={rid});DELETE FROM business_events WHERE document_type='sales_returns' AND document_id={rid};DELETE FROM sales_returns WHERE id={rid}")
    if order:
        oid = order['order_id']
        sql(f"DELETE FROM accounts WHERE biz_type='sale' AND biz_no='{order['order_no']}';DELETE FROM order_deliveries WHERE order_type='sales' AND order_id={oid};DELETE FROM business_events WHERE document_type='sales_orders' AND document_id={oid};DELETE FROM sales_orders WHERE id={oid}")
    if product:
        sql(f"""DELETE l FROM stock_operation_logs l JOIN stock_operations o ON o.id=l.operation_id WHERE o.product_id={product};
                DELETE e FROM business_events e JOIN stock_operations o ON e.document_type='stock_operations' AND e.document_id=o.id WHERE o.product_id={product};
                DELETE FROM stock_operations WHERE product_id={product};DELETE FROM stock_records WHERE product_id={product};
                DELETE FROM inventory WHERE product_id={product};DELETE FROM products WHERE id={product}""")
    for uid in created_users:
        sql(f"DELETE FROM business_events WHERE document_type='users' AND document_id={uid};DELETE FROM user_roles WHERE user_id={uid};DELETE FROM users WHERE id={uid}")
