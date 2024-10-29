#!/usr/bin/env python3

#!/usr/bin/env python3

import random
import string

head_user = """INSERT INTO users (id, login, password)
VALUES
"""

head_follow = """INSERT INTO subs (user_id, to_whom)
VALUES
"""


alls = 0

def generateName (length = 10):
    global alls
    alls += 1
    return "user_" + str (alls)

def generateUsers (nb) :
    users = ""
    follow = ""
    nb_follow = 0

    for i in range (0, nb) :
        if (i != 0) :
            users = users + ",\n"

        id = i + 3

        name = generateName ()
        users = users + f"({id}, '{name}', '$2a$12$945DEyAEzxEm7pBb2YSnrukd1ccdC1UvElhbJyJpm8/8yfSRV2kMq')"

        for z in range (nb - int (nb / (i + 1)), nb) :
            if (z + 3 != id) :
                if (nb_follow != 0) :
                    follow = follow + ",\n"
                follow = follow + f"({z + 3}, {id})"
                nb_follow += 1

    return (head_user + users + ";", head_follow + follow + ";")

if __name__ == "__main__" :
    (user,follow) = generateUsers (1000)

    with open ("users.sql", "w") as f :
        f.write (user)

    with open ("socialgraph.sql", "w") as f :
        f.write (follow)
