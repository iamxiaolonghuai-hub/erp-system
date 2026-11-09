import json, os, sys
from pathlib import Path
config=json.loads(Path('/app/config.template.json').read_text())
for name in ['DB_HOST','DB_NAME','DB_USER','DB_PASSWORD','JWT_SECRET']:
 if not os.environ.get(name):raise SystemExit('Missing environment variable: '+name)
if len(os.environ['JWT_SECRET'])<32:raise SystemExit('JWT_SECRET must contain at least 32 characters')
db=config['db_clients'][0]
for key,name in [('host','DB_HOST'),('dbname','DB_NAME'),('user','DB_USER'),('passwd','DB_PASSWORD')]:db[key]=os.environ[name]
db['port']=int(os.environ.get('DB_PORT','3306'))
config['custom_config']['jwt_secret']=os.environ['JWT_SECRET']
Path('/app/config.json').write_text(json.dumps(config))
os.chmod('/app/config.json',0o600)
os.execvp(sys.argv[1],sys.argv[1:])
