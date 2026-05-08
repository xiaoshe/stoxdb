import time
from jfconf import db
from py68 import pytime
import stoxdb

rs = db.Redis2()
sdb = stoxdb.Client()

while True:
    second = pytime.second()
    if ("09:30:00" <= second <= "11:31:00") or ("13:00:00" <= second <= "15:01:00"):
        tm1 = int(time.time() *1000)
        count = 0
        for x in rs.all():
            x.pop("level5")
        
            x["type"] = x["type2"]
            x.pop("type2")
            x["date"] = pytime.date(x["time"])
            x["close"] = x["price"]
            x.pop("price")
            x["vol"] = int(x["vol"])
            x["amt"] = int(x["amt"])
        
            sdb.set(x["code"], x)
            count += 1
        tm2 = int(time.time() *1000)
        print("update stock ok. count:{} time:{}ms".format(count, tm2-tm1))
        time.sleep(1)

    elif second > "15:01:00":
        break
    else:
        time.sleep(10)
