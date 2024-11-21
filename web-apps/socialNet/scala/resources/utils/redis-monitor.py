import subprocess
import time
import sys

lastHits = 0
lastMisses = 0

time.sleep (4)
with open (sys.argv[2], "w") as fp:
    fp.write ("HITS;MISSES;MEMORY\n")
    while True :
        hits = 0
        misses = 0
        mem = 0
        with subprocess.Popen(["redis-cli", "-p", sys.argv[1], "info", "stats"], stdout=subprocess.PIPE) as proc:
            lines = (proc.stdout.read ().decode ("utf-8"))
            for i in lines.splitlines () :
                if ("keyspace_hits" in i):
                    hits = int (i [len ("keyspace_hits") + 1:])
                elif ("keyspace_misses" in i):
                    misses = int (i [len ("keyspace_misses") + 1:])

        with subprocess.Popen (["redis-cli", "-p", sys.argv[1], "info", "memory"], stdout=subprocess.PIPE) as proc:
            lines = (proc.stdout.read ().decode ("utf-8"))
            for i in lines.splitlines () :
                if ("used_memory:" in i):
                    mem = int(i[len ("used_memory:"):])

        fp.write (f"{hits - lastHits};{misses - lastMisses};{round (mem/(1024*1024),2)}\n")
        lastMisses = misses
        lastHits = hits

        time.sleep (1)
        fp.flush ()
