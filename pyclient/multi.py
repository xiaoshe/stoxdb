import time
import concurrent.futures as thread
from jfconf import db
import stoxdb


# 执行具体任务
def do_task(code):
    xdb = stoxdb.Client()
    req = stoxdb.Request()
    req.fields = "code,name,incr,close,mv"
    req.keys = code
    for x in xdb.get(req):
        print(x)
    print("Task {} was completed ".format(code))
    return code

executor = thread.ThreadPoolExecutor(max_workers=64) # 6个线程

rs = db.Redis2()
codelist = []
for x in rs.all():
    codelist.append(x["code"])

all_task = [] # 全部任务
for code in codelist:
    t = executor.submit(do_task, code)
    all_task.append(t)

for x in thread.as_completed(all_task):
    pass

