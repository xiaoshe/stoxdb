from jfconf import db
from py68 import pytime
import stockdb

sdb = stockdb.Client()

x = {
    "code":"000001.SZ",
    "name":"平安银行",
    "close":11.2,
}
sdb.set("000001.SZ", x)
