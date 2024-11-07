#!/usr/bin/env python3

import requests
import random
import json

def registerUsers (url, nbUsers) :
    url = url + "/new"
    for i in range (1, nbUsers + 1) :
        user = { "login" : f"user_{i}", "password" : "pass" }
        x = requests.post (url, json = user)
        print (i, " ", x)


def logUsers (url, nbUsers):
    uids = {}
    urlLog = url + "/login"

    for i in range (1, nbUsers + 1):
        login = { "login" : f"user_{i}", "password" : "pass" }
        lg = requests.post (urlLog, json = login)
        if (lg.status_code == 200) :
            js = json.loads (lg.text)
            token = js ["token"]
            uid = js ["user_id"]
            uids [i] = js
        print (lg)
    return uids


def makeZipFollow (url, nbUsers) :
    subs = {i : [] for i in range (1, nbUsers + 1)}
    folls = {i : [] for i in range (1, nbUsers + 1)}
    uids = logUsers (url, nbUsers)

    for i in range (1, nbUsers + 1) :
        print (i)
        nbFollows = (nbUsers / i) - 1
        nbFollowed = 0
        while nbFollowed < nbFollows :
            j = random.randint (1, nbUsers)
            if (j != i and (i not in subs [j])):
                nbFollowed += 1
                subs [j].append (i)
                folls [i].append (j)
        print (folls [i])

    print (folls, subs)
    performFollows (url, uids, subs)


def makeFollow2 (url, nbUsers, nbFollows):
    uids = logUsers (url, nbUsers)
    subs = {i + 1 : [] for i in range (nbUsers)}

    for i in range (1, nbUsers) :
        nbFollowed = 0
        while nbFollowed != nbFollows :
            j = random.randint (1, nbUsers)
            if (j != uid):
                nbFollowed += 1
                subs [i].append (i)
    performFollows (url, uids, subs)

def performFollows (url, uids, subs) :
    urlFol = url + "/follow"
    for i in subs :
        for j in subs [i]:
            fl = { "user_id" : uids [i]["user_id"], "to_whom" : uids [j]["user_id"], "token" : uids [i]["token"] }
            x = requests.post (urlFol, json = fl)
            if (x.status_code != 200) :
                print ("Failed : ", x)
                break
            print (x)

def main () :
    #registerUsers ("http://127.0.0.1:8080", 20000)
    makeZipFollow ("http://127.0.0.1:8080", 20000)
    #makeFollow2 ("http://127.0.0.1:8080", 20000, 3)

if __name__ == "__main__" :
    main ()
