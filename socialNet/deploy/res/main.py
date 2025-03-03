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


def logUser (url, user) :
    login = { "login" : f"user_{user}", "password" : "pass" }
    lg = requests.post (url + "/login", json = login)
    if (lg.status_code == 200) :
        js = json.loads (lg.text)
        print ("Log ", user, " ", js)
        return js
    else :
        print ("Log fails : ", lg)
        return None

def makeZipFollow (url, nbUsers) :
    subs = {i : [] for i in range (1, nbUsers + 1)}
    folls = {i : [] for i in range (1, nbUsers + 1)}

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
    performFollows (url, subs)


# First 'nbAll' users are followed by everyone, then a makeFollow2 (url, nbUsers, nbFollows) is performed
def makeFollowAllNb_then2 (url, nbUsers, nbAll, nbFollows):
    subs = {i : [] for i in range (1, nbUsers + 1)}
    for i in range (1, nbAll) :
        for j in range (1, nbUsers + 1):
            if j != i and (i not in subs [j]):
                subs [j].append (i)

    for i in range (nbAll + 1, nbUsers + 1) :
        nbFollowed = 0
        while nbFollowed != nbFollows :
            j = random.randint (nbAll + 1, nbUsers)
            if (j != i):
                nbFollowed += 1
                subs [i].append (j)

    print (subs)
    performFollows (url, subs)


def makeFollow2 (url, nbUsers, nbFollows):
    subs = {i + 1 : [] for i in range (nbUsers)}

    for i in range (11, nbUsers) :
        nbFollowed = 0
        while nbFollowed != nbFollows :
            j = random.randint (1, nbUsers)
            if (j != i):
                nbFollowed += 1
                subs [i].append (j)

    print (subs)
    performFollows (url, subs)

def performFollows (url, subs) :
    uids = {}
    urlFol = url + "/follow"
    for i in subs :
        if i not in uids :
            uids [i] = logUser (url, i)

        for j in subs [i]:
            if j not in uids :
                uids [j] = logUser (url, j)

            fl = { "user_id" : uids [i]["user_id"], "to_whom" : uids [j]["user_id"], "token" : uids [i]["token"] }
            x = requests.post (urlFol, json = fl)
            if (x.status_code != 200) :
                print ("Failed : ", x)
                break
            print ("Follow ", i, " => ", j, " ", x, "/", len (subs))

def main () :
    url = "http://192.168.1.21:8080"
    nbUsers = 2000
    registerUsers (url, nbUsers)
    makeFollowAllNb_then2 (url, nbUsers, 11, 3)

if __name__ == "__main__" :
    main ()
