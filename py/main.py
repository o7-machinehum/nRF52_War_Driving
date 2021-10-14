import requests
import argparse
import time
import pdb
import pickle

def get_mac_details(mac_address):
      
    # We will use an API to get the vendor details
    url = "https://api.macvendors.com/"
      
    # Use get method to fetch details
    response = requests.get(url+mac_address)
    return response.content.decode()

with open("data.txt") as f:
    data = f.read()

rows = data.split("\n")

macs = []
for row in rows:
    try:
        macs.append(row.split(" ")[3])
    except IndexError:
        continue

print("{} Mac addresses".format(len(macs)))

umacs =  []

for mac in macs:
    if mac not in umacs:
        umacs.append(mac)

print("{} Unique Mac addresses".format(len(umacs)))


vendors = {}
for umac in umacs:
    print(umac)
    ret = get_mac_details(umac)
    if ret == '{"errors":{"detail":"Not Found"}}':
        if "unknown" not in vendors:
            vendors["unknown"] = 1
        else:
            vendors["unknown"] += 1
    else:
        if ret not in vendors:
            vendors[ret] = 1
        else:
            vendors[ret] += 1
        
    print(ret)
    time.sleep(1)

with open("data.pkl") as f:
    pickle.dump(vendors, f)
