from jfconf import db
from py68 import pytime
import stockdb

sdb = stockdb.Client()

req = stockdb.Request()
req.fields = "*"
req.keys = "000001.SZ,600519.SH"
req.fmt = "{}"
for x in sdb.get(req):
    print(x)
