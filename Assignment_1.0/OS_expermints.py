
#Find The longest path way
import os
import time

def FirstDepthSearch(pathway):
    stack = []
    Re = []
    stack.append(pathway);
    while len(stack) > 0:
        Temp = stack.pop(len(stack) - 1)
        if(os.path.isdir(Temp)):
            Re.append(Temp)
            for i in os.listdir(Temp):
                stack.append(os.path.join(Temp, i))
        if(os.path.isfile(Temp)):
            Re.append(Temp)

    return Re

def f():
    start = time.time()
    home = os.path.expanduser("~")
    LongestPath = FirstDepthSearch(home)


    print(max([x for x in LongestPath ],key=lambda x: x.count("/")))
    elapsed = (time.time() - start)
    print("This code took: " + str(elapsed) + " seconds")

