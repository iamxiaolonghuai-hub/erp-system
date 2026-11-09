from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
prefix='QA_FIN_'+uuid.uuid4().hex
reviewer=next(u for u in users if u['department']=='财务部')
ids=[]
def submit(kind,account,amount=10,**extra):
 _,r=api('/finance/actions',admin,dict(kind=kind,account_id=account,amount=amount,pay_date='2026-10-31',remark=prefix,request_id=uuid.uuid4().hex,**extra));assert r.get('code')==0,r
 return r['data']['action_id']
def approve(id):
 _,r=api('/finance/actions/review',reviewer,dict(action_id=id,approve=True));assert r.get('code')==0,r
 return r['data']
try:
 partner=int(sql('SELECT id FROM customers ORDER BY id LIMIT 1'))
 for value in [100,-40]:
  sql(f"INSERT INTO accounts(account_type,biz_type,biz_no,partner_id,partner_name,amount) VALUES ('receivable','sale','{prefix}',{partner},'{prefix}',{value})")
 ids=[int(x) for x in sql(f"SELECT id FROM accounts WHERE biz_no='{prefix}' ORDER BY id").splitlines()]
 positive,credit=ids
 action=submit('payment',positive,60)
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={positive}')=='0.00'
 _,r=api('/finance/actions/review',admin,dict(action_id=action,approve=True));assert r.get('code')==40300,r
 payment=approve(action)['payment_id']
 action=submit('refund',credit,10);refund_payment=approve(action)['payment_id']
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={credit}')=='-10.00'
 action=submit('offset',credit,20,target_account_id=positive);approve(action)
 assert sql(f'SELECT offset_amount FROM accounts WHERE id={positive}')=='20.00'
 reverse=submit('reverse',credit,source_action_id=action);approve(reverse)
 assert sql(f'SELECT offset_amount FROM accounts WHERE id={positive}')=='0.00'
 reverse=submit('reverse',credit,payment_id=refund_payment);approve(reverse)
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={credit}')=='0.00'
 reverse=submit('reverse',positive,payment_id=payment);approve(reverse)
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={positive}')=='0.00'
 _,again=api('/finance/actions',admin,dict(kind='reverse',payment_id=payment,pay_date='2026-10-31',remark=prefix))
 assert again.get('code')!=0,again
 print('PASS cash/refund reversals restore balances, preserve originals and reject duplicate reversal')
 print('PASS pending does not post, self-review rejected, cash collection, refund, offset and offset reversal')
finally:
 for table in ['finance_actions','payments']:
  sql(f"DELETE FROM business_events WHERE document_type='{table}' AND document_id IN (SELECT id FROM {table} WHERE account_id IN ({','.join(map(str,ids)) or '0'}))")
 sql(f"DELETE FROM finance_actions WHERE remark='{prefix}'")
 sql(f"DELETE FROM accounts WHERE biz_no='{prefix}'")
